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
#include "RTC_PCF85063.h"     // RTC

// Configuration pour les gros fichiers MQTT
#define MQTT_MAX_PACKET_SIZE 150000  // 150KB pour les fichiers WAV
#define WAV_CHUNK_SIZE 8192         // Traiter par chunks de 8KB pour économiser la RAM


// ---- Paramétrage WiFi ----

const char* ssid_local = "iPhone de Melvin";
const char* password_local = "motdepasse2";


// ---- Paramétrage MQTT ----

const char* mqtt_server = "test.mosquitto.org";
WiFiClient espClient;
PubSubClient client(espClient);


// ---- Variables d'état pour gérer l'échec de connexion ----
bool wifiConnected = false;
bool mqttConnected = false;

// ---- Variables pour la réception de fichiers WAV par chunks ----
uint8_t* wavBuffer = nullptr;
size_t wavBufferSize = 0;
size_t expectedTotalSize = 0;
size_t receivedSize = 0;
int expectedChunks = 0;
int receivedChunks = 0;
bool receivingWav = false;


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
  client.setBufferSize(MQTT_MAX_PACKET_SIZE);  // Configurer la taille du buffer pour les gros fichiers
  client.setCallback(mqttCallback); // Définir la fonction de rappel (des qu'un message est reçu alors on appelle cette fonction)
  while (!client.connected()) {
    // Tentative
    if (client.connect("EchoWatchClient")) {
      // Connecté
      client.subscribe("esp32/color");
      client.subscribe("esp32/sound");
      client.subscribe("esp32/sound/meta");
      client.subscribe("esp32/sound/chunk/+");
      client.subscribe("esp32/sound/complete");
      Serial.println("Connecté au broker MQTT et abonné aux topics");
    } else {
      // Échec et nouvelle tentative dans 2 secondes
      Serial.println("Échec de connexion MQTT, nouvelle tentative...");
      delay(2000);
    }
  }
}


