#include "NetworkManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <ArduinoJson.h>

#include "AudioControl.h"
#include "LampControl.h"
#include "ui.h"

namespace {
constexpr const char* WIFI_SSID = "L'espoir fait vivre";
constexpr const char* WIFI_PASSWORD = "ekip31470";

constexpr const char* MQTT_SERVER = "test.mosquitto.org";
constexpr uint16_t MQTT_PORT = 1883;

constexpr const char* MQTT_AUDIO_REQUEST_TOPIC = "esp32/audio/request";
constexpr const char* MQTT_AUDIO_RESPONSE_TOPIC = "esp32/audio/response";

constexpr const char* NTP_SERVER = "time.windows.com";
constexpr long GMT_OFFSET_SEC = 3600;
constexpr int DAYLIGHT_OFFSET_SEC = 3600;

constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 5000;
constexpr unsigned long MQTT_RETRY_INTERVAL_MS = 3000;

WiFiClient g_wifiClient;
PubSubClient g_mqttClient(g_wifiClient);

bool g_wifiConnected = false;
bool g_mqttConnected = false;
bool g_ntpConfigured = false;

unsigned long g_wifiLastAttempt = 0;
unsigned long g_mqttLastAttempt = 0;

bool g_audioRequestActive = false;
bool g_audioWasRunning = false;
char g_audioCurrentUrl[256] = {0};
unsigned long g_audioStartTimestamp = 0;

void resetAudioRequestState() {
  g_audioRequestActive = false;
  g_audioWasRunning = false;
  g_audioCurrentUrl[0] = '\0';
  g_audioStartTimestamp = 0;
}

void publishAudioResponse(const char* status, const char* url = nullptr, const char* message = nullptr) {
  StaticJsonDocument<256> doc;
  doc["status"] = status;
  if (url && url[0] != '\0') {
    doc["url"] = url;
  }
  if (message && message[0] != '\0') {
    doc["message"] = message;
  }

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len >= sizeof(buffer)) {
    len = sizeof(buffer) - 1;
  }
  buffer[len] = '\0';
  g_mqttClient.publish(MQTT_AUDIO_RESPONSE_TOPIC, buffer);
}

void handleAudioPlaybackState() {
  if (!g_audioRequestActive) {
    return;
  }

  // Si flux vraiment terminé alors publier l'événement
  char infoBuf[64];
  if (AudioControl_consumeStreamFinished(infoBuf, sizeof(infoBuf))) {
    const char* msg = "lecture terminee";
    if (infoBuf[0] != '\0') {
      if (strcmp(infoBuf, g_audioCurrentUrl) == 0) {
        msg = infoBuf;
      } else {
        // Événement EOF d'un flux précédent: ne pas confondre l'utilisateur
        printf("\n[Audio] EOF d'un autre flux (info='%s' != current='%s')\n", infoBuf, g_audioCurrentUrl);
      }
    }
    publishAudioResponse("completed", g_audioCurrentUrl, msg);
    resetAudioRequestState();
    return;
  }

  bool running = AudioControl_isRunning();
  if (running) {
    g_audioWasRunning = true;
    return;
  }

  if (g_audioWasRunning) {
    publishAudioResponse("completed", g_audioCurrentUrl, "lecture terminee");
    resetAudioRequestState();
    return;
  }

  // Si lecture jamais démarré après un court délai, considérer comme une erreur
  if (g_audioStartTimestamp != 0 && millis() - g_audioStartTimestamp > 2000UL) {
    publishAudioResponse("error", g_audioCurrentUrl, "lecture non demarree (timeout)");
    resetAudioRequestState();
  }
}

