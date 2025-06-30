#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"

#define Vext 21
#define LOG_LINES 6
#define MAX_LINE_LEN 21  // ~21 caractères par ligne OLED

extern SSD1306Wire display;

String logs[LOG_LINES];
bool receivingEnabled = true;
volatile bool receivedFlag = false;

uint8_t* receivedBuffer = nullptr;
uint16_t receivedSize = 0;
int16_t lastRSSI = 0;
int8_t lastSNR = 0;

class LoRaConfig {
public:
  uint32_t rfFrequency;
  uint8_t loraBandwidth;
  uint8_t loraSpreadingFactor;
  uint8_t loraCodingRate;

  LoRaConfig(uint32_t freq = 434000000, uint8_t bw = 0, uint8_t sf = 11, uint8_t cr = 1)
      : rfFrequency(freq), loraBandwidth(bw), loraSpreadingFactor(sf), loraCodingRate(cr) {}
};

LoRaConfig config;
static RadioEvents_t RadioEvents;

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

void logHexMultiline(const String& hex, const String& meta = "") {
  Serial.println("[LOG RX] " + hex);
  if (meta.length() > 0) Serial.println(meta);

  int start = 0;
  while (start < hex.length()) {
    String line = hex.substring(start, start + MAX_LINE_LEN);

    for (int i = 0; i < LOG_LINES - 1; i++) {
      logs[i] = logs[i + 1];
    }
    logs[LOG_LINES - 1] = line;
    start += MAX_LINE_LEN;
  }

  if (meta.length() > 0) {
    for (int i = 0; i < LOG_LINES - 1; i++) {
      logs[i] = logs[i + 1];
    }
    logs[LOG_LINES - 1] = meta;
  }

  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  for (int i = 0; i < LOG_LINES; i++) {
    display.drawString(0, i * 10, logs[i]);
  }
  display.display();
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (!receivingEnabled) return;

  if (receivedBuffer != nullptr) {
    free(receivedBuffer);  // Libère le précédent
  }

  receivedBuffer = (uint8_t*)malloc(size);
  if (receivedBuffer == nullptr) {
    Serial.println("[ERROR] malloc failed in OnRxDone");
    return;
  }

  memcpy(receivedBuffer, payload, size);
  receivedSize = size;
  lastRSSI = rssi;
  lastSNR = snr;
  receivedFlag = true;
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
    0, 8, 0,
    false, 0, true,
    false, 0,
    false, true
  );

  if (receivingEnabled) Radio.Rx(0);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  VextON();
  delay(100);
  display_init();
  delay(100);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  radio_setup();

  for (int i = 0; i < LOG_LINES; i++) logs[i] = "";

  Serial.println("LoRa RX ready.");
  logHexMultiline("LoRa RX ready");
}

void loop() {
  Radio.IrqProcess();

  if (receivedFlag && receivedBuffer != nullptr) {
    receivedFlag = false;

    Serial.print("Received size: ");
    Serial.println(receivedSize);

    String hexMsg = "";
    for (int i = 0; i < receivedSize; i++) {
      if (receivedBuffer[i] < 0x10) hexMsg += "0";
      hexMsg += String(receivedBuffer[i], HEX);
    }
    hexMsg.toUpperCase();

    String meta = "RSSI " + String(lastRSSI) + " SNR " + String(lastSNR);
    logHexMultiline(hexMsg, meta);

    free(receivedBuffer);
    receivedBuffer = nullptr;
  }

  delay(10);
}
