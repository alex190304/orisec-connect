#include <Arduino.h>

#include "Globals.h"
#include "Log.h"
#include "ConfigStore.h"
#include "Buttons.h"
#include "Portal.h"
#include "Tables.h"
#include "WifiMqtt.h"
#include "PanelLogic.h"
#include "PanelComm.h"
#include "Ota.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(CONFIG_BTN_PIN, INPUT_PULLUP);
  pinMode(FACTORY_BTN_PIN, INPUT_PULLUP);

  apPassword = makeApPassword();

  DBGLN("");
  DBGLN("Orisec2HA 0.1.0 Booting...");
  DBGLN("Wiring: Panel TX->D7, Panel RX<-D4, GND common");
  DBGLN(String("Config button: D5 (GPIO14) to GND (short press)"));
  DBGLN(String("Factory button: D6 (GPIO12) to GND (hold 10s)"));
  DBGLN(String("Setup AP SSID: Orisec2HA"));
  DBGLN(String("Setup AP PASS: ") + apPassword);

  loadSettings();

  panelRx.begin(PANEL_BAUD);
  Serial1.begin(PANEL_BAUD);

  initTables();

  if (settings.enableIdeOta) setupArduinoIdeOta();

  wifiEnsure();
  mqttEnsure();

  if (mqtt.connected()) {
    initialDiscoveryAndPoll();
  } else {
    DBGLN("MQTT not connected (missing settings or WiFi). Press config button to set.");
  }

  DBGLN("Setup complete.");
}

void loop() {
  factoryButtonLoop();

  if (configButtonPressedEvent()) startConfigPortal();

  if (configModeActive) {
    configPortalLoop();
    pollPanelRx(); // keep RX alive, TX blocked
    return;
  }

  wifiEnsure();
  mqttEnsure();

  mqtt.loop();
  heartbeatLoop();

  if (settings.enableIdeOta) ArduinoOTA.handle();

  pollPanelRx();

  if (doRelearn) { doRelearn=false; relearnAll(); }

  if (txLocked) return;

  periodicPollLoop();
}
