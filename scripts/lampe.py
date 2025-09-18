#!/usr/bin/env python3
# Simulateur de lampe MQTT simple
# - Souscrit à esp32/lampe (commande: ON/OFF)
# - Publie sur esp32/lampe/ack l'état après application
# - Optionnel: echo périodique de l'état

import os
import sys
import time
import signal
import argparse
import threading

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Veuillez installer paho-mqtt: pip install paho-mqtt", file=sys.stderr)
    sys.exit(1)

BROKER = os.environ.get("MQTT_BROKER", "test.mosquitto.org")
PORT = int(os.environ.get("MQTT_PORT", "1883"))
TOPIC_CMD = os.environ.get("MQTT_TOPIC_CMD", "esp32/lampe")
TOPIC_ACK = os.environ.get("MQTT_TOPIC_ACK", "esp32/lampe/ack")
CLIENT_ID = os.environ.get("MQTT_CLIENT_ID", "LampSim")

state_lock = threading.Lock()
state_on = False


def on_connect(client, userdata, flags, rc):
    print(f"[MQTT] connect rc={rc}")
    if rc == 0:
        client.subscribe(TOPIC_CMD)
    else:
        print("Connexion broker échouée")


def norm(payload: bytes) -> str:
    s = payload.decode("utf-8", errors="ignore").strip().upper()
    return s


def publish_ack(client):
    with state_lock:
        payload = "ON" if state_on else "OFF"
    client.publish(TOPIC_ACK, payload, qos=0, retain=False)
    print(f"[ACK] {payload}")


def on_message(client, userdata, msg):
    global state_on
    cmd = norm(msg.payload)
    print(f"[CMD] topic={msg.topic} payload={cmd}")
    if cmd in ("ON", "1", "TRUE"):
        with state_lock:
            state_on = True
        publish_ack(client)
    elif cmd in ("OFF", "0", "FALSE"):
        with state_lock:
            state_on = False
        publish_ack(client)
    else:
        print("Commande inconnue, ignorée")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--broker", default=BROKER)
    parser.add_argument("--port", type=int, default=PORT)
    args = parser.parse_args()

    client = mqtt.Client(client_id=CLIENT_ID, clean_session=True)
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(args.broker, args.port, keepalive=30)

    def handle_sig(*_):
        print("Stop...")
        client.disconnect()
        sys.exit(0)

    signal.signal(signal.SIGINT, handle_sig)
    signal.signal(signal.SIGTERM, handle_sig)

    client.loop_forever()


if __name__ == "__main__":
    main()
