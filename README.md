# 📁 What You Will Find in This Repository

## 📌 Summary

1. A guide to install and configure TinyGS, including a setup example for the Heltec LoRa 32 V3.
2. A script to send and receive messages for radio range testing.
3. A script for the ground station to communicate with Coconut using the Heltec LoRa 32 V3.
4. An updated version of `thebridge.py` with a debug option.
5.  A guide to flash the scripts on  the Heltec LoRa 32 V3.

---

## 📄 Documentation Locations

- 🛠️ **TinyGS Installation and Configuration Guide**  
  → [`./installingandconfiguringTinyGS.md`](./installingandconfiguringTinyGS.md)

- 📡 **Radio Range Testing Script**  
  → [`./other_scripts/README.md`](./other_scripts/README.md)

- 🛰️ **Ground Station Scripts (Coconut Communication)**  
  → [`./GSE/README.md`](./GSE/README.md)

- 🧩 **New Version of `thebridge.py` with Debug Option**  
  → [`./new_bridge/README.md`](./new_bridge/README.md)

-  **LoRa radio Range Test Report**  
  → ['Lora range test report](https://docs.google.com/document/d/1vxGPzcisJnAlRVnOyCcJEj1ERbSNojEU1-XNJQtcNaU/edit?mode=html&tab=t.0)

-  **The guide to install a script***  
  → 🔽 See below for instructions


# 📥 Installing the Arduino IDE and Importing Sketches from GitHub

This guide explains how to:

1. Install the **Arduino IDE**
2. Import an **Arduino sketch** from **GitHub**
3. Use GitHub to find and clone other Arduino projects

---

## ✅ 1. Installing the Arduino IDE

### 🔗 Quick Method

- Visit the official website: [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

- Follow the installer instructions to complete the installation.

> 💡 **Tip:** You can also use the **Arduino Web Editor** (no installation needed):  
> [https://create.arduino.cc/editor](https://create.arduino.cc/editor)

---

## 📁 2. Importing a Sketch from a GitHub Repository to Arduino IDE

### 🧭 Steps

1. **Clone using Git (advanced):**

   Make sure Git is installed. Then, in your terminal:

   ```bash
   git clone git@github.com:Luczerty/Lora-Testing.git

2. **Open your Arduino IDE**

On the top bar, go to File > Open, and select the file you want to import.

![Example](./doc/cp.png)

3. **Install the library**
Click on the library icon  and install the HelTech ESP32 Dev-Board

![Install library example](./doc/library.png)

3. **Install the Board Manager**

Click on the icon below the folder icon and install the Arduino ESP32 Board.

![Board Manger Example](./doc/boardmanager.png)

You should see your board recognized in the top bar of the Arduino IDE:

![Board recognition Example](./doc/BoardReco.png)

If not, click on "Select other board", choose Heltec WiFi LoRa 32, and click OK.
5. **Upload it on your board**

Finally, click on Upload.

![Board recognition Example](./doc/upload.png)

## 🔐 License Problem (Heltec LoRa Board)

If you encounter the following error:


Please provide a correct license! For more information:
http://www.heltec.cn/search/
ESP32ChipID=XXXXXXXXXXX


### ✅ Solution

1. Go to the official Heltec license page:  
   👉 [http://www.heltec.cn/search/](http://www.heltec.cn/search/)

2. Enter your `ESP32ChipID` to retrieve your license key.

3. Open the **Serial Monitor** in the Arduino IDE (baud: 115200).

4. Send the following command in the Serial input, replacing with your actual license values:
