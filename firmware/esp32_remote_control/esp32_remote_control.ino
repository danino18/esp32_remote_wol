#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <time.h>


#include "secrets.h"
#include "config.h"
// =========================
// Wi-Fi
// =========================
const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;

// =========================
// EMQX MQTT
// =========================
const char* mqtt_host = MQTT_HOST;

const int mqtt_port = MQTT_PORT;

const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;

const char* mqtt_client_id = MQTT_CLIENT_ID;
const char* mqtt_topic = MQTT_TOPIC;

// =========================
// EMQX CA certificate
// =========================
static const char root_ca[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

// =========================
// Wake-on-LAN
// =========================
WiFiUDP udp;

const uint8_t targetMac[6] = { TARGET_MAC_BYTES };

IPAddress broadcastIp(WOL_BROADCAST_OCTETS);
const int wolPort = WOL_PORT;

// =========================
// MQTT objects
// =========================
WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

unsigned long lastWake = 0;
const unsigned long wakeCooldown = 30000;


// =========================
// Send Wake-on-LAN
// =========================
void sendWakeOnLan() {

  // Prevent repeated wake commands
  if (millis() - lastWake < wakeCooldown && lastWake != 0) {
    Serial.println("Wake ignored: cooldown active");
    return;
  }

  uint8_t packet[102];

  for (int i = 0; i < 6; i++) {
    packet[i] = 0xFF;
  }

  for (int i = 1; i <= 16; i++) {
    memcpy(&packet[i * 6], targetMac, 6);
  }

  // Send three times for reliability
  for (int i = 0; i < 3; i++) {
    udp.beginPacket(broadcastIp, wolPort);
    udp.write(packet, sizeof(packet));
    udp.endPacket();

    delay(100);
  }

  lastWake = millis();

  Serial.println("Wake-on-LAN packet sent");
}


// =========================
// MQTT message received
// =========================
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  String message;

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  message.trim();

  Serial.print("MQTT message received: ");
  Serial.println(message);

  // Only one valid command
  if (message == "WAKE") {
    sendWakeOnLan();
  }
  else {
    Serial.println("Unknown command ignored");
  }
}


// =========================
// Connect Wi-Fi
// =========================
void connectWiFi() {

  Serial.print("Connecting to Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected");

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  udp.begin(wolPort);
}


// =========================
// Set correct time for TLS
// =========================
void syncClock() {

  Serial.print("Synchronizing clock");

  configTime(
    0,
    0,
    "pool.ntp.org",
    "time.google.com"
  );

  time_t now = time(nullptr);

  while (now < 1700000000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println();
  Serial.println("Clock synchronized");
}


// =========================
// Connect MQTT
// =========================
void connectMQTT() {

  while (!mqtt.connected()) {

    Serial.print("Connecting securely to EMQX... ");

    if (mqtt.connect(
          mqtt_client_id,
          mqtt_username,
          mqtt_password
        )) {

      Serial.println("CONNECTED");

      if (mqtt.subscribe(mqtt_topic, 1)) {
        Serial.print("Subscribed to: ");
        Serial.println(mqtt_topic);
      }
      else {
        Serial.println("MQTT subscription failed");
      }
    }
    else {

      Serial.print("FAILED, MQTT state=");
      Serial.println(mqtt.state());

      delay(5000);
    }
  }
}


// =========================
// Setup
// =========================
void setup() {

  Serial.begin(115200);
  delay(1000);

  connectWiFi();

  // TLS certificate validation needs correct time
  syncClock();

  secureClient.setCACert(root_ca);

  mqtt.setServer(mqtt_host, mqtt_port);
  mqtt.setCallback(mqttCallback);
  mqtt.setKeepAlive(30);

  connectMQTT();

  Serial.println();
  Serial.println("ESP32 WOL controller ready");
}


// =========================
// Main loop
// =========================
void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqtt.connected()) {
    connectMQTT();
  }

  mqtt.loop();
}
