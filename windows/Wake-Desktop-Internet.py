import json
import ssl
import sys
import time
from pathlib import Path

import certifi
import keyring
import paho.mqtt.client as mqtt


CONFIG_PATH = Path(__file__).with_name("config.json")

if not CONFIG_PATH.exists():
    print("ERROR: windows/config.json was not found.")
    print("Copy config.example.json to config.json and fill in your values.")
    sys.exit(1)

with CONFIG_PATH.open("r", encoding="utf-8-sig") as f:
    config = json.load(f)

broker = config["mqtt_host"]
port = config.get("mqtt_port", 8883)
username = config["mqtt_username"]
client_id = config["mqtt_client_id"]
topic = config["mqtt_topic"]
keyring_service = config["keyring_service"]

password = keyring.get_password(keyring_service, username)

if not password:
    print("ERROR: MQTT password was not found in Windows Credential Manager.")
    sys.exit(1)

connected = False
connection_error = None


def on_connect(client, userdata, flags, reason_code, properties):
    global connected, connection_error

    if reason_code == 0:
        connected = True
        print("Securely connected to MQTT broker.")
    else:
        connection_error = str(reason_code)


client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id=client_id,
    protocol=mqtt.MQTTv311,
)

client.username_pw_set(username, password)

client.tls_set(
    ca_certs=certifi.where(),
    cert_reqs=ssl.CERT_REQUIRED,
    tls_version=ssl.PROTOCOL_TLS_CLIENT,
)

client.on_connect = on_connect

print("Connecting to MQTT broker...")

try:
    client.connect(broker, port, 30)
    client.loop_start()

    timeout = time.time() + 10

    while not connected and connection_error is None and time.time() < timeout:
        time.sleep(0.1)

    if not connected:
        print("ERROR: Could not connect to MQTT broker.")
        if connection_error:
            print("Reason:", connection_error)
        sys.exit(1)

    result = client.publish(
        topic,
        payload="WAKE",
        qos=1,
        retain=False,
    )

    result.wait_for_publish(timeout=10)

    if result.rc != mqtt.MQTT_ERR_SUCCESS or not result.is_published():
        print("ERROR: MQTT publish failed:", result.rc)
        sys.exit(1)

    print("WAKE command sent successfully.")

finally:
    client.loop_stop()
    client.disconnect()
