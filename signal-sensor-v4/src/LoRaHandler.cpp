#include "LoRaHandler.h"
#include "Secrets.h"

#include <SPI.h>
#include <RadioLib.h>

// Heltec WiFi LoRa 32 V4 – SX1262 hardware connections
//   NSS  = GPIO8   (SS in pins_arduino.h)
//   DIO1 = GPIO14  (interrupt line – Heltec labels it DIO0 in silkscreen)
//   RST  = GPIO12  (RST_LoRa)
//   BUSY = GPIO13  (BUSY_LoRa)
//   SPI  : SCK=9, MISO=11, MOSI=10
static SX1262 radio = new Module(8, 14, 12, 13);

// EU868 regional parameters (change to e.g. US915 if needed)
static LoRaWANNode node(&radio, &EU868);

static bool     loraRadioOk      = false;
static bool     loraJoined       = false;
static uint32_t lastLoraTxMs     = 0;
static uint32_t lastJoinAttemptMs = 0;

// Minimum gap between uplinks – respect TTN Fair Use Policy (≤30 s airtime/day)
static constexpr uint32_t LORA_TX_INTERVAL_MS   = 60000UL; // 60 s
// Back-off between join retries – TTN enforces a join rate limit
static constexpr uint32_t LORA_JOIN_RETRY_MS    = 30000UL; // 30 s

// Parse a hex string (e.g. "DA2975C24D40A06D2E65203AB9EA9076") into raw bytes.
// 'hex' must contain exactly len*2 ASCII hex characters.
static void hexToBytes(const char* hex, uint8_t* out, size_t len) {
    for (size_t i = 0; i < len; i++) {
        auto nibble = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
            if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
            if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
            return 0;
        };
        out[i] = (uint8_t)((nibble(hex[i * 2]) << 4) | nibble(hex[i * 2 + 1]));
    }
}

// Parse a 16-character hex string into a uint64_t (MSB first, as TTN shows it).
static uint64_t hexToU64(const char* hex) {
    uint64_t result = 0;
    for (int i = 0; i < 16; i++) {
        char c = hex[i];
        uint8_t nibble;
        if      (c >= '0' && c <= '9') nibble = (uint8_t)(c - '0');
        else if (c >= 'A' && c <= 'F') nibble = (uint8_t)(c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') nibble = (uint8_t)(c - 'a' + 10);
        else                           nibble = 0;
        result = (result << 4) | nibble;
    }
    return result;
}

static void tryJoinOTAA() {
    Serial.println("[LoRa] Joining TTN via OTAA …");
    int16_t state = node.activateOTAA();
    lastJoinAttemptMs = millis();
    if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_LORAWAN_SESSION_RESTORED) {
        loraJoined = true;
        Serial.println("[LoRa] Joined TTN!");
    } else {
        Serial.printf("[LoRa] Join failed: %d — will retry in %lu s\n", state, LORA_JOIN_RETRY_MS / 1000);
    }
}

void initLoRa() {
    // Initialise SPI bus with the V4 LoRa radio pins
    SPI.begin(9 /*SCK*/, 11 /*MISO*/, 10 /*MOSI*/, 8 /*SS*/);

    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Radio init failed: %d\n", state);
        return;
    }
    loraRadioOk = true;

    // Derive OTAA credentials from Secrets.h
    // LORA_APPEUI / LORA_DEVEUI are 16-char hex strings (8 bytes each, MSB)
    // LORA_APPKEY is a 32-char hex string (16 bytes)
    uint64_t joinEUI = hexToU64(LORA_APPEUI);
    uint64_t devEUI  = hexToU64(LORA_DEVEUI);

    // LoRaWAN 1.0.x: there is one root key (AppKey); RadioLib expects both
    // nwkKey and appKey, so pass the same value for both.
    uint8_t appKey[16];
    uint8_t nwkKey[16];
    hexToBytes(LORA_APPKEY, appKey, 16);
    hexToBytes(LORA_APPKEY, nwkKey, 16);

    node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

    tryJoinOTAA();
}

void processLoRa(float avgMv, float dominantHz, float sampleRateHz) {
    if (!loraRadioOk) return;

    uint32_t now = millis();

    if (!loraJoined) {
        if (now - lastJoinAttemptMs >= LORA_JOIN_RETRY_MS) {
            tryJoinOTAA();
        }
        return;
    }

    if (now - lastLoraTxMs < LORA_TX_INTERVAL_MS) return;
    lastLoraTxMs = now;

    // 6-byte payload (big-endian uint16_t each):
    //   [0-1] dominant frequency in units of 0.1 Hz  (range: 0–6553.5 Hz)
    //   [2-3] average signal level in mV             (range: 0–65535 mV)
    //   [4-5] ADC sample rate in Hz                  (range: 0–65535 Hz)
    uint16_t freqEnc = (uint16_t)(dominantHz * 10.0f);
    uint16_t avgEnc  = (uint16_t)(avgMv);
    uint16_t srEnc   = (uint16_t)(sampleRateHz);

    uint8_t payload[6];
    payload[0] = (freqEnc >> 8) & 0xFF;
    payload[1] =  freqEnc       & 0xFF;
    payload[2] = (avgEnc  >> 8) & 0xFF;
    payload[3] =  avgEnc        & 0xFF;
    payload[4] = (srEnc   >> 8) & 0xFF;
    payload[5] =  srEnc         & 0xFF;

    int16_t state = node.sendReceive(payload, sizeof(payload));
    if (state >= RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Uplink sent (downlink: %s)\n", state > 0 ? "yes" : "no");
    } else {
        Serial.printf("[LoRa] Send failed: %d\n", state);
    }
}
