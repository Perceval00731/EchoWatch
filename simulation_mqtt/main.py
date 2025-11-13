import json

import paho.mqtt.client as mqtt
import time

BROKER = "broker.emqx.io"
PORT = 1883
# Topics
TOPIC_COLOR = "esp32/color"
TOPIC_SOUND = "esp32/sound"
TOPIC_TTS = "esp32/tts"
TOPIC_GYRO = "esp32/gyro"
TOPIC_LAMP_CMD = "esp32/lampe"
TOPIC_LAMP_ACK = "esp32/lampe/ack"
TOPIC_AUDIO_REQUEST = "esp32/audio/request"
TOPIC_AUDIO_RESPONSE = "esp32/audio/response"

TOPICS = [
    TOPIC_COLOR,
    TOPIC_SOUND,
    TOPIC_TTS,
    TOPIC_GYRO,
    TOPIC_LAMP_CMD,
    TOPIC_LAMP_ACK,
    TOPIC_AUDIO_REQUEST,
    TOPIC_AUDIO_RESPONSE,
]

# États locaux
global readGyro
readGyro = False
lamp_state = None  # "ON" / "OFF" ou None si inconnu


def publish_lamp_ack(client, state: str):
    """Publie un ACK retained pour la lampe et met à jour l'état local."""
    global lamp_state
    state_up = state.strip().upper()
    if state_up not in ("ON", "OFF"):
        return
    lamp_state = state_up
    client.publish(TOPIC_LAMP_ACK, state_up, qos=0, retain=True)
    print(f"[LAMPE][ACK][local] État actuel: {lamp_state}")


def norm(payload: bytes) -> str:
    return payload.decode("utf-8", errors="ignore").strip().upper()

def on_connect(client, userdata, flags, rc):
    print(f"Connecté : {rc}")
    for topic in TOPICS:
        client.subscribe(topic)
        print(f"Abonné à {topic}")

def on_message(client, userdata, msg):
    global lamp_state
    if msg.topic == TOPIC_GYRO and readGyro:
        print(f"Données du gyroscope reçues : {msg.payload.decode()}")
    elif msg.topic == TOPIC_LAMP_CMD:
        cmd = norm(msg.payload)
        print(f"[LAMPE][CMD] topic={msg.topic} payload={cmd}")
        if cmd in ("ON", "1", "TRUE"):
            publish_lamp_ack(client, "ON")
        elif cmd in ("OFF", "0", "FALSE"):
            publish_lamp_ack(client, "OFF")
        else:
            print("[LAMPE] Commande inconnue, ignorée")
    elif msg.topic == TOPIC_LAMP_ACK:
        payload = msg.payload.decode("utf-8", errors="ignore").strip().upper()
        if payload in ("ON", "OFF"):
            lamp_state = payload
            print(f"[LAMPE][ACK] État actuel: {lamp_state}")
        else:
            print(f"[LAMPE][ACK] Payload inconnu: {payload}")
    elif msg.topic == TOPIC_AUDIO_RESPONSE:
        print(f"[AUDIO][RESPONSE] {msg.payload.decode('utf-8', errors='ignore')}")
    
def choixAction():
    etat_lampe = lamp_state if lamp_state is not None else "inconnu"
    print("Actions disponibles :")
    print("1. Changer la couleur (topic: esp32/color, payload: couleur)")
    print("2. Jouer un son (topic: esp32/sound, payload: nom_du_son)")
    print("3. Synthèse vocale (topic: esp32/tts, payload: {texte: \"...\", lang: \"fr\"})")
    print("4. Lire les données du gyroscope (topic: esp32/gyro, payload: 'read')")
    print(f"5. Lampe (etat={etat_lampe}) -> ON/OFF/TOGGLE/ETAT")
    print("6. Lecture audio HTTP (topic: esp32/audio/request, payload: {\"url\": \"...\"})")
    print("7. Quitter")
    action = str(input("Sélectionner une action : ")).strip()
    return action

def effectuerAction(action):
    global readGyro, lamp_state
    if action == "1":
        couleur = str(input("Entrez la couleur hexadécimale (ex: FF0000, 00FF00, 0000FF) : ")).strip()
        client.publish(TOPIC_COLOR, couleur)
    elif action == "2":
        print("Déclenchement d'un son.")
        client.publish(TOPIC_SOUND, "TRIGGER")
    elif action == "3":
        texte = str(input("Entrez le texte à synthétiser : ")).replace('"', '\\"')
        lang = str(input("Entrez la langue (ex: 'fr', 'en') : ")).strip()
        client.publish(TOPIC_TTS, f'{{"text": "{texte}", "lang": "{lang}"}}')
    elif action == "4":
        readGyro = not readGyro
        if readGyro:
            print("Lecture des données du gyroscope activée.")
            client.publish(TOPIC_GYRO, "read")
        else:
            print("Lecture des données du gyroscope désactivée.")
    elif action == "5":
        cmd = str(input("Commande lampe [ON/OFF/TOGGLE/ETAT] : ")).strip().upper()
        if cmd == "ETAT":
            print(f"État lampe connu: {lamp_state if lamp_state is not None else 'inconnu'}")
        elif cmd == "TOGGLE":
            if lamp_state is None:
                print("État inconnu, bascule vers ON par défaut.")
                client.publish(TOPIC_LAMP_CMD, "ON")
            else:
                next_state = "OFF" if lamp_state == "ON" else "ON"
                client.publish(TOPIC_LAMP_CMD, next_state)
        elif cmd in ("ON", "OFF", "1", "0", "TRUE", "FALSE"):
            normalized = "ON" if cmd in ("ON", "1", "TRUE") else "OFF"
            client.publish(TOPIC_LAMP_CMD, normalized)
        else:
            print("Commande lampe non reconnue.")
    elif action == "6":
        url = str(input("Entrez l'URL du fichier audio (MP3/WAV) : ")).strip()
        if url:
            if not url.startswith("http://") and not url.startswith("https://"):
                url = "https://" + url
            payload = json.dumps({"url": url})
            client.publish(TOPIC_AUDIO_REQUEST, payload)
            print(f"[DEBUG] URL envoyée: {url}")
        else:
            print("URL vide, requête annulée.")
    elif action == "7":
        print("Quitter le programme.")
        return False
    else:
        print("Action non reconnue.")
    return True

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)
client.loop_start()

while True:
    action = choixAction()
    if not effectuerAction(action):
        break

client.loop_stop()
client.disconnect()