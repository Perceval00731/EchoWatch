# Documentation Technique

Ce document décrit l'architecture matérielle et logicielle du projet EchoWatch, les composants clés, les points d'intégration (MQTT, audio), ainsi que des instructions pratiques pour la compilation, le déploiement et le debug.

## 1. Présentation
EchoWatch est une montre connectée basée sur une carte ESP32-S3 (module "Touch-LCD-1.46B"), avec écran tactile, gestion audio (playback, TTS), capteurs (gyroscope, RTC, batterie), stockage microSD et connectivité réseau (Wi‑Fi + MQTT).

## 2. Architecture matérielle (résumé)
- Carte : ESP32-S3-Touch-LCD-1.46B (voir répertoire `LVGL_Arduino` pour le code spécifique)
- Écran/Tactile : SPD2010 (driver inclus dans le projet)
- Audio : DAC PCM5101 (driver `Audio_PCM5101.*`), gestion I2S
- Capteurs : QMI8658 (gyro/accel), PCF85063 (RTC)
- Stockage : microSD (SD_MMC)

## 3. Organisation du code
- `LVGL_Arduino/` : application principale Arduino (entry `LVGL_Arduino.ino`), drivers matériels et UI
  - `LVGL_Arduino.ino` : initialisation (AppInit, NetworkManager_begin, GyroPublisher_init, etc.) et boucle principale qui appelle `Lvgl_Loop()`, `NetworkManager_loop()`, `LampControl_loop()`, `GyroPublisher_loop()`
  - Drivers audio : `Audio_PCM5101.*` (initialisation I2S, volume, lecture depuis SD), `AudioControl.{h,cpp}` (API exposée pour le reste de l'application)
  - Network : `NetworkManager.cpp` (gestion WiFi, MQTT, NTP, topics, logique de lecture audio à distance)
  - UI : fichiers générés/maintenus par SquareLine (dans `SquareLine/` et `SquareLineProject/`)
- `libraries/` : bibliothèques tierces incluses dans le dépôt pour compilation hors ligne
- `docs/` : documentation et guides

## 4. Comportement logiciel détaillé

Entrée / boucle principale
- `LVGL_Arduino.ino` appelle au startup : `AppInit()` puis `NetworkManager_begin()`. La `loop()` appelle périodiquement `Lvgl_Loop()` (LVGL) et `NetworkManager_loop()`.

Network & MQTT (`NetworkManager.cpp`)
- Gestion WiFi : tentative périodique via `startWiFiAttempt()` si non connecté.
- MQTT : utilise `PubSubClient` (instance `g_mqttClient`) ; connexion via `attemptMQTTOnce()` puis `g_mqttClient.loop()` une fois connecté.
- Configuration TLS : connexion sécurisée sur port 8883 avec `WiFiClientSecure` (vérification certificat désactivée via `setInsecure()` pour brokers publics)
- Topics abonnés (exemples présents dans le code) :
  - `esp32/color` : payload = 6 hex chars (RRGGBB) — met à jour la couleur de l'écran FindApp
  - `esp32/sound` : raccourci play/pause — appelle `playMusic()`
  - `esp32/tts` : JSON { "text": "...", "lang": "fr" } — lance `playTextToSpeech(text, lang)` ; publie réponse sur `esp32/tts/response`
  - `esp32/lampe/ack` : ack de la lampe — gestion par `LampControl_handleAck`
  - `esp32/sensors/request` : JSON de configuration → `GyroPublisher_handleRequest`
  - `esp32/http` : (présent dans le code, handler non détaillé ici)
  - `esp32/audio/request` : topic pour requêtes de lecture HTTP (voir ci‑dessous)

Audio à distance (MQTT)
- Topic de requête : `esp32/audio/request` — attend un JSON contenant au moins la clé `url`.
- Exemple de payload (requête) : { "url": "https://example.com/stream.mp3" }
- Comportement à la réception : `handleAudioRequest()` parse le JSON, stoppe une lecture précédente si nécessaire, appelle `playHTTPStream(url)` (via l'API `AudioControl`) et publie immédiatement une réponse `playing`.

- Topic de réponse : `esp32/audio/response` — format JSON envoyé par `publishAudioResponse(status, url, message)`.
  - Champs possibles : `status` ("playing", "completed", "error", "stopped"), `url`, `message` (texte descriptif)

Gestion de l'état de lecture (`handleAudioPlaybackState()`)
- La boucle appelle périodiquement `handleAudioPlaybackState()` qui :
  - vérifie si `AudioControl_consumeStreamFinished()` a renvoyé un EOF pour associer la fin du flux et publier `completed` si l'EOF correspond au flux courant (comparaison normalisée des URLs) ;
  - surveille `AudioControl_isRunning()` pour détecter démarrage effectif (`g_audioFirstRunningAt`) puis arrêt prématuré ;
  - calcule `elapsed` et `duration` via `AudioControl_getElapsed()`/`getDuration()` (unités en secondes) et `playedMs` (millis depuis premier running) pour décider si l'arrêt est un succès (`completed`) ou une erreur (`error`) selon heuristiques configurées (durée minimale, court fichier connu, etc.) ;
  - si lecture ne démarre pas dans un timeout (2s), publie `error` "lecture non demarree (timeout)".

API audio exposée (interface)
- `playMusic()` — play/pause du player local
- `setVolume(int)`
- `playTextToSpeech(const char* text, const char* lang)` — TTS via la pile audio
- `playHTTPStream(const char* url)` — démarre un flux HTTP (retourne bool success)
- `AudioControl_stop()` — stop
- `AudioControl_isRunning()` — indique si la lecture est active
- `AudioControl_consumeStreamFinished(char* infoBuffer, size_t bufferLen)` — consomme un événement EOF si présent et retourne une info (souvent l'URL ou le chemin)
- `AudioControl_getElapsed()` / `AudioControl_getDuration()` — exposent elapsed/duration en secondes

Notes :
- `consumeStreamFinished` est utilisé pour assurer que l'EOF rapporté correspond bien à l'URL courante (normalisation et comparaison par queue pour traiter cas où l'info ne contient que le chemin ou le nom de fichier).

## 5. Compilation et déploiement
- Ouvrir le projet dans l'Arduino IDE (ou PlatformIO en configurant les bonnes cibles ESP32-S3).
- Fichiers principaux : `LVGL_Arduino/LVGL_Arduino.ino` (point d'entrée) et le dossier `LVGL_Arduino/`.
- Bibliothèques incluses dans `libraries/` : LVGL, ESP32-audioI2S-master, PubSubClient, etc. Installer via le gestionnaire de bibliothèques de l'IDE si nécessaire.
- PCB/board : sélectionner la carte ESP32-S3 correspondante et configurer la vitesse de flash et partition scheme si besoin.
- Téléversement : connecteur USB-C fourni par la carte.

## 6. Dépendances (observées dans le dépôt)
- LVGL (version approximative utilisée : v8.x)
- ESP32-audioI2S-master (gestion I2S/Audio)
- PubSubClient (client MQTT)

Remarque : les versions exactes peuvent se trouver dans `libraries/` (fichiers `library.json` ou `README.md`).

## 7. Exemples de payloads MQTT

Cette section détaille les formats de payloads pour chaque fonctionnalité MQTT disponible.

### 7.1. Changement de couleur (Localisation)
**Topic:** `esp32/color`  
**Format:** Couleur hexadécimale (6 caractères, sans #)  
**Type:** String brut (non-JSON)

**Exemples:**
```
FF0000    → Rouge
00FF00    → Vert  
0000FF    → Bleu
FFFF00    → Jaune
FF00FF    → Magenta
00FFFF    → Cyan
FFFFFF    → Blanc
```

**Commande test (mosquitto_pub):**
```bash
mosquitto_pub -h broker.emqx.io -p 8883 --capath /etc/ssl/certs/ -t "esp32/color" -m "FF0000"
```

---

### 7.2. Contrôle audio (Play/Pause)
**Topic:** `esp32/sound`  
**Format:** Aucun payload spécifique requis  
**Type:** N'importe quelle valeur

**Exemples:**
```
play
pause
(ou vide)
```

**Commande test:**
```bash
mosquitto_pub -h broker.emqx.io -p 8883 --capath /etc/ssl/certs/ -t "esp32/sound" -m "play"
```

---

### 7.3. Synthèse vocale (TTS)
**Topic:** `esp32/tts`  
**Format:** JSON  
**Type:** Object avec champs `text` (requis) et `lang` (optionnel, défaut: "fr")

**Exemples:**
```json
{
  "text": "Bonjour, comment allez-vous ?",
  "lang": "fr"
}
```
```json
{
  "text": "Il est l'heure de prendre vos médicaments",
  "lang": "fr"
}
```
```json
{
  "text": "Hello, how are you?",
  "lang": "en"
}
```
```json
{
  "text": "Attention, batterie faible"
}
```

**Topic de réponse:** `esp32/tts/response`  
**Réponses possibles:**
- `"Synthèse vocale démarrée"` (succès)
- `"Erreur de synthèse vocale"` (échec)
- `"Texte vide pour la synthèse vocale"` (erreur)
- `"Erreur de parsing JSON"` (format invalide)

**Commande test:**
```bash
mosquitto_pub -h broker.emqx.io -p 8883 --capath /etc/ssl/certs/ -t "esp32/tts" -m '{"text":"Bonjour tout le monde","lang":"fr"}'
```

---

### 7.4. Lecture audio HTTP (Streaming)
**Topic:** `esp32/audio/request`  
**Format:** JSON  
**Type:** Object avec champ `url` (requis)

**Exemples:**
```json
{
  "url": "http://icecast.radiofrance.fr/franceinter-midfi.mp3"
}
```
```json
{
  "url": "http://direct.fipradio.fr/live/fip-midfi.mp3"
}
```
```json
{
  "url": "https://example.com/audio/sample.mp3"
}
```
```json
{
  "url": "http://streaming.radio.co/s123456/listen"
}
```

**Topic de réponse:** `esp32/audio/response`  
**Format de réponse:** JSON avec champs `status`, `url`, `message`

**Réponses possibles:**
```json
{
  "status": "playing",
  "url": "http://icecast.radiofrance.fr/franceinter-midfi.mp3",
  "message": "lecture en cours"
}
```
```json
{
  "status": "completed",
  "url": "http://example.com/audio.mp3",
  "message": "lecture terminee"
}
```
```json
{
  "status": "error",
  "url": "http://invalid-url.com/audio.mp3",
  "message": "impossible de demarrer la lecture"
}
```
```json
{
  "status": "error",
  "url": "http://example.com/audio.mp3",
  "message": "lecture non demarree (timeout)"
}
```
```json
{
  "status": "error",
  "url": "http://example.com/audio.mp3",
  "message": "lecture arretee trop tot (<1s)"
}
```
```json
{
  "status": "stopped",
  "url": "http://previous-stream.mp3",
  "message": "lecture interrompue par une nouvelle requete"
}
```
```json
{
  "status": "error",
  "url": "",
  "message": "URL manquante"
}
```

**Commande test:**
```bash
mosquitto_pub -h broker.emqx.io -p 8883 --capath /etc/ssl/certs/ -t "esp32/audio/request" -m '{"url":"http://icecast.radiofrance.fr/franceinter-midfi.mp3"}'
```

---

### 7.5. Contrôle de la lampe
**Topic commande:** `esp32/lampe`  
**Topic ACK:** `esp32/lampe/ack`  
**Format:** String brut (non-JSON)  
**Type:** Valeurs textuelles

**Exemples de commandes (vers `esp32/lampe`):**
```
ON
OFF
1
0
TRUE
FALSE
```

**Valeurs interprétées comme "ON":**
- `ON`, `1`, `TRUE` (insensible à la casse)

**Valeurs interprétées comme "OFF":**
- Toutes les autres valeurs

**Exemples d'ACK (depuis `esp32/lampe/ack`):**
```
ON     → Lampe allumée
OFF    → Lampe éteinte
```

**Commande test:**
```bash
# Allumer la lampe
mosquitto_pub -h broker.emqx.io -p 8883 --capath /etc/ssl/certs/ -t "esp32/lampe" -m "ON"

# Éteindre la lampe
mosquitto_pub -h broker.emqx.io -p 8883 --capath /etc/ssl/certs/ -t "esp32/lampe" -m "OFF"

# Écouter les ACK
mosquitto_sub -h broker.emqx.io -p 8883 --capath /etc/ssl/certs/ -t "esp32/lampe/ack" -v
```

---

### 7.6. Configuration des capteurs (Gyroscope)
**Topic:** `esp32/sensors/request`  
**Format:** JSON  
**Type:** Object avec champs configurables

**Exemples:**
```json
{
  "enabled": true,
  "periodicity_ms": 1000,
  "timeout_ms": 5000
}
```
```json
{
  "enabled": false
}
```
```json
{
  "periodicity_ms": 500
}
```

**Champs disponibles:**
- `enabled` (boolean) : Activer/désactiver la publication des données
- `periodicity_ms` (integer) : Période d'échantillonnage en millisecondes
- `timeout_ms` (integer) : Timeout en millisecondes

**Commande test:**
```bash
mosquitto_pub -h broker.emqx.io -p 8883 --capath /etc/ssl/certs/ -t "esp32/sensors/request" -m '{"enabled":true,"periodicity_ms":1000}'
```

---

### 7.7. Script Python de test complet

Le fichier `simulation_mqtt/main.py` contient un client interactif permettant de tester toutes les fonctionnalités MQTT.

**Utilisation:**
```bash
cd simulation_mqtt/
pip install -r requirements.txt
python main.py
```

**Fonctionnalités du script:**
1. Changement de couleur
2. Lecture/pause audio
3. Synthèse vocale TTS
4. Lecture données gyroscope
5. Contrôle lampe (ON/OFF/TOGGLE/ETAT)
6. Lecture audio HTTP
7. Quitter

---

## 8. Debug & observabilité
- Logs : utilisation intensive de `printf` / `Serial.printf` pour diagnostiquer (NetworkManager, AudioControl, etc.).
- MQTT : surveiller `esp32/audio/response` et `esp32/tts/response` pour connaître l'état côté device.
- Pour diagnostiquer EOF non associés : `NetworkManager` normalise et loggue `infoBuf` et sa version normalisée (`normInfo`) quand un EOF est ignoré.

## 8. Bonnes pratiques et recommandations
- Normalisation d'URL : la logique actuelle supprime seulement `http://`/`https://` et compare la queue ; pour plus de robustesse, envisager :
  - supprimer la query string (`?…`) avant comparaison ;
  - décoder percent-encoding si nécessaire ;
  - comparer les derniers segments de chemin (2–3 segments) pour réduire les faux-positifs.
- Paramètres/constantes (timeouts, seuils) : centraliser dans des constantes configurables (déjà partiellement fait dans `NetworkManager.cpp`).
- Ajouter un wrapper de log (levels) plutôt que `printf` direct pour filtrer facilement INFO/DEBUG/ERROR.
- Si on veut tester la logique de décision audio hors matos : stuber `AudioControl_*` et écrire tests unitaires pour `handleAudioPlaybackState()`.
- **Vérification TLS:** Les logs au démarrage affichent le port utilisé (8883 pour TLS). Utiliser Wireshark pour confirmer le handshake TLS si nécessaire.

## 10. Ressources & fichiers clés
- `LVGL_Arduino/LVGL_Arduino.ino` — point d'entrée, boucle et initialisation
- `LVGL_Arduino/NetworkManager.cpp` — gestion WiFi, MQTT, NTP, handlers MQTT, audio remote control
- `LVGL_Arduino/AudioControl.h` — interface d'abstraction audio
- `LVGL_Arduino/Audio_PCM5101.*` — implémentation I2S/DAC
- `libraries/` — bibliothèques embarquées (LVGL, audio, PubSubClient…)
- `simulation_mqtt/main.py` — script Python de test MQTT interactif

## 11. Évolutivité
- Le projet est conçu pour être extensible : ajouter capteurs, services réseau, ou fonctions audio additionnelles (ex. streaming multipiste, queue de lecture). L'architecture modulaire (NetworkManager, AudioControl, LampControl, GyroPublisher) facilite l'extension.