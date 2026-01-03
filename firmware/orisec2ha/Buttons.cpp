#include "Buttons.h"
#include "ConfigStore.h"

static bool lastConfigBtn = false;
static uint32_t lastConfigBtnChange = 0;
static bool buttonDown = false;
static uint32_t buttonDownSince = 0;
static bool longPressHandled = false;
static bool shortPressPending = false;

bool configButtonPressedEvent() {
  if (!shortPressPending) return false;
  shortPressPending = false;
  return true;
}

void factoryButtonLoop() {
  bool down = (digitalRead(CONFIG_BTN_PIN) == LOW);
  uint32_t now = millis();
  if (down != lastConfigBtn && (now - lastConfigBtnChange) > BTN_DEBOUNCE_MS) {
    lastConfigBtnChange = now;
    lastConfigBtn = down;
    if (down) {
      buttonDown = true;
      buttonDownSince = now;
      longPressHandled = false;
    } else {
      if (buttonDown && !longPressHandled && buttonDownSince && (now - buttonDownSince) < FACTORY_HOLD_MS) {
        shortPressPending = true;
      }
      buttonDown = false;
      buttonDownSince = 0;
      longPressHandled = false;
    }
    buttonDownSince = 0;
  }

  if (buttonDown && buttonDownSince && (now - buttonDownSince) >= FACTORY_HOLD_MS) {
    buttonDown = false;
    buttonDownSince = 0;
    shortPressPending = false;
    factoryReset();
  }

  if (buttonDown && !longPressHandled && buttonDownSince && (now - buttonDownSince) >= FACTORY_HOLD_MS) {
    longPressHandled = true;
    factoryReset();
  }

  if (stableState && buttonDownSince && (now - buttonDownSince) >= FACTORY_HOLD_MS) {
    buttonDownSince = 0;
    shortPressPending = false;
    factoryReset();
  }

  if (stableState && buttonDownSince && (now - buttonDownSince) >= FACTORY_HOLD_MS) {
    buttonDownSince = 0;
    shortPressPending = false;
    factoryReset();
  }

  if (stableState && buttonDownSince && (now - buttonDownSince) >= FACTORY_HOLD_MS) {
    buttonDownSince = 0;
    shortPressPending = false;
    factoryReset();
  }
}
