#include "HADiscovery.h"
#include "WifiMqtt.h"
#include "OrisecUtil.h"

String haDeviceBlock() {
  String model = panelModel;
  if (panelVersion.length()) model += " " + panelVersion;

  String payload = "\"device\":{";
  payload += "\"identifiers\":[\"" + settings.deviceId + "\"";
  if (panelSerialStr.length()) payload += ",\"" + jsonEscape(panelSerialStr) + "\"";
  payload += "],";
  payload += "\"name\":\"" + jsonEscape(settings.deviceName) + "\",";
  payload += "\"manufacturer\":\"Orisec\",";
  payload += "\"model\":\"" + jsonEscape(model) + "\"";
  payload += "}";
  return payload;
}

void publishDiscoveryButton(const String& objectId, const String& name, const String& commandTopic, const String& icon) {
  String cfgTopic = String(DISCOVERY_PREFIX) + "/button/" + settings.deviceId + "/" + objectId + "/config";
  String payload = "{";
  payload += "\"name\":\"" + jsonEscape(name) + "\",";
  payload += "\"unique_id\":\"" + jsonEscape(settings.deviceId + "_" + objectId) + "\",";
  payload += "\"command_topic\":\"" + jsonEscape(commandTopic) + "\",";
  payload += "\"payload_press\":\"PRESS\",";
  payload += "\"availability_topic\":\"" + String(AVAIL_TOPIC) + "\",";
  if (icon.length()) payload += "\"icon\":\"" + jsonEscape(icon) + "\",";
  payload += haDeviceBlock();
  payload += "}";
  mqttPublishRetained(cfgTopic, payload);
}

void publishDiscoverySensor(const String& objectId, const String& name, const String& stateTopicStr,
                            const String& icon, const String& unit, const String& deviceClass) {
  String cfgTopic = String(DISCOVERY_PREFIX) + "/sensor/" + settings.deviceId + "/" + objectId + "/config";
  String payload = "{";
  payload += "\"name\":\"" + jsonEscape(name) + "\",";
  payload += "\"unique_id\":\"" + jsonEscape(settings.deviceId + "_" + objectId) + "\",";
  payload += "\"state_topic\":\"" + jsonEscape(stateTopicStr) + "\",";
  payload += "\"availability_topic\":\"" + String(AVAIL_TOPIC) + "\",";
  payload += "\"suggested_display_precision\":1,";
  if (icon.length()) payload += "\"icon\":\"" + jsonEscape(icon) + "\",";
  if (unit.length()) payload += "\"unit_of_measurement\":\"" + jsonEscape(unit) + "\",";
  if (deviceClass.length()) payload += "\"device_class\":\"" + jsonEscape(deviceClass) + "\",";
  payload += haDeviceBlock();
  payload += "}";
  mqttPublishRetained(cfgTopic, payload);
}

void publishDiscoverySwitch(const String& objectId,
                            const String& name,
                            const String& stateTopicStr,
                            const String& commandTopicStr,
                            const String& icon) {
  String cfgTopic = String(DISCOVERY_PREFIX) + "/switch/" + settings.deviceId + "/" + objectId + "/config";

  String payload = "{";
  payload += "\"name\":\"" + jsonEscape(name) + "\",";
  payload += "\"unique_id\":\"" + jsonEscape(settings.deviceId + "_" + objectId) + "\",";
  payload += "\"state_topic\":\"" + jsonEscape(stateTopicStr) + "\",";
  payload += "\"command_topic\":\"" + jsonEscape(commandTopicStr) + "\",";
  payload += "\"availability_topic\":\"" + String(AVAIL_TOPIC) + "\",";
  payload += "\"payload_on\":\"ON\",";
  payload += "\"payload_off\":\"OFF\",";
  payload += "\"state_on\":\"ON\",";
  payload += "\"state_off\":\"OFF\",";
  if (icon.length()) payload += "\"icon\":\"" + jsonEscape(icon) + "\",";
  payload += haDeviceBlock();
  payload += "}";
  mqttPublishRetained(cfgTopic, payload);
}

