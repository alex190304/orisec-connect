#include "Buttons.h"
#include "ConfigStore.h"

static uint32_t lastConfigBtnChange = 0;
static bool buttonDown = false;
static uint32_t buttonDownSince = 0;
static bool shortPressPending = false;

bool configButtonPressedEvent() {
  if (!shortPressPending) return false;
  shortPressPending = false;
  return true;
}

void factoryButtonLoop() {
  bool down = (digitalRead(CONFIG_BTN_PIN) == LOW);
  uint32_t now = millis();
  if (down && !buttonDown && (now - lastConfigBtnChange) > BTN_DEBOUNCE_MS) {
    buttonDown = true;
    buttonDownSince = now;
    lastConfigBtnChange = now;
  } else if (!down && buttonDown && (now - lastConfigBtnChange) > BTN_DEBOUNCE_MS) {
    buttonDown = false;
    lastConfigBtnChange = now;
    if (buttonDownSince && (now - buttonDownSince) < FACTORY_HOLD_MS) {
      shortPressPending = true;
    }
    buttonDownSince = 0;
  }

  if (buttonDown && buttonDownSince && (now - buttonDownSince) >= FACTORY_HOLD_MS) {
    buttonDown = false;
    buttonDownSince = 0;
    shortPressPending = false;
    factoryReset();
  }
}
