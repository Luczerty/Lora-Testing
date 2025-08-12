
# 📡 Getting Started with TinyGS

## 1. 🔐 Create Your TinyGS Account

To register your ground station:

1. **Join the official TinyGS Telegram group**  
   👉 [Join Telegram Group](https://t.me/joinchat/DmYSElZahiJGwHX6jCzB3Q)

2. **Start a private chat** with the bot [@tinygs_personal_bot](https://t.me/tinygs_personal_bot) and send the following commands:
   - `/weblogin` – Link your Telegram account and register your email for web access.
   - `/mqtt` – Receive your personalized MQTT credentials.

---

## 2. 🚀 Install the TinyGS Firmware

You have two installation options:

### ✅ Option 1 – Via the Web Installer

- Go to the official [TinyGS Flasher](https://installer.tinygs.com)
- Select your board (e.g. **Heltec WiFi LoRa 32 V3**)
- Flash the latest firmware directly from your browser

### 💻 Option 2 – Using PlatformIO in VS Code

1. Download the latest [TinyGS release from GitHub](https://github.com/G4lile0/tinyGS/releases)
2. Open the project in **Visual Studio Code** with the **PlatformIO** extension
3. Make sure your `platformio.ini` file contains:

   ```ini
   [env:heltec_wifi_lora_32_V3]
   platform = espressif32@6.5.0
   board = heltec_wifi_lora_32_V3
   board_build.mcu = esp32s3
   framework = arduino
   ```

4. Connect your board via USB  
5. Click **"Upload"** in PlatformIO to flash the firmware

---

## 3. ⚙️ Configure Your TinyGS Node

1. Connect to the **Wi-Fi network** named `MYTinyGS`
2. Open a browser and navigate to: `http://192.168.4.1`
3. Fill in the following configuration:
   - Your **Wi-Fi SSID and password**
   - Your **station location**

4. Click **Apply**, then **Reset** the device

5. Reconnect to your usual Wi-Fi  
   The device should now appear on your local network with an IP like:  
   `http://192.168.1.XX` (the exact number may vary)

---

## 4. 🌐 Access the TinyGS Dashboard

Once the device is connected:

- Go to [https://dashboard.tinygs.com](https://dashboard.tinygs.com)
- Log in with the email you registered via `/weblogin`
- View your station's activity and statistics

---

## 5. 🎛️ Radio Tuning

Refer to the full guide here:  
👉 [Radio Tuning Guide on GitHub](https://github.com/G4lile0/tinyGS/wiki/Radio-Tuning-Guide)

### Configuration Parameters

| Parameter | Description |
|----------|-------------|
| `mode`   | Radio mode: `LoRa` or `FSK` |
| `freq`   | Frequency in MHz (e.g. `434.000`) |
| `bw`     | Bandwidth in kHz (e.g. `125.0`) |
| `sf`     | Spreading Factor (e.g. `9`) |
| `cr`     | LoRa Coding Rate (e.g. `5`) |
| `sw`     | Sync Word (e.g. `35`) |
| `pwr`    | Transmission Power (e.g. `22`) |
| `cl`     | Current Limit in mA (e.g. `120`) |
| `pl`     | Preamble Length (e.g. `8`) |
| `gain`   | LNA Gain (`0` = auto) |
| `crc`    | Enable CRC (`true` or `false`) |
| `fldro`  | Frequency Drift: `0` = disable, `1` = enable, `2` = auto |
| `sat`    | Satellite name |
| `NORAD`  | NORAD number of the satellite |
| `reg`    | Custom register overrides (advanced use) |

### Example Configuration

```json
{
  "mode": "LoRa",
  "freq": 434.000,
  "bw": 125.0,
  "sf": 9,
  "cr": 5,
  "sw": 35,
  "pwr": 22,
  "cl": 120,
  "pl": 8,
  "gain": 0,
  "crc": true,
  "fldro": 1,
  "sat": "not registered yet",
  "NORAD": "expecttochange"
}
```

---


