#include <LoRaWan_APP.h>
#include "Arduino.h"
#include "HT_SSD1306Wire.h"

#define Vext 21
#define LOG_LINES 6
#define MAX_LINE_LEN 21  // ≈ 128px / 6px per OLED line

extern SSD1306Wire display;

bool lora_idle = true;
String logs[LOG_LINES];

uint8_t* receivedBuffer = nullptr;
uint16_t receivedSize = 0;
int16_t lastRSSI = 0;
int8_t lastSNR = 0;
bool receivedFlag = false;

static RadioEvents_t RadioEvents;

// ---------- OLED ----------
void display_init() {
    display.init();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
}

void logHexMultiline(const String& full) {
    int start = 0;
    while (start < full.length()) {
        String line = full.substring(start, start + MAX_LINE_LEN);
        for (int i = 0; i < LOG_LINES - 1; i++) logs[i] = logs[i + 1];
        logs[LOG_LINES - 1] = line;
        start += MAX_LINE_LEN;
    }
    display.clear();
    for (int i = 0; i < LOG_LINES; i++)
        display.drawString(0, i * 10, logs[i]);
    display.display();
}

void VextON() {
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
}

// ---------- RADIO CALLBACKS ----------
void OnTxDone() {
    logHexMultiline("TxDone");
    lora_idle = true;
    Radio.Rx(0);
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (receivedBuffer) free(receivedBuffer);
    receivedBuffer = (uint8_t*)malloc(size);
    if (!receivedBuffer) return;
    memcpy(receivedBuffer, payload, size);
    receivedSize = size;
    lastRSSI = rssi;
    lastSNR = snr;
    receivedFlag = true;
}

void OnTxTimeout() {
    logHexMultiline("TxTimeout");
    lora_idle = true;
    Radio.Rx(0);
}

// ---------- SETUP ----------
void setup() {
    Serial.begin(115200);
    delay(100);
    VextON();
    display_init();
    delay(100);
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxDone = OnRxDone;
    Radio.Init(&RadioEvents);

    // ---- LORA PARAMETERS ----
    uint32_t freq = 434000000; // 434 MHz
    uint8_t bw = 0; // 0 = 125 kHz
    uint8_t sf = 9;
    uint8_t cr = 3; // 1=4/5, 2=4/6, 3=4/7, 4=4/8

    Radio.SetChannel(freq);
    Radio.SetTxConfig(
        MODEM_LORA,
        22,       // Power in dBm
        0,        // FSK deviation (not used)
        bw,       // Bandwidth (0 = 125 kHz)
        sf,       // Spreading Factor
        cr,       // Coding Rate
        8,        // Preamble length
        false, false, false,
        0, false, 3000
    );
    Radio.Rx(0);
    logHexMultiline("READY 434MHz");
}

// ---------- LOOP ----------
void loop() {
    Radio.IrqProcess();

    // SEND: As soon as a line is entered on Serial (USB)
    if (lora_idle && Serial.available()) {
        String line = Serial.readStringUntil('\n');
        if (line.length() > 0) {
            // Show sent hexadecimal on OLED
            String txHex = "";
            for (uint16_t i = 0; i < line.length(); i++) {
                uint8_t b = line[i];
                if (b < 16) txHex += "0";
                txHex += String(b, HEX);
                txHex += " ";
            }
            logHexMultiline("TX:");
            logHexMultiline(txHex);

            // Send the frame over radio
            lora_idle = false;
            Radio.Send((uint8_t*)line.c_str(), line.length());
            logHexMultiline("Sending...");
        }
    }

    // RECEIVE: When a LoRa frame arrives
    if (receivedFlag) {
        // 1. OLED: Show RSSI/SNR and content in HEX
        String info = "RX " + String(lastRSSI) + "dBm/" + String(lastSNR);
        logHexMultiline(info);

        String hexString = "";
        for (uint16_t i = 0; i < receivedSize; i++) {
            uint8_t val = receivedBuffer[i];
            if (val < 16) hexString += "0";
            hexString += String(val, HEX);
            hexString += " ";
        }
        logHexMultiline(hexString);

        // 2. SERIAL: Send raw received binary buffer
        Serial.write(receivedBuffer, receivedSize);

        free(receivedBuffer);
        receivedBuffer = nullptr;
        receivedSize = 0;
        receivedFlag = false;
    }
}
