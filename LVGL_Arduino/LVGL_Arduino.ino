/* Using LVGL with Arduino requires some extra steps:
 * Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  
 */

#include "Display_SPD2010.h"  // Affichage
#include "Touch_SPD2010.h"    // Tactile
#include "LVGL_Driver.h"      // LVGL
#include "Audio_PCM5101.h"    // Son
#include "MIC_MSM.h"          // Micro
#include <WiFi.h>             // Wi-Fi
#include "SD_Card.h"          // Carte SD
#include "ui.h"               // UI LVGL
#include <PubSubClient.h>     // MQTT
#include "BAT_Driver.h"       // Batterie
#include "RTC_PCF85063.h"     // RTC
#include "time.h"

extern "C" void playMusic();
extern "C" void setVolume(int volume);

// ---- Paramétrage WiFi ----

const char* ssid_local = "iPhone de Melvin";
const char* password_local = "motdepasse2";

// ---- Paramétrage NTP ----
const char* ntpServer = "time.windows.com";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// ---- Paramétrage MQTT ----

const char* mqtt_server = "test.mosquitto.org";
WiFiClient espClient;
PubSubClient client(espClient);


// ---- Variables d'état pour gérer l'échec de connexion ----
bool wifiConnected = false;
bool mqttConnected = false;


// connect wifi/mqtt (non bloquant)

static unsigned long wifiLastAttempt = 0;
static const unsigned long WIFI_RETRY_INTERVAL = 5000;
static bool ntpConfigured = false;

static unsigned long mqttLastAttempt = 0;
static const unsigned long MQTT_RETRY_INTERVAL = 3000;

void startWiFiAttempt() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_local, password_local);
  wifiLastAttempt = millis();
  Serial.println("[WiFi] Tentative de connexion...");
}

void attemptMQTTOnce() {
  if (!wifiConnected) return;
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  if (client.connect("EchoWatchClient")) {
    Serial.println("[MQTT] Connecté");
    client.subscribe("esp32/color");
    client.subscribe("esp32/sound");
    mqttConnected = true;
  } else {
    Serial.print("[MQTT] Échec code=");
    Serial.println(client.state());
    mqttConnected = false;
  }
  mqttLastAttempt = millis();
}

void networkLoop() {
  if (!wifiConnected) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.print("[WiFi] Connecté. IP: ");
      Serial.println(WiFi.localIP());
    } else {
      unsigned long now = millis();
      if (now - wifiLastAttempt >= WIFI_RETRY_INTERVAL) {
        startWiFiAttempt();
      }
    }
  }

  // config NTP après wifi (une seule fois)
  if (wifiConnected && !ntpConfigured) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    ntpConfigured = true;
    Serial.println("[NTP] Configuration envoyée");
  }

  // mqtt
  if (wifiConnected) {
    if (!client.connected()) {
      unsigned long now = millis();
      if (now - mqttLastAttempt >= MQTT_RETRY_INTERVAL) {
        attemptMQTTOnce();
      }
    } else {
      mqttConnected = true;
      client.loop();
    }
  }
}


// ---- Callback MQTT ----

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  if (strcmp(topic, "esp32/color") == 0) { // Message sur esp32/color
    mqttMessageColor(payload, length);
  } 
  else if (strcmp(topic, "esp32/sound") == 0) { // Message sur esp32/sound
    playMusic();
  }
}


// ---- Fonction executée lors d'un message sur esp32/color ----

void mqttMessageColor(uint8_t* payload, unsigned int length) {
  // Convertir le payload en chaîne de caractères
  char colorStr[7] = {0};
  if (length >= 6) {
    for (unsigned int i = 0; i < 6 && i < length; i++) {
      colorStr[i] = (char)payload[i];
    }
    // Conversion et application de la couleur
    uint32_t color = (uint32_t)strtol(colorStr, NULL, 16);
    // Une fois la couleur obtenue, on en fait ce qu'on veut, ici, changer le fond d'écran de l'app Finder
    if (ui_FindAppScreen) {
      lv_obj_set_style_bg_color(ui_FindAppScreen, lv_color_hex(color), LV_PART_MAIN);
    }
  } else {
    Serial.println("Payload couleur invalide"); // Erreur de format dans le message MQTT
  }
}

// ---- Boucle principale ----

// ---- Heure ---- 

