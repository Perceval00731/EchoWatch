import json
import paho.mqtt.client as mqtt

BROKER = "test.mosquitto.org"
PORT = 1883
SUB_TOPICS = [
    "esp32/color",
    "esp32/sound",
    "esp32/tts",
    "esp32/tts/response",
    "esp32/lampe",
    "esp32/lampe/ack",
    "esp32/sensors"
]

client = mqtt.Client()
show_sensors = True
invert_ack = False


def on_connect(_client, _userdata, _flags, rc):
    print(f"Connecté : {rc}")
    for topic in SUB_TOPICS:
        _client.subscribe(topic)
        print(f"Abonné à {topic}")


def handle_lamp_command(payload: str):
    global invert_ack
    desired = payload.strip().upper()
    if desired not in ("ON", "OFF"):
        print(f"[Lampe] Commande inconnue: {payload}")
        return

    if invert_ack:
        ack = "OFF" if desired == "ON" else "ON"
    else:
        ack = desired

    invert_ack = not invert_ack
    client.publish("esp32/lampe/ack", ack)
    print(f"[Lampe][Sim] Commande {desired} reçue, ACK envoyé: {ack}")


def on_message(_client, _userdata, msg):
    payload = msg.payload.decode("utf-8", errors="ignore")
    if msg.topic == "esp32/lampe":
        handle_lamp_command(payload)
    elif msg.topic == "esp32/lampe/ack":
        print(f"[Lampe][ACK] {payload}")
    elif msg.topic == "esp32/sensors" and show_sensors:
        print(f"[Capteurs] {payload}")
    elif msg.topic == "esp32/tts/response":
        print(f"[TTS][Réponse] {payload}")
    else:
        print(f"Message reçu sur {msg.topic} : {payload}")


def choix_action():
    print("Actions disponibles :")
    print("1. Changer la couleur (topic: esp32/color)")
    print("2. Jouer un son (topic: esp32/sound)")
    print("3. Synthèse vocale (topic: esp32/tts)")
    print("4. Configurer les capteurs (topic: esp32/sensors/request)")
    print("5. Basculer l'affichage des données capteurs")
    print("6. Quitter")
    return input("Sélectionner une action : ").strip()


def envoyer_couleur():
    couleur = input("Couleur hexadécimale (ex: FF0000) : ").strip()
    client.publish("esp32/color", couleur)


def jouer_son():
    client.publish("esp32/sound", "play")


def envoyer_tts():
    texte = input("Texte à synthétiser : ").strip()
    lang = input("Langue (ex: fr, en) : ").strip() or "fr"
    payload = json.dumps({"text": texte, "lang": lang})
    client.publish("esp32/tts", payload)


def configurer_capteurs():
    try:
        periodicite = input("Période (ms, laisser vide pour conserver) : ").strip()
        timeout = input("Timeout (ms, laisser vide pour conserver) : ").strip()
        enabled = input("Activer la publication ? (o/n, vide = oui) : ").strip().lower()
        payload = {}
        if periodicite:
            payload["periodicity_ms"] = int(periodicite)
        if timeout:
            payload["timeout_ms"] = int(timeout)
        if enabled in ("o", "n"):
            payload["enabled"] = enabled == "o"
        if not payload:
            payload["enabled"] = True
        client.publish("esp32/sensors/request", json.dumps(payload))
        print("Configuration envoyée.")
    except ValueError:
        print("Valeur numérique invalide.")


def basculer_affichage_capteurs():
    global show_sensors
    show_sensors = not show_sensors
    etat = "activé" if show_sensors else "désactivé"
    print(f"Affichage des données capteurs {etat}.")


client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)
client.loop_start()

try:
    while True:
        action = choix_action()
        if action == "1":
            envoyer_couleur()
        elif action == "2":
            jouer_son()
        elif action == "3":
            envoyer_tts()
        elif action == "4":
            configurer_capteurs()
        elif action == "5":
            basculer_affichage_capteurs()
        elif action == "6":
            print("Quitter le programme.")
            break
        else:
            print("Action non reconnue.")
except KeyboardInterrupt:
    print("Interruption utilisateur.")

client.loop_stop()
client.disconnect()
