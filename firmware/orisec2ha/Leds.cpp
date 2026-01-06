#include "Leds.h"
#include "Log.h"
#include "Globals.h"

// If your LED is wired to 3V3->LED->resistor->GPIO (so GPIO sinks current),
// or you're using an onboard LED that's active-low, set this to 1.
#define CONFIG_LED_ACTIVE_LOW 0

static uint32_t txLedUntil = 0;
static uint32_t rxLedUntil = 0;

static bool lastConfigOut = false;
static bool lastPowerOut  = false;
static bool lastTxOut     = false;
static bool lastRxOut     = false;

static inline void writePinIfChanged(uint8_t pin, bool &last, bool level) {
  if (last == level) return;
  last = level;
  digitalWrite(pin, level ? HIGH : LOW);
}

void initStatusLeds() {
  pinMode(TX_LED_PIN, OUTPUT);
  pinMode(RX_LED_PIN, OUTPUT);
  pinMode(CONFIG_MODE_PIN, OUTPUT);
  pinMode(POWER_RUN_PIN, OUTPUT);

  // start off
  digitalWrite(TX_LED_PIN, LOW);
  digitalWrite(RX_LED_PIN, LOW);
  digitalWrite(CONFIG_MODE_PIN, LOW);
  digitalWrite(POWER_RUN_PIN, LOW);

  lastTxOut = false;
  lastRxOut = false;
  lastConfigOut = false;
  lastPowerOut = false;
}

void pulseTxLed() {
  txLedUntil = millis() + 50;
  writePinIfChanged(TX_LED_PIN, lastTxOut, true);
}

void pulseRxLed() {
  rxLedUntil = millis() + 50;
  writePinIfChanged(RX_LED_PIN, lastRxOut, true);
}

// Simple heartbeat on POWER_RUN_PIN (optional)
// If you want it solid ON always, just replace with writePinIfChanged(..., true)
static void updateHeartbeat(uint32_t now) {
  // 100ms ON every 1s
  bool hb = ((now % 1000) < 100);
  writePinIfChanged(POWER_RUN_PIN, lastPowerOut, hb);
}

static void updateConfigLed(uint32_t now) {
  // Detect transitions into/out of config mode
  bool desired;

  if (configModeActive) {
    // 100ms ON every 1s
    bool cm = ((now % 1000) < 100);
    writePinIfChanged(CONFIG_MODE_PIN, lastConfigOut, cm); 
    } else {
    // solid reflect MQTT
    desired = mqtt.connected();
    writePinIfChanged(CONFIG_MODE_PIN, lastConfigOut, desired);
  }
}


void updateStatusLeds() {
  uint32_t now = millis();

  // TX/RX pulse timeout
  if (txLedUntil && (int32_t)(now - txLedUntil) >= 0) {
    txLedUntil = 0;
    writePinIfChanged(TX_LED_PIN, lastTxOut, false);
  }
  if (rxLedUntil && (int32_t)(now - rxLedUntil) >= 0) {
    rxLedUntil = 0;
    writePinIfChanged(RX_LED_PIN, lastRxOut, false);
  }

  updateConfigLed(now);
  updateHeartbeat(now);
}
