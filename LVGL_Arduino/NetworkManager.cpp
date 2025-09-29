#include "NetworkManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <ArduinoJson.h>

#include "AudioControl.h"
#include "LampControl.h"
#include "GyroPublisher.h"
#include "ui.h"

namespace {
constexpr const char* WIFI_SSID = "iPhone de Melvin";
constexpr const char* WIFI_PASSWORD = "motdepasse2";

constexpr const char* MQTT_SERVER = "test.mosquitto.org";
constexpr uint16_t MQTT_PORT = 1883;

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
    g_mqttClient.subscribe("esp32/http");
    g_mqttClient.subscribe("esp32/sensors/request");
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
  // Lecture d'un audio via flux HTTP (exemple fixe ici mais à modifier)
  // else if (strcmp(topic, "esp32/http") == 0) {
  //   printf("\nMessage reçu sur esp32/http, lecture d'un flux HTTP");
  //   if (playHTTPStream("https://raw.githubusercontent.com/Perceval00731/EchoWatch/master/sample-1.wav")) {
  //     printf("\nLecture du flux HTTP démarrée");
  //   }
  // } 
  // Accusé de réception de la lampe
  else if (strcmp(topic, "esp32/lampe/ack") == 0) {
    handleLampAck(payload, length);
  }
  // Requête de configuration des capteurs
  else if (strcmp(topic, "esp32/sensors/request") == 0) {
    printf("\nMessage reçu sur esp32/sensors/request, configuration des capteurs");
    char json_str[512] = {0};
    unsigned int copy_len = (length < sizeof(json_str) - 1) ? length : sizeof(json_str) - 1;
    memcpy(json_str, payload, copy_len);
    json_str[copy_len] = '\0';
    GyroPublisher_handleRequest(json_str);
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
