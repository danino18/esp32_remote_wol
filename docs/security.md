# Security

## Network boundaries

The ESP32 makes an outbound MQTT-over-TLS connection to EMQX Cloud. The laptop also uses certificate-validated TLS. No ESP32 port forwarding is needed. Desktop access uses Tailscale and Windows authentication; never expose RDP directly to the Internet. Router configuration, tailnet access policy, Windows Firewall, and account permissions are deployment responsibilities, not enforced by these scripts.

TLS protects the broker connections. Wake-on-LAN on the home LAN is an unauthenticated UDP broadcast; it is not encrypted by MQTT or Tailscale. The desktop must be on a trusted LAN.

## Least privilege

Use separate MQTT users with distinct passwords. Allow the ESP32 only to subscribe to the one wake topic; allow the laptop only to publish to that topic. Deny all other publish/subscribe operations and verify the effective rule order in EMQX. Authentication alone does not enforce topic permissions.

The firmware accepts only the case-sensitive `WAKE` command after trimming surrounding whitespace. It has no remote shell or arbitrary command execution. Its cooldown reduces repeated waking; it is not a substitute for authorization. Anyone with valid publisher credentials and topic access can request a wake.

The publisher sets `retain=False`, but the firmware does not distinguish retained deliveries. Clear old retained commands and prevent retained publication through broker policy where available. MQTT publication acknowledgement is not an acknowledgement from the ESP32.

## Credentials and local configuration

| Local file or store | Contents | Git handling |
| --- | --- | --- |
| `firmware/esp32_remote_control/secrets.h` | Wi-Fi and ESP32 MQTT credentials | Ignored |
| `firmware/esp32_remote_control/config.h` | Broker, topic, client ID, MAC, broadcast | Ignored |
| `windows/config.json` | Laptop MQTT metadata and Tailscale address | Ignored |
| Windows Credential Manager | Laptop MQTT password, keyed by service and username | Outside the repository |

Public example files contain placeholders and sample settings. The firmware CA certificate is a public certificate and safe to commit. Firmware credentials are compiled into the device image; Git exclusion does not protect them against someone with physical device access. Keep local files and the ESP32 under your control.

Use the Windows keyring backend. Do not configure a plaintext fallback. Do not include credentials in command-line arguments, screenshots, debug logs, issues, or example files. The firmware logs incoming MQTT payloads, so never send credentials on its wake topic.

## Before publishing

1. Run `git status --short --untracked-files=all` and check that the three local files are ignored.
2. Inspect every proposed public file, including newly created examples and documentation. Look for real Wi-Fi values, MQTT passwords/hosts, device addresses, tokens, and private keys.
3. If staging later, inspect `git diff --cached --name-only` and the full staged diff. Ignore rules do not remove files that were already tracked.
4. Run a secret scanner if available, and review findings without copying sensitive values into logs. Automated scans supplement manual review; they cannot prove the absence of all secrets.
5. Keep commits and pushes pending until the intended public diff has been approved.

If a credential was committed or disclosed, revoke or rotate it. Deleting it in a later commit does not erase earlier history. The known-working backup is outside this repository and should remain untouched.
