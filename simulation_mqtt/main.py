import http.server
import re
import shutil
import socket
import socketserver
import threading
import time
from pathlib import Path

import paho.mqtt.client as mqtt

HTTP_BIND = "0.0.0.0"
HTTP_PORT = 8000
HTTP_PATH = "/stream"
AUDIO_FILE = Path(__file__).resolve().parent.parent / "sample-1.wav"
MQTT_BROKER = "test.mosquitto.org"
MQTT_PORT = 1883
MQTT_TOPICS = ("esp32/color", "esp32/http")

pattern = re.compile(r"^[0-9A-F]{6}$")


def is_hex_color(s: str) -> bool:
    return bool(pattern.fullmatch(s))


def get_lan_ip() -> str:
    """Return the LAN IP of this machine or fallback to localhost."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))
            return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"


class AudioRequestHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path.rstrip('/') != HTTP_PATH:
            self.send_error(404, "Not Found")
            return

        if not AUDIO_FILE.exists():
            self.send_error(404, "Audio file not found")
            return

        try:
            file_size = AUDIO_FILE.stat().st_size
            with AUDIO_FILE.open('rb') as audio_stream:
                self.send_response(200)
                self.send_header("Content-Type", "audio/wav")
                self.send_header("Content-Length", str(file_size))
                self.send_header("Cache-Control", "no-cache")
                self.end_headers()
                shutil.copyfileobj(audio_stream, self.wfile)
        except OSError as exc:
            self.send_error(500, f"Unable to read audio file: {exc}")

    def log_message(self, format, *args):
        # Suppress default HTTP logging to keep output readable for MQTT logs
        return


class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def start_http_server():
    server = ThreadedHTTPServer((HTTP_BIND, HTTP_PORT), AudioRequestHandler)
    lan_ip = get_lan_ip()
    print(f"HTTP server listening on http://{lan_ip}:{HTTP_PORT}{HTTP_PATH} (bind {HTTP_BIND})")
    print("Ensure clients are on the same network and firewall allows inbound connections.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        print("HTTP server stopped")


def mqtt_worker():
    mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message

    while True:
        try:
            mqttc.connect(MQTT_BROKER, MQTT_PORT, 60)
            mqttc.loop_forever()
        except KeyboardInterrupt:
            break
        except Exception as exc:
            print(f"MQTT connection error: {exc}. Retrying in 5s...")
            time.sleep(5)


def on_connect(client, userdata, flags, reason_code, properties):
    for topic in MQTT_TOPICS:
        client.subscribe(topic)


def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.payload))


if __name__ == "__main__":
    mqtt_thread = threading.Thread(target=mqtt_worker, name="MQTTWorker", daemon=True)
    mqtt_thread.start()

    start_http_server()
