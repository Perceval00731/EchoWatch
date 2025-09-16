import paho.mqtt.client as mqtt
import re

pattern = re.compile(r'^[0-9A-F]{6}$')
def is_hex_color(s: str) -> bool:
    return bool(pattern.fullmatch(s))

def on_connect(client, userdata, flags, reason_code, properties):
    client.subscribe("esp32/color")

def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))
    # Hexadécimal regex
    couleur = re.match("[0-9]")

mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_connect = on_connect
mqttc.on_message = on_message

mqttc.connect("test.mosquitto.org", 1883, 60)

mqttc.loop_forever()
