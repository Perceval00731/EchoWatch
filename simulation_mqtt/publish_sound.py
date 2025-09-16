import paho.mqtt.client as mqtt
import os
import time

def on_connect(client, userdata, flags, reason_code, properties):
    """Callback appelé lors de la connexion au broker MQTT"""
    print(f"Connecté au broker MQTT avec le code: {reason_code}")

def on_publish(client, userdata, mid, reason_code, properties):
    """Callback appelé lors de la publication d'un message"""
    print(f"Message publié avec l'ID: {mid}")

def publish_wav_file(wav_file_path, mqtt_broker="test.mosquitto.org", mqtt_port=1883, topic="esp32/sound"):
    """
    Publie un fichier WAV sur un topic MQTT
    
    Args:
        wav_file_path (str): Chemin vers le fichier WAV
        mqtt_broker (str): Adresse du broker MQTT
        mqtt_port (int): Port du broker MQTT
        topic (str): Topic MQTT pour publier le fichier
    """
    
    # Vérifier si le fichier existe
    if not os.path.exists(wav_file_path):
        print(f"Erreur: Le fichier {wav_file_path} n'existe pas")
        return False
    
    # Lire le fichier WAV en mode binaire
    try:
        with open(wav_file_path, 'rb') as wav_file:
            wav_data = wav_file.read()
            print(f"Fichier WAV lu: {len(wav_data)} bytes")
    except Exception as e:
        print(f"Erreur lors de la lecture du fichier: {e}")
        return False
    
    # Créer le client MQTT
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_publish = on_publish
    
    try:
        # Se connecter au broker
        print(f"Connexion au broker MQTT: {mqtt_broker}:{mqtt_port}")
        client.connect(mqtt_broker, mqtt_port, 60)
        
        # Démarrer la boucle en arrière-plan
        client.loop_start()
        
        # Attendre un peu pour que la connexion s'établisse
        time.sleep(2)
        
        # Publier le fichier WAV
        print(f"Publication du fichier sur le topic: {topic}")
        result = client.publish(topic, wav_data, qos=1)
        
        # Attendre que le message soit publié
        result.wait_for_publish()
        
        print("Fichier WAV publié avec succès!")
        
        # Arrêter la boucle et déconnecter
        client.loop_stop()
        client.disconnect()
        
        return True
        
    except Exception as e:
        print(f"Erreur MQTT: {e}")
        return False

def main():
    """Fonction principale"""
    # Chemin vers le fichier WAV
    current_dir = os.path.dirname(os.path.abspath(__file__))
    wav_file_path = os.path.join(current_dir, "ring.wav")
    
    print("=== Publication de fichier WAV sur MQTT ===")
    print(f"Fichier à publier: {wav_file_path}")
    
    # Publier le fichier
    success = publish_wav_file(
        wav_file_path=wav_file_path,
        mqtt_broker="test.mosquitto.org",
        mqtt_port=1883,
        topic="esp32/sound"
    )
    
    if success:
        print("✅ Publication terminée avec succès")
    else:
        print("❌ Échec de la publication")

if __name__ == "__main__":
    main()
