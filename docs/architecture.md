# Architecture

The wake path and desktop access path serve different purposes:

```text
Laptop -> MQTT over TLS -> EMQX Cloud -> MQTT over TLS -> ESP32
ESP32 -> local UDP magic packet -> Windows desktop wakes
Laptop -> Tailscale -> Windows Remote Desktop
```

The laptop can be on a mobile hotspot or another external network. The ESP32 stays powered and connected to the home Wi-Fi while the desktop sleeps. The desktop's Ethernet adapter must remain capable of receiving Wake-on-LAN packets, and the home network must allow Wi-Fi-to-Ethernet broadcast traffic.

## Firmware

On boot the ESP32 connects to Wi-Fi, synchronizes its clock using NTP, validates the broker's TLS certificate with the embedded public DigiCert Global Root G2 certificate, authenticates with its own MQTT identity, and subscribes at QoS 1 to the configured topic. The firmware reconnects when Wi-Fi or MQTT connectivity is lost. Connection and clock loops retry indefinitely; they are not bounded health checks.

The callback trims surrounding whitespace and accepts only `WAKE`. It builds the standard 102-byte magic packet: six `FF` bytes followed by sixteen repetitions of the target MAC. It sends three UDP broadcasts, 100 milliseconds apart, and suppresses additional accepted commands for 30 seconds. This also limits duplicate QoS 1 deliveries.

Credentials come from local `secrets.h`; deployment values come from local `config.h`. The embedded CA certificate is public trust material, not a private key. If the broker changes its trust chain, update the CA after verifying the provider's certificate guidance.

## Windows clients

The Python client reads `config.json` beside the script and retrieves the password using the configured keyring service and MQTT username. It uses Paho's version 2 callback API, MQTT 3.1.1, and certificate validation with the `certifi` CA bundle. It waits for connection, publishes `WAKE` at QoS 1 with `retain=False`, and checks that publication was acknowledged within the wait period.

The PowerShell launcher runs that client, polls the configured Tailscale address and RDP TCP port, and starts Remote Desktop once the port is reachable. It waits up to approximately 90 seconds; individual connection attempts can extend the final timeout slightly. A reachable port does not prove that Windows credentials will be accepted.

## Delivery limits

There is no ESP32 acknowledgement topic or desktop power-state feedback. A broker acknowledgement confirms MQTT publication only. QoS 1 can duplicate delivery; it does not guarantee wake completion. Do not retain wake commands: a retained command can wake the machine when the ESP32 reconnects. Existing retained messages must be cleared at the broker.

Sleep was tested in the original installation. Hibernate, shutdown, recovery after power failure, and different hardware or subnets require separate local testing. Tailscale provides the access path after waking; it does not relay this LAN broadcast.
