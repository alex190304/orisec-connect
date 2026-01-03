#include "Leds.h"
#include "Globals.h"

static uint32_t txLedUntil = 0;
static uint32_t rxLedUntil = 0;
static uint32_t nextHeartbeat = 0;
static bool heartbeatOn = false;
static uint32_t nextConfigFlash = 0;
static bool configFlashOn = false;

void initStatusLeds() {
  pinMode(TX_LED_PIN, OUTPUT);
  pinMode(RX_LED_PIN, OUTPUT);
  pinMode(CONFIG_MODE_PIN, OUTPUT);
  pinMode(POWER_RUN_PIN, OUTPUT);
  digitalWrite(TX_LED_PIN, LOW);
  digitalWrite(RX_LED_PIN, LOW);
  digitalWrite(CONFIG_MODE_PIN, LOW);
  digitalWrite(POWER_RUN_PIN, LOW);
}

void pulseTxLed() {
  txLedUntil = millis() + 50;
  digitalWrite(TX_LED_PIN, HIGH);
}

void pulseRxLed() {
  rxLedUntil = millis() + 50;
  digitalWrite(RX_LED_PIN, HIGH);
}

void setFactoryResetActive(bool active) {
  factoryResetActive = active;
  if (active) {
    digitalWrite(POWER_RUN_PIN, HIGH);
  }
}

static void updateHeartbeat(uint32_t now) {
  if (factoryResetActive) {
    digitalWrite(POWER_RUN_PIN, HIGH);
    return;
  }
  if (now >= nextHeartbeat) {
    heartbeatOn = !heartbeatOn;
    digitalWrite(POWER_RUN_PIN, heartbeatOn ? HIGH : LOW);
    nextHeartbeat = now + (heartbeatOn ? 80 : 920);
  }
}

static void updateConfigLed(uint32_t now) {
  if (configModeActive) {
    if (now >= nextConfigFlash) {
      configFlashOn = !configFlashOn;
      digitalWrite(CONFIG_MODE_PIN, configFlashOn ? HIGH : LOW);
      nextConfigFlash = now + 250;
    }
    return;
  }

  configFlashOn = false;
  digitalWrite(CONFIG_MODE_PIN, mqtt.connected() ? HIGH : LOW);
}

void updateStatusLeds() {
  uint32_t now = millis();

  if (txLedUntil && now >= txLedUntil) {
    txLedUntil = 0;
    digitalWrite(TX_LED_PIN, LOW);
  }
  if (rxLedUntil && now >= rxLedUntil) {
    rxLedUntil = 0;
    digitalWrite(RX_LED_PIN, LOW);
  }

  updateHeartbeat(now);
  updateConfigLed(now);
}
