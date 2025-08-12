#include <LoRaWan_APP.h>
#include "Arduino.h"
#include "HT_SSD1306Wire.h"

#define Vext 21
#define LOG_LINES 6
#define MAX_LINE_LEN 21  // ≈ 128px / 6px par ligne OLED

extern SSD1306Wire display;

bool lora_idle = true;
String logs[LOG_LINES];

void OnTxDone();
void OnTxTimeout();
void logHexMultiline(const String& hex);
void VextON();
void display_init();

static RadioEvents_t RadioEvents;

void setup() {
  Serial.begin(115200);
  delay(100);

  VextON();
  delay(100);
  display_init();
  delay(100);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);

  Radio.SetChannel(434000000);

  Radio.SetTxConfig(
    MODEM_LORA,
    22,    // dBm
    0,     // FSK deviation
    0,     // Bandwidth 125 kHz
    9,    // SF11 pour fiabilité
    1,     // CR 4/5
    8,     // Preamble length
    false, false, false,
    0, false, 3000
  );

  Radio.Rx(0);

  for (int i = 0; i < LOG_LINES; i++) logs[i] = "";
  logHexMultiline("LoRa TX ready");
}

void loop() {
  Radio.IrqProcess();

  if (Serial.available() && lora_idle) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    int msgLen = msg.length();
    if (msgLen > 0) {
      // Allouer dynamiquement le buffer
      uint8_t* txpacket = (uint8_t*)malloc(msgLen);
      if (!txpacket) {
        Serial.println("[ERROR] Allocation failed");
        return;
      }

      for (int i = 0; i < msgLen; i++) {
        txpacket[i] = (uint8_t)msg[i];
      }

      Radio.Send(txpacket, msgLen);
      lora_idle = false;

      // Log hex
      String hexMsg = "";
      for (int i = 0; i < msgLen; i++) {
        if (txpacket[i] < 0x10) hexMsg += "0";
        hexMsg += String(txpacket[i], HEX);
      }
      hexMsg.toUpperCase();
      logHexMultiline("TX: " + hexMsg);

      free(txpacket);
    }
  }

  delay(10);
}

void OnTxDone() {
  logHexMultiline("TX done.");
  lora_idle = true;
  Radio.Rx(0);
}

void OnTxTimeout() {
  logHexMultiline("TX timeout.");
  lora_idle = true;
  Radio.Rx(0);
}

void logHexMultiline(const String& full) {
  Serial.println("[LOG] " + full);

  int start = 0;
  while (start < full.length()) {
    String line = full.substring(start, start + MAX_LINE_LEN);

    for (int i = 0; i < LOG_LINES - 1; i++) {
      logs[i] = logs[i + 1];
    }
    logs[LOG_LINES - 1] = line;
    start += MAX_LINE_LEN;
  }

  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  for (int i = 0; i < LOG_LINES; i++) {
    display.drawString(0, i * 10, logs[i]);
  }
  display.display();
}

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
