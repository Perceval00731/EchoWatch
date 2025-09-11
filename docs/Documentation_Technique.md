# Documentation Technique

## 1. Présentation du projet
EchoWatch est une montre connectée basée sur une carte ESP32-S3-Touch-LCD-1.46B, intégrant un écran tactile circulaire, des capteurs (gyroscope, micro, batterie, RTC), et des fonctionnalités audio, réseau et stockage.

## 2. Architecture matérielle
- **Carte principale** : ESP32-S3-Touch-LCD-1.46B
- **Écran** : SPD2010, tactile capacitif
- **Audio** : DAC PCM5101, micro MSM
- **Capteurs** : Gyroscope QMI8658, RTC PCF85063, gestion batterie
- **Stockage** : microSD
- **Connectivité** : WiFi, MQTT (PubSubClient)

## 3. Organisation des fichiers
- **LVGL_Arduino/** : Code principal, drivers matériels, gestion UI (LVGL)
  - LVGL_Arduino.ino : point d’entrée Arduino
  - Drivers : Audio_PCM5101, Gyro_QMI8658, BAT_Driver, etc.
  - UI : ui.c, ui_Screen1.c, assets/
- **libraries/** : Bibliothèques tierces
  - ESP32-audioI2S-master : gestion audio
  - lvgl : bibliothèque graphique LVGL
  - PubSubClient : client MQTT
- **docs/** : Documentation technique et chiffrage

## 4. Fonctionnement logiciel
- **Initialisation** : Configuration des drivers, de l’écran, des capteurs et de la connectivité.
- **Interface graphique** : LVGL gère l’affichage et les interactions tactiles.
- **Audio** : Lecture et enregistrement via I2S et DAC.
- **Capteurs** : Lecture périodique des données (gyroscope, batterie, RTC).
- **Stockage** : Accès à la microSD pour les fichiers audio et logs.
- **Communication** : Envoi/réception de données via MQTT.

## 5. Compilation et déploiement
- Utiliser Arduino IDE avec la carte ESP32-S3-Touch-LCD-1.46B.
- Ouvrir LVGL_Arduino.ino et vérifier les paramètres (voir README.md).
- Compiler et téléverser sur la carte via USB-C.
 >_En tenant compte que vous ayez suivi au préalable toutes les étapes d'installation des bibliothèques et configurations nécessaires du guide d'installation._

## 6. Personnalisation UI
- Utiliser SquareLine Studio pour créer/modifier l’interface.
- Exporter les fichiers UI et les placer dans LVGL_Arduino.

## 7. Dépendances
- LVGL v8.3.10
- ESP32-audioI2S-master v2.0.0
- PubSubClient v2.8

## 8. Bonnes pratiques
- Vérifier les connexions matérielles avant upload.
- Utiliser des versions compatibles des bibliothèques.

## 9. Ressources
- README.md : guide d’installation et configuration
- docs/Chiffrage_Projet.md : estimation des coûts
- docs/Documentation_Technique.md : informations techniques
- docs/Guide_Installation.md : guide d'installation

## 10. Évolutivité
Le projet est conçu pour être extensible (ajout de capteurs, nouvelles interfaces, etc.) :
- En ce qui concerne la localisation d'objets, cette montre dispose de capteurs permettant de déterminer son orientation ("Angular deflection" selon la documentation officielle).
- Il est également possible d'intégrer une IHM auditive grâce à la reconnaissance vocale ("speech recognition") pour effectuer des commandes vocales.
- Pour plus d’informations, consulter la documentation officielle : https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.46B