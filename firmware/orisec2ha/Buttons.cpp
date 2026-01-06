#include "Buttons.h"
#include "ConfigStore.h"

static bool lastConfigBtn = true;
static uint32_t lastConfigBtnChange = 0;

bool configButtonPressedEvent() {
  bool cur = digitalRead(CONFIG_BTN_PIN); // HIGH idle, LOW pressed
  uint32_t now = millis();
  if (cur != lastConfigBtn && (now - lastConfigBtnChange) > BTN_DEBOUNCE_MS) {
      ESP.restart();
  }
  return false;
}