void handleAudioRequest(uint8_t* payload, unsigned int length) {
  printf("\nMessage reçu sur %s, lecture audio HTTP", MQTT_AUDIO_REQUEST_TOPIC);

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    printf("\nErreur de parsing JSON pour la requête audio: %s", error.c_str());
    publishAudioResponse("error", "", "JSON invalide");
    return;
  }

  const char* url = doc["url"] | "";
  if (!url || url[0] == '\0') {
    printf("\nURL manquante dans la requête audio");
    publishAudioResponse("error", "", "URL manquante");
    return;
  }

  if (g_audioRequestActive && g_audioCurrentUrl[0] != '\0') {
    publishAudioResponse("stopped", g_audioCurrentUrl, "lecture interrompue par une nouvelle requete");
  }

  bool wasRunning = AudioControl_isRunning();
  if (wasRunning) {
    AudioControl_stop();
    AudioControl_consumeStreamFinished(nullptr, 0);
  }

  resetAudioRequestState();

  if (strlen(url) >= sizeof(g_audioCurrentUrl)) {
    printf("\nURL audio trop longue");
    publishAudioResponse("error", "", "URL trop longue");
    return;
  }

  strlcpy(g_audioCurrentUrl, url, sizeof(g_audioCurrentUrl));

  if (!playHTTPStream(g_audioCurrentUrl)) {
    printf("\nImpossible de démarrer le flux HTTP: %s", g_audioCurrentUrl);
    publishAudioResponse("error", g_audioCurrentUrl, "impossible de demarrer la lecture");
    g_audioCurrentUrl[0] = '\0';
    return;
  }

  g_audioRequestActive = true;
  g_audioWasRunning = AudioControl_isRunning();
  g_audioStartTimestamp = millis();
  publishAudioResponse("playing", g_audioCurrentUrl, "lecture en cours");
}

void applyLampDisconnectedState() {
  LampControl_onMqttDisconnected();
}

void handleColorMessage(uint8_t* payload, unsigned int length) {
  char colorStr[7] = {0};
  if (length < 6) {
    printf("\nPayload couleur invalide");
    return;
  }

  for (unsigned int i = 0; i < 6 && i < length; ++i) {
    colorStr[i] = static_cast<char>(payload[i]);
  }

  uint32_t color = static_cast<uint32_t>(strtol(colorStr, nullptr, 16));
  if (ui_FindAppScreen) {
    lv_obj_set_style_bg_color(ui_FindAppScreen, lv_color_hex(color), LV_PART_MAIN);
  }
}

void handleLampAck(uint8_t* payload, unsigned int length) {
  char buf[16];
  unsigned int n = (length < sizeof(buf) - 1) ? length : sizeof(buf) - 1;
  memcpy(buf, payload, n);
  buf[n] = '\0';
  for (char* p = buf; *p; ++p) {
    *p = static_cast<char>(toupper(static_cast<unsigned char>(*p)));
  }
  bool on = (strcmp(buf, "ON") == 0 || strcmp(buf, "1") == 0 || strcmp(buf, "TRUE") == 0);
  LampControl_handleAck(on);
}

void startWiFiAttempt() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  g_wifiLastAttempt = millis();
  printf("\n[WiFi] Tentative de connexion...");
}