// ---- Callback MQTT ----

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  if (strcmp(topic, "esp32/color") == 0) { // Message sur esp32/color
    mqttMessageColor(payload, length);
  } 
  else if (strcmp(topic, "esp32/sound") == 0) { // Message sur esp32/sound (fichier complet)
    mqttMessageSound(payload, length);
  }
  else if (strcmp(topic, "esp32/sound/meta") == 0) { // Métadonnées des chunks
    mqttSoundMeta(payload, length);
  }
  else if (strncmp(topic, "esp32/sound/chunk/", 18) == 0) { // Chunk de fichier WAV
    int chunkIndex = atoi(topic + 18); // Extraire l'index du chunk
    mqttSoundChunk(chunkIndex, payload, length);
  }
  else if (strcmp(topic, "esp32/sound/complete") == 0) { // Signal de fin de réception
    mqttSoundComplete();
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

// ---- Fonction executée lors d'un message sur esp32/sound ----

void mqttMessageSound(uint8_t* payload, unsigned int length) {
  Serial.print("Réception de fichier WAV via MQTT: ");
  Serial.print(length);
  Serial.println(" bytes");
  
  // Vérification de la taille minimale d'un fichier WAV (44 bytes pour l'en-tête)
  if (length < 44) {
    Serial.println("Erreur: Fichier WAV trop petit");
    return;
  }
  
  // Vérifier si c'est bien un fichier WAV (en-tête RIFF)
  if (payload[0] != 'R' || payload[1] != 'I' || payload[2] != 'F' || payload[3] != 'F') {
    Serial.println("Erreur: Ce n'est pas un fichier WAV valide");
    return;
  }
  
  // Vérifier si on a assez de RAM libre
  size_t freeHeap = ESP.getFreeHeap();
  Serial.print("RAM libre: ");
  Serial.print(freeHeap);
  Serial.println(" bytes");
  
  if (length > freeHeap * 0.8) {  // Garder 20% de marge
    Serial.println("ERREUR: Fichier trop volumineux pour la RAM disponible");
    Serial.print("Fichier: ");
    Serial.print(length);
    Serial.print(" bytes, RAM libre: ");
    Serial.print(freeHeap);
    Serial.println(" bytes");
    Serial.println("Solution: Utilisez un fichier WAV plus petit");
    return;
  }
  
  // Allouer la mémoire pour le fichier WAV
  uint8_t* wavBuffer = (uint8_t*)malloc(length);
  if (wavBuffer == nullptr) {
    Serial.println("ERREUR: Échec d'allocation mémoire");
    return;
  }
  
  // Copier les données WAV en mémoire
  memcpy(wavBuffer, payload, length);
  Serial.println("✅ Fichier WAV chargé en RAM");
  
  // Jouer directement depuis la mémoire
  playMqttSoundFromRAM(wavBuffer, length);
}

// ---- Fonction pour traiter les métadonnées des chunks WAV ----

void mqttSoundMeta(uint8_t* payload, unsigned int length) {
  // Convertir le payload JSON en string
  char jsonStr[length + 1];
  memcpy(jsonStr, payload, length);
  jsonStr[length] = '\0';
  
  Serial.print("Métadonnées reçues: ");
  Serial.println(jsonStr);
  
  // Parser les métadonnées (simple parsing manuel pour éviter les dépendances)
  // Format attendu: {"total_size":125534,"num_chunks":3,"chunk_size":50000,"filename":"ring.wav"}
  char* totalSizePtr = strstr(jsonStr, "\"total_size\":");
  char* numChunksPtr = strstr(jsonStr, "\"num_chunks\":");
  
  if (totalSizePtr && numChunksPtr) {
    expectedTotalSize = atol(totalSizePtr + 13); // Skip "total_size":
    expectedChunks = atoi(numChunksPtr + 13);    // Skip "num_chunks":
    
    Serial.print("Préparation pour recevoir ");
    Serial.print(expectedChunks);
    Serial.print(" chunks, taille totale: ");
    Serial.print(expectedTotalSize);
    Serial.println(" bytes");
    
    // Vérifier si on a assez de RAM
    size_t freeHeap = ESP.getFreeHeap();
    if (expectedTotalSize > freeHeap - 10000) { // Garder 10KB de marge
      Serial.println("ERREUR: Pas assez de RAM pour ce fichier");
      return;
    }
    
    // Libérer l'ancien buffer s'il existe
    if (wavBuffer != nullptr) {
      free(wavBuffer);
      wavBuffer = nullptr;
    }
    
    // Allouer le buffer pour le fichier complet
    wavBuffer = (uint8_t*)malloc(expectedTotalSize);
    if (wavBuffer == nullptr) {
      Serial.println("ERREUR: Échec d'allocation mémoire pour les chunks");
      return;
    }
    
    // Initialiser les variables de réception
    receivingWav = true;
    receivedSize = 0;
    receivedChunks = 0;
    wavBufferSize = expectedTotalSize;
    
    Serial.println("✅ Prêt à recevoir les chunks");
  } else {
    Serial.println("ERREUR: Métadonnées mal formées");
  }
}

// ---- Fonction pour traiter un chunk de fichier WAV ----

void mqttSoundChunk(int chunkIndex, uint8_t* payload, unsigned int length) {
  if (!receivingWav || wavBuffer == nullptr) {
    Serial.println("ERREUR: Chunk reçu mais pas en mode réception");
    return;
  }
  
  Serial.print("Chunk ");
  Serial.print(chunkIndex);
  Serial.print(" reçu (");
  Serial.print(length);
  Serial.println(" bytes)");
  
  // Calculer la position dans le buffer
  size_t position = chunkIndex * 50000; // Taille de chunk par défaut
  
  // Vérifier que le chunk rentre dans le buffer
  if (position + length > wavBufferSize) {
    Serial.println("ERREUR: Chunk dépasse la taille du buffer");
    return;
  }
  
  // Copier le chunk dans le buffer
  memcpy(wavBuffer + position, payload, length);
  receivedSize += length;
  receivedChunks++;
  
  Serial.print("Progression: ");
  Serial.print(receivedChunks);
  Serial.print("/");
  Serial.print(expectedChunks);
  Serial.print(" chunks (");
  Serial.print((receivedSize * 100) / expectedTotalSize);
  Serial.println("%)");
}

// ---- Fonction appelée quand tous les chunks sont reçus ----

void mqttSoundComplete() {
  if (!receivingWav || wavBuffer == nullptr) {
    Serial.println("ERREUR: Signal de fin reçu mais pas en mode réception");
    return;
  }
  
  Serial.println("✅ Tous les chunks reçus, assemblage terminé");
  Serial.print("Taille finale: ");
  Serial.print(receivedSize);
  Serial.println(" bytes");
  
  // Vérifier l'intégrité
  if (receivedSize == expectedTotalSize && receivedChunks == expectedChunks) {
    Serial.println("🎵 Lecture du fichier WAV assemblé...");
    playMqttSoundFromRAM(wavBuffer, receivedSize);
  } else {
    Serial.println("ERREUR: Fichier WAV incomplet");
    Serial.print("Attendu: ");
    Serial.print(expectedTotalSize);
    Serial.print(" bytes, reçu: ");
    Serial.println(receivedSize);
  }
  
  // Reset des variables
  receivingWav = false;
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

// ---- Setup ----

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("===== Démarrage =====");
  Init();
  connectToWiFi();
  connectToMQTT();
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
  if (ui_Hour) {
    char hourStr[6];
    snprintf(hourStr, sizeof(hourStr), "%02d:%02d", datetime.hour, datetime.minute);
    lv_label_set_text(ui_Hour, hourStr);
  }

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

// ---- Son ----

void playMqttSoundFromRAM(uint8_t* wavBuffer, size_t bufferSize) {
  // Arrêter la musique en cours si elle joue
  if (audio.isRunning()) {
    audio.pauseResume();
    Serial.println("Arrêt de la musique en cours");
    delay(100);
  }
  
  Serial.println("🎵 Lecture audio depuis la RAM");
  Volume_adjustment(21);  // Volume maximum
  
  // Tenter de lire le fichier WAV depuis le buffer mémoire
  bool ret = audio.connecttobuffer(wavBuffer, bufferSize);
  if (ret) {
    Serial.println("✅ Lecture WAV depuis RAM réussie");
    
    // Attendre que la lecture soit terminée pour libérer la mémoire
    while (audio.isRunning()) {
      audio.loop();
      delay(10);
    }
    
    // Libérer la mémoire une fois la lecture terminée
    free(wavBuffer);
    Serial.println("🗑️ Mémoire WAV libérée");
    
  } else {
    Serial.println("❌ Échec de la lecture WAV depuis RAM");
    // Libérer la mémoire même en cas d'échec
    free(wavBuffer);
  }
}

void playMqttSound() {
  Serial.println("❌ Cette fonction ne doit plus être utilisée - utilisez playMqttSoundFromRAM()");
}

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