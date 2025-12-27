#include "PanelLogic.h"
#include "Log.h"
#include "PanelComm.h"
#include "Parsers.h"
#include "HADiscovery.h"
#include "WifiMqtt.h"
#include "OrisecUtil.h"
#include "Tables.h"

void probePanelVersionAndSerial() {
  if (requestWithRetry("GETVER", "GETVER:", 1200, true)) {
    parseGETVER();
    zoneLimit = deriveZoneLimitFromModel(panelModel);
    DBGLN(String("Panel model=") + panelModel + " version=" + panelVersion + " zoneLimit=" + zoneLimit);
  } else DBGLN("GETVER failed");

  if (requestWithRetry("GETSERIAL", "GETSERIAL:", 1200)) {
    parseGETSERIAL();
    DBGLN(String("Panel serial=") + panelSerial);
  } else DBGLN("GETSERIAL failed");
}

static bool snapshotBootGETZ() {
  for (int i=1;i<=MAX_ZONES_HARD;i++) bootZoneStatus[i]=0x80;

  if (!requestWithRetry("GETZ", "GETZ:", 1500)) return false;
  if (lastFrameLen < 5 + 1 + 1) return false;

  const size_t prefix=5;
  size_t zoneBytesAvail = lastFrameLen - prefix - 1;
  size_t n = (size_t)zoneLimit;
  if (n > zoneBytesAvail) n = zoneBytesAvail;

  for (size_t i=0;i<n;i++) bootZoneStatus[i+1] = lastFrame[prefix+i];
  return true;
}

void discoverPartitions() {
  DBGLN("Discover partitions (GETP count + PINFO labels) ...");

  if (requestWithRetry("GETP", "GETP:", 1200)) {
    String st;
    if (parseGETP(st)) {
      partitionCount = (int)st.length();
      if (partitionCount > MAX_PARTITIONS_HARD) partitionCount = MAX_PARTITIONS_HARD;
    }
  }
  if (partitionCount <= 0) { DBGLN("No partitions found."); return; }

  for (int p=1;p<=partitionCount;p++) {
    String label="";
    if (requestWithRetry("PINFO:" + String(p), "PINFO:", 1200)) parsePINFO(label);
    parts[p].present=true;
    parts[p].label=label;

    String name = label.length() ? label : ("Area " + String(p));
    publishDiscoveryAlarmPanel(p, name);
  }
}

void discoverZones() {
  DBGLN("Discover zones ...");
  if (!snapshotBootGETZ()) { DBGLN("GETZ snapshot failed"); return; }

  for (int z=1; z<=zoneLimit; z++) {
    if (!zoneProgrammed(bootZoneStatus[z])) continue;

    String label="";
    if (requestWithRetry("ZINFO:" + String(z), "ZINFO:", 1200)) parseZINFO(label);
    zones[z].present=true;
    zones[z].label=label;

    publishZoneDiscovery(z);
    mqttPublishRetainedIfChanged(lastZoneState[z], topic("zone/" + String(z) + "/state"),
                                 zoneStateFromByte(bootZoneStatus[z]));
  }
}

void discoverOutputs() {
  DBGLN("Discover outputs (ROPS count + OINFO labels) ...");
  if (!requestWithRetry("ROPS", "ROPS:", 1500)) return;

  String bits;
  if (!parseROPS(bits)) return;

  outputCount = (int)bits.length();
  if (outputCount > MAX_OUTPUTS_HARD) outputCount = MAX_OUTPUTS_HARD;

  for (int o=1; o<=outputCount; o++) {
    String label="";
    if (requestWithRetry("OINFO:" + String(o), "OINFO:", 1200)) {
      if (!parseOINFO(label)) label="";
    }
    if (!label.length()) {
      outs[o].present = false;
      outs[o].label = "";
      continue;
    }

    outs[o].present = true;
    outs[o].label = label;

    publishOutputDiscovery(o);

    String st = (o <= (int)bits.length() && bits[o-1]=='1') ? "ON" : "OFF";
    mqttPublishRetainedIfChanged(lastOutState[o], topic("output/" + String(o) + "/state"), st);
  }
}

void pollGETZ() {
  if (!requestWithRetry("GETZ", "GETZ:", 1500)) { DBGLN("GETZ poll: no response"); return; }
  if (lastFrameLen < 5 + 1 + 1) return;

  const size_t prefix=5;
  size_t zoneBytesAvail = lastFrameLen - prefix - 1;
  size_t maxProcess = (size_t)zoneLimit;
  if (maxProcess > zoneBytesAvail) maxProcess = zoneBytesAvail;

  for (size_t i=0;i<maxProcess;i++) {
    int z = (int)i + 1;
    if (!zones[z].present) continue;
    uint8_t zb = lastFrame[prefix+i];
    if (!zoneProgrammed(zb)) continue;
    mqttPublishRetainedIfChanged(lastZoneState[z], topic("zone/" + String(z) + "/state"), zoneStateFromByte(zb));
  }
}

void pollGETP() {
  if (!requestWithRetry("GETP", "GETP:", 1200)) { DBGLN("GETP poll: no response"); return; }
  String st;
  if (!parseGETP(st)) return;

  int lim = min((int)st.length(), partitionCount);
  for (int p=1;p<=lim;p++) {
    if (!parts[p].present) continue;
    mqttPublishRetainedIfChanged(lastPartAlarmState[p],
                                topic("partition/" + String(p) + "/alarm_state"),
                                partCharToHAAlarmState(st[p-1]));
  }
}

