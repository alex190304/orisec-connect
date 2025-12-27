#include "Buttons.h"
#include "ConfigStore.h"

static bool lastConfigBtn = true;
static uint32_t lastConfigBtnChange = 0;

bool configButtonPressedEvent() {
  bool cur = digitalRead(CONFIG_BTN_PIN); // HIGH idle, LOW pressed
  uint32_t now = millis();
  if (cur != lastConfigBtn && (now - lastConfigBtnChange) > BTN_DEBOUNCE_MS) {
    lastConfigBtnChange = now;
    lastConfigBtn = cur;
    if (cur == LOW) return true;
  }
  return false;
}

static bool factoryBtnWasDown = false;
static uint32_t factoryDownSince = 0;

void factoryButtonLoop() {
  bool down = (digitalRead(FACTORY_BTN_PIN) == LOW);
  uint32_t now = millis();
  if (down && !factoryBtnWasDown) {
    factoryBtnWasDown = true;
    factoryDownSince = now;
  } else if (!down && factoryBtnWasDown) {
    factoryBtnWasDown = false;
    factoryDownSince = 0;
  } else if (down && factoryBtnWasDown) {
    if (factoryDownSince && (now - factoryDownSince) >= FACTORY_HOLD_MS) {
      factoryReset();
    }
  }
}
