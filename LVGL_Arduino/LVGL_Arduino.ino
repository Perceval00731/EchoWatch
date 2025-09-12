/* Using LVGL with Arduino requires some extra steps:
 * Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  
 */

#include "Display_SPD2010.h"  // Affichage
#include "Touch_SPD2010.h"    // Tactile
#include "LVGL_Driver.h"      // LVGL
#include "Audio_PCM5101.h"    // Son
#include "MIC_MSM.h"          // Micro
#include <WiFi.h>             // Wi-Fi
#include "ui.h"               // UI LVGL
#include <PubSubClient.h>     // MQTT
#include "SD_Card.h"          // Carte SD
#include "BAT_Driver.h"       // Batterie


// ---- Paramétrage WiFi ----

const char* ssid_local = "L'espoir fait vivre";
const char* password_local = "ekip31470";

// ---- Paramétrage MQTT ----

const char* mqtt_server = "test.mosquitto.org";
WiFiClient espClient;
PubSubClient client(espClient);


// ---- Connection WiFi ----

void connectToWiFi() {
  Serial.println("[WiFi] Initialisation...");
  WiFi.mode(WIFI_STA);
  delay(200);
  WiFi.begin(ssid_local, password_local);
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 10000;
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connecté ! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("[WiFi] Échec de connexion !");
  }
}

// ---- Callback MQTT ----
// Modifier ce comportement afin de séparer les actions
// Exemple: Changer la couleur de fond de l'écran en fonction du message reçu (si le message est une couleur hexadécimale)
// Peut être prévoir plusieurs topics pour différentes actions (Lumière, Son, "localisation", etc.)

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  Serial.print("[MQTT] Message reçu sur le topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  char colorStr[7] = {0};
  if (length >= 6) {
    for (unsigned int i = 0; i < 6 && i < length; i++) {
      colorStr[i] = (char)payload[i];
    }
    // Conversion et application de la couleur
    uint32_t color = (uint32_t)strtol(colorStr, NULL, 16);
    if (ui_FindAppScreen) {
      lv_obj_set_style_bg_color(ui_FindAppScreen, lv_color_hex(color), LV_PART_MAIN);
    }
  } else {
    for (unsigned int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  }
}


// ---- Boucle principale ----


// ---- Initialisation ----

void Init() {
  I2C_Init();
  Backlight_Init();
  Set_Backlight(50);
  LCD_Init();
  Lvgl_Init();
  Touch_Init();
  SD_Init();
  Audio_Init();
  MIC_Init();
  ui_init();
}

// ---- Connection MQTT ----

void connectToMQTT() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  Serial.println("[MQTT] Connexion au serveur...");
  while (!client.connected()) {
    Serial.print("[MQTT] Tentative...");
    if (client.connect("EchoWatchClient")) {
      Serial.println("[MQTT] Connecté !");
      client.subscribe("esp32/color");
    } else {
      Serial.print("[MQTT] Échec, rc=");
      Serial.print(client.state());
      Serial.println(". Nouvelle tentative dans 2s...");
      delay(2000);
    }
  }
}

// ---- Setup ----

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("===== Démarrage =====");
  Init();
  connectToWiFi();
  connectToMQTT();
  Serial.println("===== Setup terminé =====");
}

// ---- Loop ----

void loop() {
  Lvgl_Loop();
  client.loop();
  vTaskDelay(pdMS_TO_TICKS(5));
}