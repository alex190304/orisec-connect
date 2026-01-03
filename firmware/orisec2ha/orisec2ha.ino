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
#include "Leds.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(CONFIG_BTN_PIN, INPUT_PULLUP);
  initStatusLeds();

  apPassword = makeApPassword();

  DBGLN("");
  DBGLN("Orisec2HA 0.1.0 Booting...");
  DBGLN("Wiring: Panel TX->GPIO16, Panel RX<-GPIO17, GND common");
  DBGLN(String("Config button: GPIO13 to GND (short press config, hold 10s factory)"));
  DBGLN(String("Config mode output: GPIO25"));
  DBGLN(String("Power/run output: GPIO26"));
  DBGLN(String("Setup AP SSID: Orisec2HA"));
  DBGLN(String("Setup AP PASS: ") + apPassword);

  loadSettings();

  panelSerial.begin(PANEL_BAUD, SERIAL_8N1, PANEL_RX_PIN, PANEL_TX_PIN);

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
    updateStatusLeds();
    return;
  }

  wifiEnsure();
  mqttEnsure();

  mqtt.loop();
  heartbeatLoop();

  if (settings.enableIdeOta) ArduinoOTA.handle();

  pollPanelRx();

  if (doRelearn) { doRelearn=false; relearnAll(); }

  if (txLocked) {
    updateStatusLeds();
    return;
  }

  periodicPollLoop();
  updateStatusLeds();
}
