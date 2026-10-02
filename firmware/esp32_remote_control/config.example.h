#pragma once

// Copy to config.h and replace these example values for your network.
#define MQTT_HOST "YOUR_EMQX_HOST.emqxsl.com"
#define MQTT_PORT 8883
#define MQTT_CLIENT_ID "esp32-wol"
#define MQTT_TOPIC "home/desktop/wake"

// Six hexadecimal MAC bytes of the desktop's wired Ethernet adapter.
#define TARGET_MAC_BYTES 0x02, 0x00, 0x00, 0x00, 0x00, 0x01
// Replace with the broadcast address for your actual subnet.
#define WOL_BROADCAST_OCTETS 192, 168, 1, 255
#define WOL_PORT 9
