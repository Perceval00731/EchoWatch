# EchoWatch – ESP32-S3-Touch-LCD-1.46B

## Client
- Adrien van den Bossche, pour LODICI, projet de localisation dynamique et intelligente.

## Composition de l'équipe
| Membre | Rôle |
| :---------------: |:---------------:|
| Romy Chauviere | Scrum Master |
| Thomas Fusi Belaid | Product Owner |
| Melvin Bouyssou | Développeur |
| Nolhan Biblocque | Développeur | 

---

## Présentation du projet
EchoWatch est une montre connectée développée sur la carte **ESP32-S3-Touch-LCD-1.46B** (Waveshare).  Elle vise à être utilisée par des personnes malvoyantes et est accompagnée par des installations connectées dans leur lieu de vie.
L’objectif est de proposer une IHM interactive et des fonctionnalités connectées autour de MQTT, de l’audio et du Text-to-Speech, afin de rendre la montre autonome et polyvalente.  

Fonctionnalités principales :  
- Affichage de l’heure en temps réel  
- Changement de couleur via un ordre MQTT  
- Lecture d’audio (HTTP streaming, pause, play, volume, durée)  
- Lecture de texte via TTS (Text-to-Speech) reçu en MQTT  
- Déclenchement d’objets connectés avec accusé de réception  
- Visualisation du niveau de batterie (estimation par tension)  
- Connexion réseau non bloquante  

---

## Réunion avec le client

| Date | Lien vers l'ODJ | Lien vers le CR |
| :------ | :---: | :---: |
| 05/09/2025 | [ODJ](https://github.com/Perceval00731/EchoWatch/blob/main/docs/ODJ/ODJ%20%231.pdf) | [CR](https://github.com/Perceval00731/EchoWatch/blob/main/docs/CR/CR%20%231.pdf) |
| 12/09/2025 | [ODJ](https://github.com/Perceval00731/EchoWatch/blob/main/docs/ODJ/ODJ%20%232.pdf) | [CR](https://github.com/Perceval00731/EchoWatch/blob/main/docs/CR/CR%20%232.pdf) |
| 18/09/2025 | [ODJ](https://github.com/Perceval00731/EchoWatch/blob/main/docs/ODJ/ODJ%20%233.pdf) | [CR](https://github.com/Perceval00731/EchoWatch/blob/main/docs/CR/CR%20%233.pdf) |
| 06/11/2025 | [ODJ](https://github.com/Perceval00731/EchoWatch/blob/main/docs/ODJ/ODJ%20%234.pdf) | [CR](https://github.com/Perceval00731/EchoWatch/blob/main/docs/CR/CR%20%234.pdf) |

---

## Récap du sprint #1

[Slide](https://www.canva.com/design/DAGzCY39txE/jdT9vtsBRvO_zA0K2MZXgw/edit)

---

## Guide d’installation

### Prérequis
- Une carte **ESP32-S3-Touch-LCD-1.46B**  
- Un câble USB-C compatible données (et non uniquement charge)  
- (Optionnel) Une carte microSD  

---

### Configuration dans SquareLine Studio
1. Ouvrir **SquareLine Studio**  
2. Créer un nouveau projet (ou importer celui fourni) :  
   - Type : Arduino  
   - Template : Arduino with TFT_eSPI  
   - Paramètres :  
     - Résolution : `412 × 412`  
     - Shape : `Circle`  
     - Color depth : `16 bit`  
   - Choisir un chemin (emplacement du projet) → le garder en mémoire.  
   ![Capture d'écran de la création du projet](/ressources/create_project.png)  
3. Aller dans **File → Project Settings** :  
   - Shape : `Circle`  
   - Project Export Root : même chemin que précédemment  
   - UI Files Export Path : même chemin également  
   ![Capture d'écran des paramètres du projet](/ressources/project_settings.png)  
4. Créer le design de l’interface dans SquareLine Studio.  
5. Exporter : **Export → Export UI Files**.  

---

### Préparation des fichiers
1. Copier tous les fichiers exportés dans le dossier `LVGL_Arduino`.  
2. Ouvrir `LVGL_Arduino.ino` avec **Arduino IDE**.  

---

### Configuration dans Arduino IDE
1. Installer la plateforme **ESP32 by Espressif Systems** (version `3.3.0`) dans le Boards Manager.  
2. Sélectionner la carte : `ESP32-S3-Touch-LCD-1.46`.  
3. Installer les bibliothèques nécessaires :  
   - **LVGL** by kisvegabor – version `8.3.10`  
   - **ESP32-audioI2S-master** by schreibfaul1 – version `2.0.0`  
   - **PubSubClient** by Nick O'Leary – version `2.8`  
   > Attention : copier-coller les bibliothèques fournies depuis le dossier `libraries` vers votre dossier `libraries` d’Arduino (généralement `Documents/Arduino/libraries`).  
4. Vérifier les paramètres dans **Tools**.  
   ![Capture d'écran des paramètres de l'outil](/ressources/tools.png)  

---

### Upload
- Lancer l’upload avec la flèche en haut à gauche dans Arduino IDE.  

---

## Liens utiles
- [Documentation officielle – Waveshare ESP32-S3-Touch-LCD-1.46B](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.46B)  
- [SquareLine Studio](https://squareline.io/)  
- [Guide d’installation complet](docs/Guide_Installation.md)  
- [Documentation technique](docs/Documentation_Technique.md)  
- [Chiffrage du projet](docs/Chiffrage_Projet.md)  

---

## Remarques
- La compilation est généralement plus rapide sur **Linux/macOS** que sur **Windows**.  
