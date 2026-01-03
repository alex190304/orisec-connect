#include "Parsers.h"
#include "OrisecUtil.h"

int deriveZoneLimitFromModel(const String& model) {
  String digits;
  for (size_t i=0;i<model.length();i++) if (isDigit(model[i])) digits += model[i];
  int n = digits.toInt();
  if (n <= 0) return MAX_ZONES_HARD;
  if (n > MAX_ZONES_HARD) n = MAX_ZONES_HARD;
  return n;
}

bool parseGETVER() {
  if (!startsWith(lastFrame, lastFrameLen, "GETVER:")) return false;
  if (lastFrameLen < 8 + 1) return false;
  size_t usable = lastFrameLen - 1;

  String body;
  for (size_t i=7;i<usable;i++) body += (char)lastFrame[i];
  body.trim();

  int sp = body.indexOf(' ');
  if (sp < 0) { panelModel = body; panelVersion=""; return true; }

  panelModel = body.substring(0, sp); panelModel.trim();
  int vpos = body.indexOf("Version");
  if (vpos >= 0) { panelVersion = body.substring(vpos + 7); panelVersion.trim(); }
  else panelVersion = "";
  return true;
}

bool parseGETSERIAL() {
  if (!startsWith(lastFrame, lastFrameLen, "GETSERIAL:")) return false;
  if (lastFrameLen < 10 + 1) return false;
  size_t usable = lastFrameLen - 1;

  String body;
  for (size_t i=10;i<usable;i++) body += (char)lastFrame[i];
  body.trim();
  panelSerialStr = body;
  return panelSerialStr.length() > 0;
}

bool parseGETSYS(String& v0, String& v1, String& v2) {
  v0=v1=v2="";
  if (!startsWith(lastFrame, lastFrameLen, "GETSYS:")) return false;
  if (lastFrameLen < 7 + 2) return false;
  size_t usable = lastFrameLen - 1;

  String body;
  for (size_t i=7;i<usable;i++) body += (char)lastFrame[i];
  body.trim();

  int c1 = body.indexOf(',');
  int c2 = body.indexOf(',', c1+1);
  if (c1<0 || c2<0) return false;

  v0 = body.substring(0,c1); v0.trim();
  v1 = body.substring(c1+1,c2); v1.trim();
  v2 = body.substring(c2+1); v2.trim();
  return true;
}

bool parseGETP(String& statusChars) {
  statusChars = "";
  if (!startsWith(lastFrame, lastFrameLen, "GETP:")) return false;
  if (lastFrameLen < 5 + 1 + 1) return false;
  size_t usable = lastFrameLen - 1;
  for (size_t i=5;i<usable;i++) statusChars += (char)lastFrame[i];
  statusChars.trim();
  return statusChars.length() > 0;
}

bool parsePINFO(String& label) {
  label = "";
  if (!startsWith(lastFrame, lastFrameLen, "PINFO:")) return false;
  if (lastFrameLen < 6 + 1) return false;
  size_t usable = lastFrameLen - 1;
  for (size_t i=6;i<usable;i++) label += (char)lastFrame[i];
  label.trim();
  return true;
}

bool parseZINFO(String& label) {
  label = "";
  if (!startsWith(lastFrame, lastFrameLen, "ZINFO:")) return false;
  if (lastFrameLen < 6 + 2) return false;
  size_t usable = lastFrameLen - 1;

  String body;
  for (size_t i=6;i<usable;i++) body += (char)lastFrame[i];
  body.trim();

  int c1 = body.indexOf(',');
  if (c1 < 0) return false;

  String first = body.substring(0,c1); first.trim();
  bool firstDigits = first.length() > 0;
  for (size_t i=0;i<first.length() && firstDigits;i++) if (!isDigit(first[i])) firstDigits = false;

  if (firstDigits) {
    int c2 = body.indexOf(',', c1+1);
    if (c2 < 0) return false;
    label = body.substring(c1+1,c2);
  } else {
    label = first;
  }
  label.trim();
  return true;
}

bool parseROPS(String& bits) {
  bits = "";
  if (!startsWith(lastFrame, lastFrameLen, "ROPS:")) return false;
  if (lastFrameLen < 5 + 1 + 1) return false;
  size_t usable = lastFrameLen - 1;
  for (size_t i=5;i<usable;i++) bits += (char)lastFrame[i];
  bits.trim();
  return bits.length() > 0;
}

bool parseOINFO(String& label) {
  label = "";
  if (!startsWith(lastFrame, lastFrameLen, "OINFO:")) return false;
  if (lastFrameLen < 6 + 1) return false;
  size_t usable = lastFrameLen - 1;

  String body;
  for (size_t i=6; i<usable; i++) body += (char)lastFrame[i];
  body.trim();
  if (!body.length()) return false;

  int c1 = body.indexOf(',');
  if (c1 < 0) { label = body; label.trim(); return label.length() > 0; }

  String first = body.substring(0, c1); first.trim();
  bool firstDigits = first.length() > 0;
  for (size_t i=0; i<first.length() && firstDigits; i++) if (!isDigit(first[i])) firstDigits = false;

  if (firstDigits) {
    int c2 = body.indexOf(',', c1+1);
    if (c2 < 0) label = body.substring(c1+1);
    else label = body.substring(c1+1, c2);
  } else {
    label = first;
  }
  label.trim();
  return label.length() > 0;
}

bool zoneProgrammed(uint8_t b) { return (b & 0x80) == 0; }

String zoneStateFromByte(uint8_t b) {
  if (b & 0x80) return "unused";
  if (b & 0x20) return "alarm";
  if (b & 0x02) return "tamper";
  if (b & 0x04) return "masked";
  if (b & 0x08) return "fault";
  if (b & 0x10) return "bypassed";
  if (b & 0x01) return "active";
  return "normal";
}

String partCharToHAAlarmState(char c) {
  switch (c) {
    case 'D': return "disarmed";
    case 'F': return "armed_away";
    case '1': return "armed_home";
    case '2': return "armed_night";
    case '3': return "armed_custom_bypass";
    case 'A': return "triggered";
    case 'C': return "arming";
    case 'X': return "unknown";
    default:  return "unknown";
  }
}
