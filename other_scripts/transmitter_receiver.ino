#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"

#define Vext 21
#define BUFFER_SIZE 30
#define LOG_LINES 6

extern SSD1306Wire display;

class LoRaConfig {
public:
  uint32_t rfFrequency;
  int8_t txOutputPower;
  uint8_t loraBandwidth;
  uint8_t loraSpreadingFactor;
  uint8_t loraCodingRate;

  LoRaConfig(uint32_t freq = 434222222, int8_t power = 22,
             uint8_t bw = 2, uint8_t sf = 7, uint8_t cr = 1)
      : rfFrequency(freq), txOutputPower(power),
        loraBandwidth(bw), loraSpreadingFactor(sf), loraCodingRate(cr) {}

  void printConfig() const {
    Serial.println("=== LoRa TX Configuration ===");
    Serial.print("Frequency: "); Serial.println(rfFrequency);
    Serial.print("TX Power: "); Serial.println(txOutputPower);
    Serial.print("Bandwidth: "); Serial.println(loraBandwidth);
    Serial.print("Spreading Factor: "); Serial.println(loraSpreadingFactor);
    Serial.print("Coding Rate: "); Serial.println(loraCodingRate);
  }
};

LoRaConfig config;
char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];
bool lora_idle = true;
bool receivingEnabled = true;
bool sendEnabled = true;
int counter = 0;
String logs[LOG_LINES];
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000;

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

static RadioEvents_t RadioEvents;
void OnTxDone();
void OnTxTimeout();

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
  RadioEvents.RxDone = OnRxDone;

  Radio.Init(&RadioEvents);

  Radio.SetChannel(config.rfFrequency);

  Radio.SetTxConfig(
    MODEM_LORA,
    config.txOutputPower,
    0,
    config.loraBandwidth,
    config.loraSpreadingFactor,
    config.loraCodingRate,
    8,
    false,
    false,
    0,
    0,
    false,
    3000
  );

  Radio.Rx(0); // Start in receive mode
}

void restartRadio() {
  Radio.Sleep();
  delay(100);
  radio_setup();
  lora_idle = true;
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

void OnTxDone() {
  Serial.println("Message sent.");
  lora_idle = true;
  Radio.Rx(0); // Return to receive mode after sending
}

void OnTxTimeout() {
  Radio.Sleep();
  Serial.println("Send timeout.");
  lora_idle = true;
  Radio.Rx(0); // Return to receive mode after timeout
}

void handleSerialCommand() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toLowerCase();

  if (input.startsWith("set")) {
    int sp1 = input.indexOf(' ');
    int sp2 = input.indexOf(' ', sp1 + 1);
    if (sp1 == -1 || sp2 == -1) return;

    String param = input.substring(sp1 + 1, sp2);
    uint32_t value = input.substring(sp2 + 1).toInt();

    if (param == "freq") config.rfFrequency = value;
    else if (param == "power" && value >= 2 && value <= 22) config.txOutputPower = value;
    else if (param == "bw" && value <= 2) config.loraBandwidth = value;
    else if (param == "sf" && value >= 7 && value <= 12) config.loraSpreadingFactor = value;
    else if (param == "cr" && value >= 1 && value <= 4) config.loraCodingRate = value;
    else {
      Serial.println("Invalid parameter.");
      return;
    }

    Serial.println("Configuration updated.");
    radio_setup();

  } else if (input == "pause") {
    sendEnabled = false;
    Serial.println("Transmission paused.");

  } else if (input == "resume") {
    sendEnabled = true;
    Serial.println("Transmission resumed.");

  } else if (input == "restart") {
    restartRadio();
    Serial.println("Radio restarted.");

  } else if (input == "show") {
    config.printConfig();

  } else {
    Serial.println("Unknown command.");
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
  config.printConfig();
  radio_setup();

  for (int i = 0; i < LOG_LINES; i++) logs[i] = "";
  Serial.println("LoRa transmitter ready.");
}

void loop() {
  handleSerialCommand();
  Radio.IrqProcess();

  if (sendEnabled && lora_idle && millis() - lastSendTime >= sendInterval) {
    String message = "Test " + String(counter++);
    message.toCharArray(txpacket, BUFFER_SIZE);
    Radio.Send((uint8_t *)txpacket, strlen(txpacket));
    lora_idle = false;
    lastSendTime = millis();
    logMessage("Sent: " + message + "lucas");
  }
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (!receivingEnabled) return;

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  String msg = String(rxpacket);
  logMessage("Received: " + msg + " RSSI " + rssi + " SNR " + snr);
  Radio.Rx(0); // Continue listening
}
