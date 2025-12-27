#include "ConfigStore.h"
#include "Log.h"

static String getKV(const String& line, String& keyOut) {
  int eq = line.indexOf('=');
  if (eq < 0) { keyOut=""; return ""; }
  keyOut = line.substring(0, eq);
  keyOut.trim();
  String val = line.substring(eq + 1);
  val.trim();
  return val;
}

static void setDefaultSettingsIfNeeded() {
  if (settings.deviceName.length() == 0) settings.deviceName = "orisec-bridge";
  if (settings.deviceId.length() == 0) settings.deviceId = "orisec_bridge_1";
  if (settings.mqttPort == 0) settings.mqttPort = 1883;
}

bool loadSettings() {
  if (!LittleFS.begin()) {
    DBGLN("LittleFS mount failed");
    return false;
  }
  if (!LittleFS.exists(CFG_FILE)) {
    DBGLN("No config file found (defaults).");
    setDefaultSettingsIfNeeded();
    return false;
  }

  File f = LittleFS.open(CFG_FILE, "r");
  if (!f) {
    DBGLN("Failed opening config file.");
    setDefaultSettingsIfNeeded();
    return false;
  }

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length() || line.startsWith("#")) continue;

    String k;
    String v = getKV(line, k);
    if (!k.length()) continue;

    if (k == "wifiSsid") settings.wifiSsid = v;
    else if (k == "wifiPass") settings.wifiPass = v;

    else if (k == "mqttHost") settings.mqttHost = v;
    else if (k == "mqttPort") settings.mqttPort = (uint16_t) v.toInt();
    else if (k == "mqttUser") settings.mqttUser = v;
    else if (k == "mqttPass") settings.mqttPass = v;

    else if (k == "haAlarmCode") settings.haAlarmCode = v;
    else if (k == "deviceName") settings.deviceName = v;
    else if (k == "deviceId") settings.deviceId = v;

    else if (k == "panelUserCode") settings.panelUserCode = v;

    else if (k == "enableRemoteArming") settings.enableRemoteArming = (v == "1" || v == "true" || v == "on");
    else if (k == "enableRemoteDisarming") settings.enableRemoteDisarming = (v == "1" || v == "true" || v == "on");
    else if (k == "requireHaCodeForArmDisarm") settings.requireHaCodeForArmDisarm = (v == "1" || v == "true" || v == "on");
    else if (k == "enableIdeOta") settings.enableIdeOta = (v == "1" || v == "true" || v == "on");
    else if (k == "otaPass") settings.otaPass = v;
  }
  f.close();

  setDefaultSettingsIfNeeded();
  DBGLN("Settings loaded.");
  return true;
}

bool saveSettings(const Settings& s) {
  if (!LittleFS.begin()) return false;

  File f = LittleFS.open(CFG_FILE, "w");
  if (!f) return false;

  f.println("# Orisec bridge config (key=value)");
  f.print("wifiSsid="); f.println(s.wifiSsid);
  f.print("wifiPass="); f.println(s.wifiPass);

  f.print("mqttHost="); f.println(s.mqttHost);
  f.print("mqttPort="); f.println(String(s.mqttPort));
  f.print("mqttUser="); f.println(s.mqttUser);
  f.print("mqttPass="); f.println(s.mqttPass);

  f.print("haAlarmCode="); f.println(s.haAlarmCode);
  f.print("deviceName="); f.println(s.deviceName);
  f.print("deviceId="); f.println(s.deviceId);

  f.print("panelUserCode="); f.println(s.panelUserCode);

  f.print("enableRemoteArming="); f.println(s.enableRemoteArming ? "1" : "0");
  f.print("enableRemoteDisarming="); f.println(s.enableRemoteDisarming ? "1" : "0");
  f.print("requireHaCodeForArmDisarm="); f.println(s.requireHaCodeForArmDisarm ? "1" : "0");

  f.print("enableIdeOta="); f.println(s.enableIdeOta ? "1" : "0");
  f.print("otaPass="); f.println(s.otaPass);

  f.close();
  return true;
}

void factoryReset() {
  DBGLN("FACTORY RESET: deleting config and rebooting...");
  if (LittleFS.begin()) {
    if (LittleFS.exists(CFG_FILE)) LittleFS.remove(CFG_FILE);
  }
  delay(250);
  ESP.restart();
}