void publishDiscoveryAlarmPanel(int p, const String& name) {
  String objectId = "orisec_area_" + String(p) + "_" + sanitizeForObjectId(name);
  String cfgTopic = String(DISCOVERY_PREFIX) + "/alarm_control_panel/" + settings.deviceId + "/" + objectId + "/config";

  String stateT = topic("partition/" + String(p) + "/alarm_state");
  String cmdT   = topic("partition/" + String(p) + "/set");
  String attrT  = topic("partition/" + String(p) + "/alarm_attr");

  bool haveCode = settings.haAlarmCode.length() > 0;
  bool requireCode = settings.requireHaCodeForArmDisarm && haveCode;

  String payload = "{";
  payload += "\"name\":\"" + jsonEscape(name) + "\",";
  payload += "\"unique_id\":\"" + jsonEscape(settings.deviceId + "_alarm_p" + String(p)) + "\",";
  payload += "\"availability_topic\":\"" + String(AVAIL_TOPIC) + "\",";
  payload += "\"state_topic\":\"" + jsonEscape(stateT) + "\",";
  payload += "\"command_topic\":\"" + jsonEscape(cmdT) + "\",";
  payload += "\"supported_features\":[\"arm_home\",\"arm_away\",\"arm_night\",\"arm_custom_bypass\"],";

  payload += "\"payload_arm_home\":\"ARM_HOME\",";
  payload += "\"payload_arm_away\":\"ARM_AWAY\",";
  payload += "\"payload_arm_night\":\"ARM_NIGHT\",";
  payload += "\"payload_arm_custom_bypass\":\"ARM_CUSTOM_BYPASS\",";
  payload += "\"payload_disarm\":\"DISARM\",";

  if (haveCode && requireCode) {
    payload += "\"code\":\"" + jsonEscape(settings.haAlarmCode) + "\",";
  }
  payload += String("\"code_arm_required\":") + (requireCode ? "true" : "false") + ",";
  payload += String("\"code_disarm_required\":") + (requireCode ? "true" : "false") + ",";

  payload += haDeviceBlock();
  payload += "}";

  mqttPublishRetained(cfgTopic, payload);

  mqttPublishRetainedIfChanged(lastPartAlarmState[p], stateT, "unknown");

  String btnObjId = "reset_area_" + String(p) + "_" + sanitizeForObjectId(name);
  String btnName  = "Reset " + name;
  publishDiscoveryButton(btnObjId, btnName, topic("partition/" + String(p) + "/reset"), "mdi:backup-restore");
}

void publishCoreDiscovery() {
  publishDiscoverySensor("volt_0", "Panel Battery", topic("voltage/0"), "mdi:car-battery", "V", "voltage");
  publishDiscoverySensor("volt_1", "Panel Output 1", topic("voltage/1"), "mdi:flash", "V", "voltage");
  publishDiscoverySensor("volt_2", "Panel Output 2", topic("voltage/2"), "mdi:flash", "V", "voltage");

  publishDiscoveryButton("relearn", "Relearn zones/areas", CMD_RELEARN_TOPIC, "mdi:refresh");
  publishDiscoveryButton("restart", "Restart module", CMD_RESTART_TOPIC, "mdi:restart");
}

void publishZoneDiscovery(int z) {
  String name = zones[z].label.length() ? zones[z].label : ("Zone " + String(z));
  String objectId = "zone_" + String(z) + "_" + sanitizeForObjectId(name);
  publishDiscoverySensor(objectId, name, topic("zone/" + String(z) + "/state"), "mdi:shield-home");
}

void publishOutputDiscovery(int o) {
  String name = outs[o].label;
  String objectId = "output_" + String(o) + "_" + sanitizeForObjectId(name);
  publishDiscoverySwitch(
    objectId,
    name,
    topic("output/" + String(o) + "/state"),
    topic("output/" + String(o) + "/set"),
    "mdi:toggle-switch"
  );
}
