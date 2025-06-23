#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"

#define Vext 21
#define BUFFER_SIZE 30
#define LOG_LINES 6

// OLED (déjà défini par Heltec si nécessaire)
extern SSD1306Wire display;

class LoRaConfig {
  public:
    uint32_t rfFrequency;
    uint8_t loraBandwidth;
    uint8_t loraSpreadingFactor;
    uint8_t loraCodingRate;

    LoRaConfig(uint32_t freq = 434222222, uint8_t bw = 2, uint8_t sf =7, uint8_t cr = 1) {
      rfFrequency = freq;
      loraBandwidth = bw;
      loraSpreadingFactor = sf;
      loraCodingRate = cr;
    }

    void printConfig() const {
      Serial.println("=== LoRa RX Configuration ===");
      Serial.print("Frequency: "); Serial.println(rfFrequency);
      Serial.print("Bandwidth (0=125kHz, 1=250kHz, 2=500kHz): "); Serial.println(loraBandwidth);
      Serial.print("Spreading Factor (7–12): "); Serial.println(loraSpreadingFactor);
      Serial.print("Coding Rate (1=4/5 to 4=4/8): "); Serial.println(loraCodingRate);
    }

    void setFrequency(uint32_t freq) { rfFrequency = freq; }
    void setBandwidth(uint8_t bw) { loraBandwidth = bw; }
    void setSpreadingFactor(uint8_t sf) { loraSpreadingFactor = sf; }
    void setCodingRate(uint8_t cr) { loraCodingRate = cr; }
};

LoRaConfig config;
char rxpacket[BUFFER_SIZE];
String logs[LOG_LINES];
bool receivingEnabled = true;

static RadioEvents_t RadioEvents;
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

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
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);

  Radio.SetChannel(config.rfFrequency);

  Radio.SetRxConfig(
    MODEM_LORA,
    config.loraBandwidth,
    config.loraSpreadingFactor,
    config.loraCodingRate,
    0,
    8,
    0,
    false,
    0,
    true,
    false,
    0,
    false,
    true
  );

  if (receivingEnabled) {
    Radio.Rx(0);
  }
}

void restartRadio() {
  Serial.println("Restarting LoRa radio...");
  Radio.Sleep();
  delay(100);
  receivingEnabled = true;
  radio_setup();
  Serial.println("LoRa radio restarted.");
}

void setup() {
  Serial.begin(115200);
  delay(100);

  VextON();
  delay(100);
  display_init();
  delay(100);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  Serial.println("=== LoRa Receiver Ready ===");
  Serial.println("Commands:");
  Serial.println("  set freq <Hz>");
  Serial.println("  set bw <0-2>");
  Serial.println("  set sf <7-12>");
  Serial.println("  set cr <1-4>");
  Serial.println("  pause / resume");
  Serial.println("  show / restart");

  config.printConfig();
  radio_setup();

  for (int i = 0; i < LOG_LINES; i++) logs[i] = "";
}

void loop() {
  handleSerialCommand();
  Radio.IrqProcess();
  delay(10);
}

void logMessage(String msg) {
  for (int i = 0; i < LOG_LINES - 1; i++) logs[i] = logs[i + 1];
  logs[LOG_LINES - 1] = msg;

  display.clear();
  for (int i = 0; i < LOG_LINES; i++) {
    display.drawString(0, i * 10, logs[i]);
  }
  display.display();
  Serial.println(msg);

}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (!receivingEnabled) return;

    //Serial.write(payload, size);

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  String msg = String(rxpacket);
  String logEntry = msg + " RSI " + String(rssi) + " SN " + String(snr) ;
  logMessage(logEntry);


  Radio.Rx(0); // Restart RX mode
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
      Serial.println("Invalid format: set <param> <value>");
      return;
    }

    String param = input.substring(firstSpace + 1, secondSpace);
    String valueStr = input.substring(secondSpace + 1);
    uint32_t value = valueStr.toInt();

    if (param == "freq") {
      config.setFrequency(value);
      Serial.println("Frequency updated.");
    } else if (param == "bw" && value <= 2) {
      config.setBandwidth((uint8_t)value);
      Serial.println("Bandwidth updated.");
    } else if (param == "sf" && value >= 7 && value <= 12) {
      config.setSpreadingFactor((uint8_t)value);
      Serial.println("Spreading Factor updated.");
    } else if (param == "cr" && value >= 1 && value <= 4) {
      config.setCodingRate((uint8_t)value);
      Serial.println("Coding Rate updated.");
    } else {
      Serial.println("Unknown parameter or invalid value.");
      return;
    }

    radio_setup();

  } else if (input == "show") {
    config.printConfig();

  } else if (input == "pause") {
    receivingEnabled = false;
    Radio.Sleep();
    Serial.println("Reception paused.");

  } else if (input == "resume") {
    if (!receivingEnabled) {
      receivingEnabled = true;
      Radio.Rx(0);
      Serial.println("Reception resumed.");

    }

  } else if (input == "restart") {
    restartRadio();

  } else {
    Serial.println("Unknown command. Use: set / show / pause / resume / restart");
  }
}
