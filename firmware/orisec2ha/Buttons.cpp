#include "Buttons.h"
#include "ConfigStore.h"

static bool lastReading = true;
static bool stableState = true;
static uint32_t lastReadingChange = 0;
static uint32_t buttonDownSince = 0;
static bool shortPressPending = false;

bool configButtonPressedEvent() {
  if (!shortPressPending) return false;
  shortPressPending = false;
  return true;
}

void factoryButtonLoop() {
  uint32_t now = millis();
  bool reading = (digitalRead(CONFIG_BTN_PIN) == LOW);

  if (reading != lastReading) {
    lastReading = reading;
    lastReadingChange = now;
  }

  if ((now - lastReadingChange) > BTN_DEBOUNCE_MS && stableState != lastReading) {
    stableState = lastReading;
    if (stableState) {
      buttonDownSince = now;
    } else {
      if (buttonDownSince && (now - buttonDownSince) < FACTORY_HOLD_MS) {
        shortPressPending = true;
      }
      buttonDownSince = 0;
    }
  }

  if (stableState && buttonDownSince && (now - buttonDownSince) >= FACTORY_HOLD_MS) {
    buttonDownSince = 0;
    shortPressPending = false;
    factoryReset();
  }
}
