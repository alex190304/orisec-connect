#include "WifiMqtt.h"
#include "Log.h"
#include "OrisecUtil.h"
#include "PanelLogic.h"
#include <SPI.h>
#include <ArduinoOTA.h>

static bool wifiConfigured() { return settings.wifiSsid.length() > 0; }
static bool mqttConfigured() { return settings.mqttHost.length() > 0 && settings.mqttPort > 0; }
static bool ethernetConfigured() { return settings.useEthernet; }
static bool ethernetReady = false;
static uint32_t nextEthernetAttempt = 0;

static void selectMqttClient(Client* client) {
  if (client && netClient != client) {
    netClient = client;
    mqtt.setClient(*netClient);
  }
}

static void ethernetEnsure() {
  if (configModeActive) return;
  if (!ethernetConfigured()) return;
  if (ethernetReady && Ethernet.linkStatus() == LinkON) return;

  uint32_t now = millis();
  if (now < nextEthernetAttempt) return;
  nextEthernetAttempt = now + 5000;

  DBGLN("Starting Ethernet (W5500)...");
  SPI.begin(ETH_SCK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN, ETH_CS_PIN);
  Ethernet.init(ETH_CS_PIN);

  uint64_t chip = ESP.getEfuseMac();
  byte mac[6] = {
    (byte)(chip >> 40), (byte)(chip >> 32), (byte)(chip >> 24),
    (byte)(chip >> 16), (byte)(chip >> 8), (byte)(chip)
  };

  if (Ethernet.begin(mac) == 0) {
    DBGLN("Ethernet DHCP failed.");
    ethernetReady = false;
    return;
  }

  ethernetReady = true;
  selectMqttClient(&ethClient);
  DBGLN(String("Ethernet connected. IP=") + Ethernet.localIP().toString());
}

bool mqttPublishRetained(const String& t, const String& payload) {
  bool ok = mqtt.publish(t.c_str(), payload.c_str(), true);
#if DEBUG_SERIAL
  Serial.print("MQTT "); Serial.print(ok ? "OK  " : "FAIL"); Serial.print(" topic="); Serial.println(t);
#endif
  return ok;
}

bool mqttPublishRetainedIfChanged(String& cache, const String& t, const String& payload) {
  if (cache == payload) return true;
  bool ok = mqttPublishRetained(t, payload);
  if (ok) cache = payload;
  return ok;
}

void wifiEnsure() {
  if (configModeActive) return;
  if (settings.useEthernet) {
    ethernetEnsure();
    return;
  }
  if (!wifiConfigured()) return;
  if (WiFi.status() == WL_CONNECTED) return;

  DBGLN("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    DBG(".");
    yield();
    if (configModeActive) return;
    if (millis() - start > 20000) { DBGLN("\nWiFi connect timeout."); return; }
  }
  DBGLN("");
  DBGLN(String("WiFi connected. IP=") + WiFi.localIP().toString());
}

void mqttEnsure() {
  if (configModeActive) return;
  if (settings.useEthernet) {
    ethernetEnsure();
    if (!ethernetReady || Ethernet.linkStatus() != LinkON) return;
  } else {
    if (WiFi.status() != WL_CONNECTED) return;
    selectMqttClient(&wifiClient);
  }
  if (!mqttConfigured()) return;

  mqtt.setServer(settings.mqttHost.c_str(), settings.mqttPort);
  mqtt.setBufferSize(MQTT_BUF_SZ);
  mqtt.setCallback(mqttCallback);

  while (!mqtt.connected()) {
    String clientId = settings.deviceId + "_" + String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
    DBGLN(String("Connecting MQTT to ") + settings.mqttHost + ":" + settings.mqttPort + " ...");

    bool ok = mqtt.connect(clientId.c_str(),
                           settings.mqttUser.c_str(), settings.mqttPass.c_str(),
                           AVAIL_TOPIC, 1, true, "offline");
    if (ok) {
      DBGLN("MQTT connected.");
      mqttPublishRetained(AVAIL_TOPIC, "online");

      mqtt.subscribe(CMD_RELEARN_TOPIC);
      mqtt.subscribe(CMD_RESTART_TOPIC);

      for (int p=1;p<=MAX_PARTITIONS_HARD;p++) {
        mqtt.subscribe(topic("partition/" + String(p) + "/set").c_str());
        mqtt.subscribe(topic("partition/" + String(p) + "/reset").c_str());
      }
      for (int o=1;o<=MAX_OUTPUTS_HARD;o++) mqtt.subscribe(topic("output/" + String(o) + "/set").c_str());
    } else {
      DBGLN(String("MQTT connect failed rc=") + mqtt.state() + " retrying...");
      delay(750);
    }
    yield();
    if (configModeActive) return;
  }
}

void heartbeatLoop() {
  if (configModeActive) return;
  static uint32_t nextHb=0;
  uint32_t now=millis();
  if ((int32_t)(now-nextHb) >= 0) {
    nextHb = now + HEARTBEAT_MS;
    if (mqtt.connected()) mqttPublishRetained(AVAIL_TOPIC, "online");
  }
}

void mqttCallback(char* tpc, byte* payload, unsigned int length) {
  String tStr(tpc);
  String msg; msg.reserve(length);
  for (unsigned int i=0;i<length;i++) msg += (char)payload[i];
  msg.trim();

  if (tStr == CMD_RELEARN_TOPIC) { doRelearn = true; return; }
  if (tStr == CMD_RESTART_TOPIC) { DBGLN("Restart requested -> restarting..."); delay(200); ESP.restart(); return; }

  for (int p=1;p<=MAX_PARTITIONS_HARD;p++) {
    if (tStr == topic("partition/" + String(p) + "/reset")) { sendPanelReset(p); return; }
  }
  for (int p=1;p<=MAX_PARTITIONS_HARD;p++) {
    if (tStr == topic("partition/" + String(p) + "/set")) { handleHaAlarmCommand(p, msg); return; }
  }

  for (int o=1; o<=MAX_OUTPUTS_HARD; o++) {
    if (tStr == topic("output/" + String(o) + "/set")) {
      if (!outs[o].present) { DBGLN("[OUT] Command for non-discovered output ignored."); return; }
      String m = msg; m.toUpperCase();
      bool wantOn  = (m == "ON" || m == "1" || m == "TRUE");
      bool wantOff = (m == "OFF" || m == "0" || m == "FALSE");
      if (!wantOn && !wantOff) { DBGLN(String("[OUT] Unknown payload ignored: ") + msg); return; }

      sendRemoteOutputSet(o, wantOn);
      return;
    }
  }
}
