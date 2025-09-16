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


// ---- Connection WiFi ----

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  delay(200);
  WiFi.begin(ssid_local, password_local);
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 10000;
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
  } else {
    wifiConnected = false;
  }
}


// ---- Connection MQTT ----

void connectToMQTT() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback); // Définir la fonction de rappel (des qu'un message est reçu alors on appelle cette fonction)
  while (!client.connected()) {
    // Tentative
    if (client.connect("EchoWatchClient")) {
      // Connecté
      client.subscribe("esp32/color");
      client.subscribe("esp32/sound");
    } else {
      // Échec et nouvelle tentative dans 2 secondes
      delay(2000);
    }
  }
}


// ---- Callback MQTT ----

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  if (topic == "color") { // Message sur esp32/color
    mqttMessageColor(payload, length);
  } 
  else if (topic == "sound") { // Message sur esp32/sound
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
  if (getLocalTime(&timeinfo)) {
    if (ui_Hour) {
      char hourStr[6];
      strftime(hourStr, sizeof(hourStr), "%H:%M", &timeinfo);
      lv_label_set_text(ui_Hour, hourStr);
    }
  } else {
    Serial.println("Erreur NTP");
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
  connectToWiFi();
  connectToMQTT();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Début Musique");
  Serial.println("===== Setup terminé =====");
  Serial.println("Get Battery");
  Serial.println(BAT_Get_Volts());
}

// ---- Loop ----

unsigned long lastUpdateTime = 0;

// Fonction pour mettre à jour les informations affichées (toutes les secondes pour éviter de surcharger le CPU en recalculant à chaque loop)
void updateDisplayInfo() {
  // Mise à jour du label de l'heure
  updateTime();
  
  // Mise à jour du label de la batterie (a revoir l'unité ou autre)
  if (ui_BatteryLabel) {
    float volts = BAT_Get_Volts();
    lv_label_set_text(ui_BatteryLabel, (String((int)(volts * 20)) + "%").c_str());
  }

  // Mise à jour du durationSlider si la musique est en cours de lecture
  // NOTE A MOI MEME : FAUT LE DISABLE ET IL NE SEMBLE PAS S'INCREMENTER
  // AJOUTER UNE MAJ SUR LES LABELS DE TEMPS
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
  client.loop();
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