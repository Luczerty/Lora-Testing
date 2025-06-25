#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"
extern SSD1306Wire display;

#define Vext 21
#define BUFFER_SIZE 30
#define LOG_LINES 6

class LoRaConfig {
  public:
    uint32_t rfFrequency;
    int8_t txOutputPower;
    uint8_t loraBandwidth;
    uint8_t loraSpreadingFactor;
    uint8_t loraCodingRate;

    LoRaConfig(uint32_t freq = 434222222, int8_t power = 22,
               uint8_t bandwidth = 2, uint8_t spreadingFactor = 7, uint8_t codingRate = 1)
    {
      rfFrequency = freq;
      txOutputPower = power;
      loraBandwidth = bandwidth;
      loraSpreadingFactor = spreadingFactor;
      loraCodingRate = codingRate;
    }

    void printConfig() const {
      Serial.println("=== LoRa Configuration ===");
      Serial.print("RF Frequency (Hz): "); Serial.println(rfFrequency);
      Serial.print("TX Power (dBm): "); Serial.println(txOutputPower);
      Serial.print("Bandwidth (0=125kHz, 1=250kHz, 2=500kHz): "); Serial.println(loraBandwidth);
      Serial.print("Spreading Factor (7-12): "); Serial.println(loraSpreadingFactor);
      Serial.print("Coding Rate (1=4/5 to 4=4/8): "); Serial.println(loraCodingRate);
    }

    void setFrequency(uint32_t freq) { rfFrequency = freq; }
    void setTxPower(int8_t power) { txOutputPower = power; }
    void setBandwidth(uint8_t bw) { loraBandwidth = bw; }
    void setSpreadingFactor(uint8_t sf) { loraSpreadingFactor = sf; }
    void setCodingRate(uint8_t cr) { loraCodingRate = cr; }
};

LoRaConfig config;
char txpacket[BUFFER_SIZE];
bool lora_idle = true;
int n = 0;
String logs[LOG_LINES];
bool sendEnabled = true;

#define LORA_PREAMBLE_LENGTH 8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void display_init() {
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.display();
}

void radio_setup() {
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);

  Radio.SetPublicNetwork(true);  

  Radio.SetChannel(config.rfFrequency);
  Radio.SetTxConfig(
    MODEM_LORA,
    config.txOutputPower,
    0,
    config.loraBandwidth,
    config.loraSpreadingFactor,
    config.loraCodingRate,
    LORA_PREAMBLE_LENGTH,
    LORA_FIX_LENGTH_PAYLOAD_ON,
    false,
    0,
    0,
    LORA_IQ_INVERSION_ON,
    3000
  );
}

void restartRadio() {
  Serial.println("Restarting radio...");
  Radio.Sleep();
  delay(100);
  radio_setup();
  lora_idle = true;
  Serial.println("Radio restarted.");
}

void logMessage(String msg) {
  for (int i = 0; i < LOG_LINES - 1; i++) logs[i] = logs[i + 1];
  logs[LOG_LINES - 1] = "Sent: " + msg;

  display.clear();
  for (int i = 0; i < LOG_LINES; i++) {
    display.drawString(0, i * 10, logs[i]);
  }
  display.display();

  Serial.println("Sent: " + msg);
}

void OnTxDone() {
  Serial.println("Message sent successfully.");
  lora_idle = true;
}

void OnTxTimeout() {
  Radio.Sleep();
  Serial.println("Message transmission timed out.");
  lora_idle = true;
}

void handleSerialCommand() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toLowerCase();

  if (input.startsWith("set")) {
    int firstSpace = input.indexOf(' ');
    int secondSpace = input.indexOf(' ', firstSpace + 1);
    if (firstSpace == -1 || secondSpace == -1) {
      Serial.println("Invalid command. Use: set <param> <value>");
      return;
    }

    String param = input.substring(firstSpace + 1, secondSpace);
    String valueStr = input.substring(secondSpace + 1);
    uint32_t value = valueStr.toInt();

    if (param == "freq") {
      config.setFrequency(value);
      Serial.println("RF frequency updated.");
    } else if (param == "power") {
      if (value >= 2 && value <= 22) {
        config.setTxPower((int8_t)value);
        Serial.println("TX power updated.");
      } else {
        Serial.println("Invalid TX power (2–22 dBm).");
      }
    } else if (param == "bw") {
      if (value <= 2) {
        config.setBandwidth((uint8_t)value);
        Serial.println("Bandwidth updated.");
      } else {
        Serial.println("Invalid bandwidth (0–2).");
      }
    } else if (param == "sf") {
      if (value >= 7 && value <= 12) {
        config.setSpreadingFactor((uint8_t)value);
        Serial.println("Spreading factor updated.");
      } else {
        Serial.println("Invalid spreading factor (7–12).");
      }
    } else if (param == "cr") {
      if (value >= 1 && value <= 4) {
        config.setCodingRate((uint8_t)value);
        Serial.println("Coding rate updated.");
      } else {
        Serial.println("Invalid coding rate (1–4).");
      }
    } else {
      Serial.println("Unknown parameter.");
    }

    radio_setup();
    lora_idle = true;
  } else if (input == "show") {
    config.printConfig();

  } else if (input == "pause") {
    sendEnabled = false;
    Serial.println("Transmission paused.");

  } else if (input == "resume") {
    sendEnabled = true;
    Serial.println("Transmission resumed.");

  } else if (input == "restart") {
    sendEnabled = false;
    restartRadio();
    sendEnabled = true;

  } else {
    Serial.println("Unknown command. Use: set / show / pause / resume / restart");
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  VextON();
  delay(100);
  display_init();
  delay(100);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  Serial.println("=== LoRa Serial Control Ready ===");
  Serial.println("Available commands:");
  Serial.println("  set freq <Hz>");
  Serial.println("  set power <dBm>");
  Serial.println("  set bw <0-2>");
  Serial.println("  set sf <7-12>");
  Serial.println("  set cr <1-4>");
  Serial.println("  show");
  Serial.println("  pause / resume");
  Serial.println("  restart");

  config.printConfig();
  radio_setup();

  for (int i = 0; i < LOG_LINES; i++) logs[i] = "";
}

void loop() {
  handleSerialCommand();

  if (lora_idle && sendEnabled) {
    String message = "Test";
    String full_message = message + " " + String(n++);
    full_message.toCharArray(txpacket, BUFFER_SIZE);
    logMessage(full_message);
  }

  Radio.IrqProcess();
  delay(3000);
}