void attemptMQTTOnce() {
  if (!g_wifiConnected) {
    return;
  }

  if (g_mqttClient.connect("EchoWatchClient")) {
    printf("\n[MQTT] Connecté");
    g_mqttClient.subscribe("esp32/color");
    g_mqttClient.subscribe("esp32/sound");
    g_mqttClient.subscribe("esp32/lampe/ack");
    g_mqttClient.subscribe("esp32/tts");
    g_mqttClient.subscribe(MQTT_AUDIO_REQUEST_TOPIC);
    g_mqttConnected = true;
    LampControl_onMqttConnected();
  } else {
    printf("\n[MQTT] Échec code=%d\n", g_mqttClient.state());
    g_mqttConnected = false;
  }

  g_mqttLastAttempt = millis();
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  // Changement de couleur sur l'application FindApp
  if (strcmp(topic, "esp32/color") == 0) {
    printf("\nMessage reçu sur esp32/color, changement de couleur");
    handleColorMessage(payload, length);
  }
  // Commande de lecture/pause de la musique
  else if (strcmp(topic, "esp32/sound") == 0) {
    printf("\nMessage reçu sur esp32/sound, jouer/pause musique");
    playMusic();
  }
  // Synthèse vocale avec le texte passé en payload
  else if (strcmp(topic, "esp32/tts") == 0) {
    printf("\nMessage reçu sur esp32/tts, synthèse vocale");
    // Payload forme : { text : "le texte à dire", "lang" : "fr" }
    char text[256] = {0};
    char lang[8] = {0};
    // Extraction des champs avec ArduinoJson
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    // Stock le message d'erreur dans response
    const char* response = "";
    if (error) {
      printf("\nErreur de parsing JSON");
      response = "Erreur de parsing JSON";
      g_mqttClient.publish("esp32/tts/response", response);
      return;
    }
    strlcpy(text, doc["text"] | "", sizeof(text));
    strlcpy(lang, doc["lang"] | "fr", sizeof(lang));
    if (text[0] == '\0') {
      printf("\nTexte vide pour la synthèse vocale");
      response = "Texte vide pour la synthèse vocale";
      g_mqttClient.publish("esp32/tts/response", response);
      return;
    }
    bool success = playTextToSpeech(text, lang);
    response = success ? "Synthèse vocale démarrée" : "Erreur de synthèse vocale";
    g_mqttClient.publish("esp32/tts/response", response);
  }
  else if (strcmp(topic, MQTT_AUDIO_REQUEST_TOPIC) == 0) {
    handleAudioRequest(payload, length);
  }
  // Accusé de réception de la lampe
  else if (strcmp(topic, "esp32/lampe/ack") == 0) {
    handleLampAck(payload, length);
  } 
  // Topic inconnu
  else {
    printf("\nMessage reçu sur topic inconnu: %s\n", topic);
  }
}
}  // namespace

void NetworkManager_begin() {
  g_mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  g_mqttClient.setCallback(mqttCallback);
  startWiFiAttempt();
}

void NetworkManager_loop() {
  bool curWifi = (WiFi.status() == WL_CONNECTED);

  if (curWifi && !g_wifiConnected) {
    g_wifiConnected = true;
    String ipStr = WiFi.localIP().toString();
    printf("\n[WiFi] Connecté. IP: %s\n", ipStr.c_str());
  } else if (!curWifi && g_wifiConnected) {
    g_wifiConnected = false;
    g_mqttConnected = false;
    applyLampDisconnectedState();
  }

  if (!g_wifiConnected) {
    unsigned long now = millis();
    if (now - g_wifiLastAttempt >= WIFI_RETRY_INTERVAL_MS) {
      startWiFiAttempt();
    }
  }

  if (g_wifiConnected && !g_ntpConfigured) {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    g_ntpConfigured = true;
    printf("\n[NTP] Configuration envoyée");
  }

  bool curMqtt = g_mqttClient.connected();
  if (g_wifiConnected) {
    if (!curMqtt) {
      if (g_mqttConnected) {
        g_mqttConnected = false;
        applyLampDisconnectedState();
      }
      unsigned long now = millis();
      if (now - g_mqttLastAttempt >= MQTT_RETRY_INTERVAL_MS) {
        attemptMQTTOnce();
      }
    } else {
      g_mqttConnected = true;
      g_mqttClient.loop();
    }
  }

  handleAudioPlaybackState();
}

bool NetworkManager_isWifiConnected() {
  return g_wifiConnected;
}

bool NetworkManager_isMqttConnected() {
  return g_mqttConnected && g_mqttClient.connected();
}

bool NetworkManager_isNtpConfigured() {
  return g_ntpConfigured;
}

PubSubClient& NetworkManager_getClient() {
  return g_mqttClient;
}
