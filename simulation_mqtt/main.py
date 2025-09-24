import paho.mqtt.client as mqtt
import time

BROKER = "test.mosquitto.org"
PORT = 1883
TOPICS = ["esp32/color", "esp32/sound", "esp32/tts", "esp32/gyro"]

def on_connect(client, userdata, flags, rc):
    print(f"Connecté : {rc}")
    for topic in TOPICS:
        client.subscribe(topic)
        print(f"Abonné à {topic}")

def on_message(client, userdata, msg):
    print(f"Message reçu sur {msg.topic} : {msg.payload.decode()}")
    
def choixAction():
    print("Actions disponibles :")
    print("1. Changer la couleur (topic: esp32/color, payload: couleur)")
    print("2. Jouer un son (topic: esp32/sound, payload: nom_du_son)")
    print("3. Synthèse vocale (topic: esp32/tts, payload: texte)")
    print("4. Lire les données du gyroscope (topic: esp32/gyro, payload: 'read')")
    print("5. Switch lampe (topic: esp32/lampe, payload: 'on'/'off')")
    print("6. Quitter")
    action = str(input("Sélectionner une action : "))
    return action

def effectuerAction(action):
    if action == "1":
        couleur = str(input("Entrez la couleur hexadécimale (ex: #FF0000, #00FF00, #0000FF) : "))
        client.publish("esp32/color", couleur)
    elif action == "2":
        print("Déclenchement d'un son.")
        client.publish("esp32/sound", "n'importe quel payload, c'est juste un trigger")
    elif action == "3":
        texte = str(input("Entrez le texte à synthétiser : "))
        client.publish("esp32/tts", texte)
    elif action == "4":
        client.publish("esp32/gyro", "read")
    elif action == "5":
        etat = str(input("Entrez 'on' pour allumer ou 'off' pour éteindre la lampe : "))
        if etat in ["on", "off"]:
            client.publish("esp32/lampe", etat)
        else:
            print("État non reconnu. Utilisez 'on' ou 'off'.")
    elif action == "6":
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