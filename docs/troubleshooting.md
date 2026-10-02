# Troubleshooting

Work through the chain in order: local Wake-on-LAN, ESP32 connectivity, MQTT delivery, then Tailscale and RDP. Never fix a connection problem by disabling TLS verification or forwarding RDP to the Internet.

| Symptom | Checks |
| --- | --- |
| Firmware cannot find `secrets.h` or `config.h` | Copy the two firmware example headers to their local names beside the sketch and fill them in. |
| Firmware does not compile | Select ESP32 Dev Module, install the Espressif board package and PubSubClient, and check header syntax, especially MAC bytes and broadcast octets. |
| Wi-Fi never connects | Check local Wi-Fi credentials, ESP32-supported Wi-Fi, signal strength, and power. The connection loop retries indefinitely. |
| Clock synchronization hangs | Allow NTP access and DNS resolution. A correct clock is required for certificate validity checks. |
| TLS/MQTT connection fails | Check hostname, port, Internet access, clock, username/password, and the provider's CA chain. Keep CA validation enabled. |
| MQTT connects but subscription fails | Check the ESP32 subscribe permission on the exact topic, deny-rule ordering, and client-ID uniqueness. |
| Local Windows configuration is missing | Copy `windows/config.example.json` to `windows/config.json`, then edit the local file. |
| Password is missing | Store it in Windows Credential Manager using the same account, service string, and MQTT username as the client. Check the active keyring backend. |
| Python reports missing modules or callback API errors | Install `requirements.txt` using the same `python` executable used by PowerShell; Paho 2.x is required. |
| Publish fails or times out | Check TLS access and the controller's publish permission. A broker can reject or disconnect an unauthorized publisher. |
| Publish succeeds but the desktop stays asleep | Confirm ESP32 subscription and Serial Monitor output. Verify topic agreement, no cooldown suppression, wired MAC, subnet broadcast, Wi-Fi-to-Ethernet reachability, BIOS/NIC settings, and the tested sleep state. |
| Desktop wakes unexpectedly after an ESP32 reconnect | Clear a retained wake message in EMQX. Normal client publication is non-retained. |
| Desktop wakes but the launcher times out | Check both Tailscale sessions, desktop unattended mode, address, tailnet policy, Windows Firewall, RDP service, and configured TCP port. Try RDP while the desktop is already awake. |
| RDP opens but sign-in fails | Check Windows account credentials and Remote Desktop permissions. The MQTT credential is unrelated to Windows authentication. |

## Safe diagnostics

Run these locally from the repository root:

```powershell
python -m keyring diagnose
git check-ignore firmware/esp32_remote_control/secrets.h firmware/esp32_remote_control/config.h windows/config.json
```

For a TCP check, substitute your desktop's Tailscale address locally:

```powershell
Test-NetConnection -ComputerName YOUR_DESKTOP_TAILSCALE_IP -Port 3389
```

Review diagnostic output before sharing it. Redact credentials, broker hostnames, device addresses, Wi-Fi identifiers, and usernames where appropriate. Do not post local configuration or credential files.

The launcher waits about 90 seconds and exits with failure if the desktop remains unavailable. A successful TCP check confirms reachability only; complete an actual RDP sign-in to validate the access path.
