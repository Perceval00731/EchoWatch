import paho.mqtt.client as mqtt
import os
import time
import json

def on_connect(client, userdata, flags, reason_code, properties):
    """Callback appelé lors de la connexion au broker MQTT"""
    print(f"Connecté au broker MQTT avec le code: {reason_code}")

def on_publish(client, userdata, mid, reason_code, properties):
    """Callback appelé lors de la publication d'un message"""
    print(f"Chunk publié avec l'ID: {mid}")

def publish_wav_chunks(wav_file_path, chunk_size=50000, mqtt_broker="test.mosquitto.org", mqtt_port=1883):
    """
    Publie un fichier WAV en chunks via MQTT
    
    Args:
        wav_file_path (str): Chemin vers le fichier WAV
        chunk_size (int): Taille de chaque chunk en bytes (50KB par défaut)
        mqtt_broker (str): Adresse du broker MQTT
        mqtt_port (int): Port du broker MQTT
    """
    
    # Vérifier si le fichier existe
    if not os.path.exists(wav_file_path):
        print(f"Erreur: Le fichier {wav_file_path} n'existe pas")
        return False
    
    # Lire le fichier WAV en mode binaire
    try:
        with open(wav_file_path, 'rb') as wav_file:
            wav_data = wav_file.read()
            total_size = len(wav_data)
            print(f"Fichier WAV lu: {total_size} bytes")
    except Exception as e:
        print(f"Erreur lors de la lecture du fichier: {e}")
        return False
    
    # Calculer le nombre de chunks nécessaires
    num_chunks = (total_size + chunk_size - 1) // chunk_size  # Division avec arrondi supérieur
    print(f"Découpage en {num_chunks} chunks de {chunk_size} bytes max")
    
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
        
        # Publier les métadonnées du fichier d'abord
        metadata = {
            "total_size": total_size,
            "num_chunks": num_chunks,
            "chunk_size": chunk_size,
            "filename": "ring.wav"
        }
        
        print("Publication des métadonnées...")
        result = client.publish("esp32/sound/meta", json.dumps(metadata), qos=1)
        result.wait_for_publish()
        
        time.sleep(1)  # Pause entre métadonnées et chunks
        
        # Publier chaque chunk
        for i in range(num_chunks):
            start_pos = i * chunk_size
            end_pos = min(start_pos + chunk_size, total_size)
            chunk_data = wav_data[start_pos:end_pos]
            
            topic = f"esp32/sound/chunk/{i}"
            print(f"Publication chunk {i+1}/{num_chunks} ({len(chunk_data)} bytes) sur {topic}")
            
            result = client.publish(topic, chunk_data, qos=1)
            result.wait_for_publish()
            
            # Petite pause entre chaque chunk pour éviter la surcharge
            time.sleep(0.5)
        
        # Signal de fin
        print("Publication du signal de fin...")
        result = client.publish("esp32/sound/complete", "done", qos=1)
        result.wait_for_publish()
        
        print("Tous les chunks ont été publiés avec succès!")
        
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
    
    print("=== Publication de fichier WAV par chunks sur MQTT ===")
    print(f"Fichier à publier: {wav_file_path}")
    
    # Publier le fichier en chunks
    success = publish_wav_chunks(
        wav_file_path=wav_file_path,
        chunk_size=50000,  # 50KB par chunk
        mqtt_broker="test.mosquitto.org",
        mqtt_port=1883
    )
    
    if success:
        print("✅ Publication terminée avec succès")
    else:
        print("❌ Échec de la publication")

if __name__ == "__main__":
    main()