void updateTime() {
  struct tm timeinfo;
  bool updated = false;
  // Essayer une récupération ultra rapide (0 ms) si le NTP a déjà synchronisé
  if (wifiConnected && ntpConfigured && getLocalTime(&timeinfo, 0)) {
    updated = true;
  }
  // Fallback: utiliser l'heure du RTC (fourni par PCF85063_Loop) via la structure globale 'datetime'
  if (!updated) {
    // On suppose que 'datetime' est maintenu à jour ailleurs (PCF85063_Loop)
    timeinfo.tm_hour = datetime.hour;
    timeinfo.tm_min  = datetime.minute;
  }
  if (ui_Hour) {
    char hourStr[6];
    snprintf(hourStr, sizeof(hourStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    lv_label_set_text(ui_Hour, hourStr);
  }
}


// ---- Initialisation ----

void Init() {
  I2C_Init();
  Backlight_Init();
  Set_Backlight(50);
  LCD_Init();
  Lvgl_Init();
  SD_Init();
  Touch_Init();
  Audio_Init();
  MIC_Init();
  PCF85063_Init();
  ui_init();
}

// ---- Setup ----

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("===== Démarrage =====");
  Init();
  startWiFiAttempt(); // appel non bloquant
  Serial.println("Début Musique");
  Serial.println("===== Setup terminé =====");
  Serial.println("Get Battery");
  Serial.println(BAT_Get_Volts());
}

// ---- Loop ----

unsigned long lastUpdateTime = 0;
static unsigned long lastBatteryUpdateTime = 0; // derniere maj de la batterie en ms

// Calcul du pourcentage batterie à partir de la tension
//    >4.10V = 100%
//    3.70V - 4.10V -> interpolation 50% à 99%
//    3.50V - 3.70V -> interpolation 20% à 49%
//    <3.50V  -> interpolation de 0% à 19% entre 3.20V et 3.50V (en dessous de 3.2 on considère que c'est éteint)
static int computeBatteryPercent(float volts) {
  if (volts >= 4.10f) return 100;
  if (volts >= 3.70f) {
    // Map 3.70-4.10 -> 50-99
    float ratio = (volts - 3.70f) / (4.10f - 3.70f);
    int pct = 50 + (int)roundf(ratio * 49.0f);
    if (pct > 99) pct = 99;
    return pct;
  }
  if (volts >= 3.50f) {
    // Map 3.50-3.70 -> 20-49
    float ratio = (volts - 3.50f) / (3.70f - 3.50f);
    int pct = 20 + (int)roundf(ratio * 29.0f);
    if (pct > 49) pct = 49;
    return pct;
  }
  // En dessous de 3.50V : 0-19% entre 3.20V et 3.50V
  const float LOW_MIN = 3.20f;
  const float LOW_MAX = 3.50f; // correspond à 19%
  if (volts <= LOW_MIN) return 0;
  if (volts >= LOW_MAX) return 19; // au cas ou mais normalement géré avant
  float ratio = (volts - LOW_MIN) / (LOW_MAX - LOW_MIN);
  int pct = (int)roundf(ratio * 19.0f);
  if (pct > 19) pct = 19;
  if (pct < 0) pct = 0;
  return pct;
}

// Fonction pour mettre à jour les informations affichées (toutes les secondes pour éviter de surcharger le CPU en recalculant à chaque loop)
void updateDisplayInfo() {
  // --- MAJ de l'affichage de la batterie ---
  auto updateBatteryPanels = [](float volts){
    // on détermine le niveau selon des seuils de voltage
    // >4.1V = 100% (3 panneaux)
    // 3.7 - 4.1V = 50-99% (2 panneaux)
    // 3.5 - 3.7V = 20-49% (1 panneau)
    // <3.5V = <20% (0 panneaux allumés)
    int level = 0;
    if (volts > 4.1f) level = 3;
    else if (volts >= 3.7f) level = 2;
    else if (volts >= 3.5f) level = 1;
    else level = 0;

    // Opacité: actif = 255 et inactif = 40 (comme ça on le voit quand même un peu)
    const int OPA_ON = 255;
    const int OPA_OFF = 40;

    if (ui_EnergyPanel1) lv_obj_set_style_bg_opa(ui_EnergyPanel1, (level >= 1) ? OPA_ON : OPA_OFF, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (ui_EnergyPanel2) lv_obj_set_style_bg_opa(ui_EnergyPanel2, (level >= 2) ? OPA_ON : OPA_OFF, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (ui_EnergyPanel3) lv_obj_set_style_bg_opa(ui_EnergyPanel3, (level >= 3) ? OPA_ON : OPA_OFF, LV_PART_MAIN | LV_STATE_DEFAULT);
  };

  // Mise à jour du label de l'heure
  updateTime();
  
  // Mise à jour du label de la batterie (a revoir l'unité ou autre)
  if (ui_BatteryLabel) {
    unsigned long now = millis();
    // Actualiser la batterie chaque minute ou la première fois
    if (now - lastBatteryUpdateTime >= 60000UL || lastBatteryUpdateTime == 0) {
      //float volts = BAT_Get_Volts();
      float volts = 3.62; //pour tester en attendant d'avoir une batterie
      int pct = computeBatteryPercent(volts);
      lv_label_set_text(ui_BatteryLabel, (String(pct) + "%").c_str());
      updateBatteryPanels(volts);
      lastBatteryUpdateTime = now;
    }
  }
  // Mise à jour du durationSlider si la musique est en cours de lecture
  if (ui_DurationSlider && audio.isRunning()) {
    uint32_t musicDuration = audio.getAudioFileDuration();
    uint32_t musicElapsed = audio.getAudioCurrentTime();
    if (musicDuration > 0) {
      int sliderValue = (musicElapsed * 100) / musicDuration;
      lv_slider_set_value(ui_DurationSlider, sliderValue, LV_ANIM_OFF);
    }
  }
}

void loop() {
  Lvgl_Loop();
  networkLoop();
  PCF85063_Loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= 1000) {
    lastUpdateTime = currentMillis;
    updateDisplayInfo();
  }

  vTaskDelay(pdMS_TO_TICKS(5));
}

// ---- Son ----

extern "C" void playMusic() {
  if (audio.isRunning()) {
    printf("Le son en cours, pause.\n");
    Music_pause();
  } else {
    printf("Lecture du son.\n");
    Volume_adjustment(21);
    Play_Music("/", "berceuse-jules.mp3");
  }
}

extern "C" void setVolume(int volume) {
  if (volume < 0) { volume = 0; }
  if (volume > 21) { volume = 21; }
  Volume_adjustment(volume);
}