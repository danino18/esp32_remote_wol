# Setup

## 1. Prepare the desktop and network

1. Connect the desktop by Ethernet. Enable Wake-on-LAN in its firmware/BIOS and Ethernet adapter settings, including wake on magic packet.
2. Record the wired adapter's MAC address and the broadcast address of its actual subnet. A `/24` network often ends in `.255`; other subnet masks have different broadcast addresses.
3. Test waking from Sleep on the same LAN before setting up Internet control. Configure Windows so automatic hibernation does not move it out of the tested sleep state. Test shutdown or hibernation separately if needed.
4. Keep the ESP32 powered independently of the sleeping desktop. Use Wi-Fi that can reach the desktop's wired LAN; guest networks and client isolation may block broadcasts.
5. Enable Remote Desktop on an RDP-capable Windows edition and configure an authorized Windows account. Keep Network Level Authentication enabled.

## 2. Configure EMQX Cloud

Create a deployment with a TLS MQTT endpoint. Copy its hostname and TLS port (normally `8883`). Create two distinct authenticated users with distinct passwords:

| Identity | Topic | Action | Permission |
| --- | --- | --- | --- |
| `esp32-wol` | `home/desktop/wake` | Subscribe | Allow |
| `wol-controller` | `home/desktop/wake` | Publish | Allow |
| All users | `#` | Publish and Subscribe | Deny |

Use exact topic rules and apply the specific allows before the general deny according to your deployment's rule precedence. Remove any broader allow rules. See [EMQX default authorization](https://docs.emqx.com/en/cloud/latest/deployments/default_authz.html) for deployment-specific deny-by-default configuration.

Verify that the ESP32 identity cannot publish and the laptop identity cannot subscribe, and that both are denied access to unrelated topics. Publish wake messages without retaining them; clear any existing retained message through the broker's administration tools.

## 3. Configure and upload the ESP32

Install Arduino IDE, the Espressif ESP32 board package, and the **PubSubClient** library by Nick O'Leary. Wi-Fi, secure client, and UDP headers are provided by the ESP32 board package.

From the repository root, copy the templates only if the local files do not already exist:

```powershell
Copy-Item .\firmware\esp32_remote_control\secrets.example.h .\firmware\esp32_remote_control\secrets.h
Copy-Item .\firmware\esp32_remote_control\config.example.h .\firmware\esp32_remote_control\config.h
```

Edit `secrets.h` locally with the Wi-Fi SSID/password and the ESP32 MQTT username/password. Edit firmware `config.h` with the broker hostname/port, unique client ID, exact topic, six target MAC bytes, subnet broadcast octets, and UDP port. The two clients must use different MQTT client IDs.

The sketch currently embeds DigiCert Global Root G2. Confirm that this is the correct trust anchor for your broker, replacing it with the provider's verified CA if needed. Never bypass certificate validation to fix a TLS problem.

Open `firmware/esp32_remote_control/esp32_remote_control.ino`, select **ESP32 Dev Module** and the correct serial port, run **Verify**, and upload. Open Serial Monitor at **115200 baud**. Expect Wi-Fi connection, clock synchronization, a secure MQTT connection, subscription confirmation, and `ESP32 WOL controller ready`.

## 4. Configure the laptop

Install Python 3.10+ with Windows support and ensure `python` is available in the same environment used by the launcher. From the repository root:

```powershell
python -m pip install -r requirements.txt
Copy-Item .\windows\config.example.json .\windows\config.json
```

Skip the copy if `config.json` already exists. Set the broker hostname, laptop MQTT username, unique client ID, matching topic, keyring service, desktop Tailscale IP, and RDP port. Do not put a password in this file.

Store the laptop MQTT password interactively in **Windows Credential Manager**. Run the following in Windows using the same Python installation and Windows account as the launcher:

```powershell
@'
import getpass
import json
from pathlib import Path
import keyring
from keyring.backends.Windows import WinVaultKeyring

config = json.loads(Path("windows/config.json").read_text(encoding="utf-8-sig"))
keyring.set_keyring(WinVaultKeyring())
keyring.set_password(
    config["keyring_service"],
    config["mqtt_username"],
    getpass.getpass("MQTT password: "),
)
'@ | python -
```

The prompt hides input and avoids placing the password in command history. Check `python -m keyring diagnose` locally to confirm the normal runtime selects the Windows backend. The client uses `keyring`'s selected backend; a custom backend must not redirect storage to a plaintext file.

## 5. Configure Tailscale and RDP

Install Tailscale on both Windows machines, sign into the intended tailnet, and permit the laptop to access the desktop's RDP port through your tailnet access policy. Enable **Run unattended** on the desktop so it remains accessible without an interactive sign-in. See [unattended mode](https://tailscale.com/docs/how-to/run-unattended) and [RDP over Tailscale](https://tailscale.com/docs/solutions/access-remote-desktops-using-windows-rdp).

Use the desktop's Tailscale address in `windows/config.json`. Verify RDP over Tailscale while the desktop is awake, then test after waking from Sleep. Restrict Windows Firewall RDP access to the intended Tailscale access path. Do not forward TCP 3389, the ESP32, or a Wake-on-LAN UDP port on the router.

## 6. Verify the full workflow

1. With Serial Monitor open, put the desktop to Sleep and run `python .\windows\Wake-Desktop-Internet.py`.
2. Confirm the ESP32 reports sending the magic packet and the desktop wakes. A publish success alone is insufficient.
3. Run `powershell.exe -NoProfile -File .\windows\Wake-And-Connect.ps1` and confirm RDP opens and authenticates.
4. Repeat from a mobile hotspot with laptop Wi-Fi disconnected from the home network. This scenario passed in the original installation; test your deployment independently.

An optional desktop shortcut can target `powershell.exe -NoProfile -File "C:\path\to\esp32-remote-wol\windows\Wake-And-Connect.ps1"`. Follow your organization's execution-policy requirements if PowerShell blocks the script.

Before any commit, confirm all three local files are ignored with `git check-ignore firmware/esp32_remote_control/secrets.h firmware/esp32_remote_control/config.h windows/config.json`, then follow the review steps in [security](security.md).