void pollROPS() {
  if (!requestWithRetry("ROPS", "ROPS:", 1500)) { DBGLN("ROPS poll: no response"); return; }
  String bits;
  if (!parseROPS(bits)) return;

  int count = min((int)bits.length(), outputCount);
  for (int o=1; o<=count; o++) {
    if (!outs[o].present) continue;
    String st = (bits[o-1] == '1') ? "ON" : "OFF";
    mqttPublishRetainedIfChanged(lastOutState[o], topic("output/" + String(o) + "/state"), st);
  }
}

static String fmt1dp(const String& s) {
  float f = s.toFloat();
  return String(f, 1);
}

void pollGETSYS() {
  if (!requestWithRetry("GETSYS", "GETSYS:", 1500)) { DBGLN("GETSYS poll: no response"); return; }
  String v0,v1,v2;
  if (!parseGETSYS(v0,v1,v2)) return;

  String o0 = fmt1dp(v0);
  String o1 = fmt1dp(v1);
  String o2 = fmt1dp(v2);

  mqttPublishRetainedIfChanged(lastVolt0, topic("voltage/0"), o0);
  mqttPublishRetainedIfChanged(lastVolt1, topic("voltage/1"), o1);
  mqttPublishRetainedIfChanged(lastVolt2, topic("voltage/2"), o2);
}

void relearnAll() {
  DBGLN("=== RELEARN START ===");
  initTables();

  probePanelVersionAndSerial();
  publishCoreDiscovery();

  discoverPartitions();
  discoverZones();
  discoverOutputs();

  pollGETSYS();
  pollGETP();
  pollROPS();
  pollGETZ();

  DBGLN("=== RELEARN DONE ===");
}

static inline void sendPanelArmAway(int partition) {
  String cmd = String("ARM:") + partition;
  bool ok = sendCommandExpectOk(cmd);
  DBGLN(String("[ARM] ARM AWAY ") + (ok ? "OK" : "FAIL") + " cmd=" + cmd);
}
static inline void sendPanelArmStay(int partition, int staySet) {
  String cmd = String("STAY") + staySet + ":" + partition;
  bool ok = sendCommandExpectOk(cmd);
  DBGLN(String("[ARM] ARM STAY") + staySet + " " + (ok ? "OK" : "FAIL") + " cmd=" + cmd);
}
static inline void sendPanelDisarm(int partition) {
  String cmd = String("DISARM:") + partition;
  bool ok = sendCommandExpectOk(cmd);
  DBGLN(String("[ARM] DISARM ") + (ok ? "OK" : "FAIL") + " cmd=" + cmd);
}
void sendPanelReset(int partition) {
  String cmd = String("RESET:") + partition;
  bool ok = sendCommandExpectOk(cmd);
  DBGLN(String("[ARM] RESET ") + (ok ? "OK" : "FAIL") + " cmd=" + cmd);
}

void sendRemoteOutputSet(int o, bool on) {
  if (o < 1 || o > MAX_OUTPUTS_HARD) return;
  String cmd = on ? (String("ROPON:") + o) : (String("ROPOFF:") + o);
  bool ok = sendCommandExpectOk(cmd);
  DBGLN(String("[OUT] ") + (on ? "ON " : "OFF ") + String(o) + " " + (ok ? "OK" : "FAIL") + " cmd=" + cmd);

  forceRopsSoon = true;
  forceRopsAtMs = millis() + 250;
}

void handleHaAlarmCommand(int p, const String& msg) {
  if (msg.startsWith("ARM_")) {
    if (!settings.enableRemoteArming) { DBGLN("[ARM] Remote arming disabled - ignoring."); return; }
    if (msg == "ARM_AWAY")          { sendPanelArmAway(p); return; }
    if (msg == "ARM_HOME")          { sendPanelArmStay(p, 1); return; }
    if (msg == "ARM_NIGHT")         { sendPanelArmStay(p, 2); return; }
    if (msg == "ARM_CUSTOM_BYPASS") { sendPanelArmStay(p, 3); return; }
  }
  if (msg == "DISARM") {
    if (!settings.enableRemoteDisarming) { DBGLN("[ARM] Remote disarming disabled - ignoring."); return; }
    sendPanelDisarm(p);
    delay(200);
    sendPanelReset(p);
    return;
  }
  if (msg == "TRIGGER") { DBGLN("[ARM] TRIGGER requested (no UART mapping yet)"); return; }
  DBGLN(String("Unknown alarm command (ignored): ") + msg);
}

void initialDiscoveryAndPoll() {
  probePanelVersionAndSerial();
  publishCoreDiscovery();

  discoverPartitions();
  discoverZones();
  discoverOutputs();

  pollGETSYS();
  pollGETP();
  pollROPS();
  pollGETZ();
}

void periodicPollLoop() {
  uint32_t now=millis();

  if (forceRopsSoon && (int32_t)(now - forceRopsAtMs) >= 0) {
    forceRopsSoon = false;
    pollROPS();
    nextROPS = now + ROPS_POLL_MS;
  }

  if ((int32_t)(now - nextGETZ) >= 0)   { nextGETZ   = now + GETZ_POLL_MS;   pollGETZ(); }
  if ((int32_t)(now - nextGETP) >= 0)   { nextGETP   = now + GETP_POLL_MS;   pollGETP(); }
  if ((int32_t)(now - nextROPS) >= 0)   { nextROPS   = now + ROPS_POLL_MS;   pollROPS(); }
  if ((int32_t)(now - nextGETSYS) >= 0) { nextGETSYS = now + GETSYS_POLL_MS; pollGETSYS(); }
}
