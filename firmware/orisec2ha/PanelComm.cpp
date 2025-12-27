#include "PanelComm.h"
#include "Log.h"
#include "OrisecUtil.h"
#include <ArduinoOTA.h>

static void lockTxDueToInvalidCode() {
  if (!txLocked) {
    txLocked = true;
    DBGLN("!!! TX LOCKED: Panel reported ERROR:User Code Invalid. Will not transmit until reboot.");
  }
}
static inline bool canTransmitToPanel() { return !txLocked && !configModeActive; }

static void serial1WriteBytes(const uint8_t* data, size_t len) {
  if (!canTransmitToPanel()) {
#if DEBUG_SERIAL
    DBGLN("TX blocked (txLocked or configModeActive).");
#endif
    return;
  }
#if DEBUG_SERIAL
  Serial.print("TX BYTES ["); Serial.print(len); Serial.print("]: ");
  for (size_t i=0;i<len;i++) { Serial.print("0x"); DBGHEX(data[i]); Serial.print(" "); }
  Serial.println();
#endif
  Serial1.write(data, len);
}

void panelSendChecked(const String& body, bool allowNoUserCode) {
  if (!canTransmitToPanel()) return;

  String full;
  bool useCode = PREFIX_USER_CODE && !allowNoUserCode;
  if (useCode) full += settings.panelUserCode;
  full += body;

#if DEBUG_SERIAL
  Serial.print("TX ASCII: ");
  Serial.println(full);
#endif

  uint8_t buf[600];
  size_t n=0;
  for (size_t i=0;i<full.length() && n<sizeof(buf);i++) buf[n++] = (uint8_t)full[i];

  uint8_t cs = oriChecksum(buf, n);
#if DEBUG_SERIAL
  Serial.print("TX CS=0x"); DBGHEX(cs); Serial.println();
#endif

  if (n < sizeof(buf)) buf[n++] = cs;
  if (n + 2 <= sizeof(buf)) { buf[n++] = ORI_CR; buf[n++] = ORI_LF; }

  serial1WriteBytes(buf, n);
}

static bool isQueryBody(const String& body) {
  return (body == "GETZ" || body == "GETP" || body == "GETSYS" || body == "GETVER" || body == "GETSERIAL" || body == "ROPS"
          || body.startsWith("ZINFO:") || body.startsWith("PINFO:") || body.startsWith("OINFO:"));
}

void panelSendOKAck() { panelSendChecked("OK", true); }

static String frameToAsciiNoCsum(const uint8_t* d, size_t n) {
  while (n>0 && isTerm(d[n-1])) n--;
  if (n == 0) return "";
  size_t usable = (n >= 1) ? (n - 1) : 0;
  String out;
  out.reserve(usable);
  for (size_t i=0;i<usable;i++) {
    uint8_t b = d[i];
    if (isPrintable(b)) out += (char)b;
  }
  out.trim();
  return out;
}

static void handleIncomingFrame(const uint8_t* d, size_t n) {
  while (n>0 && isTerm(d[n-1])) n--;
  if (!n) return;

#if DEBUG_SERIAL
  Serial.print("RX ["); Serial.print(n); Serial.print("]: ");
  for (size_t i=0;i<n;i++) {
    uint8_t b=d[i];
    if (isPrintable(b)) Serial.write((char)b);
    else { Serial.print("\\x"); DBGHEX(b); }
  }
  Serial.println();
  Serial.print("RX checksum "); Serial.println(oriValidateFrame(d,n) ? "OK" : "BAD");
#endif

  String ascii = frameToAsciiNoCsum(d, n);
  if (ascii.indexOf("ERROR:User Code Invalid") >= 0) lockTxDueToInvalidCode();

  memcpy(lastFrame, d, n);
  lastFrameLen = n;
  haveFrame = true;

  if (startsWith(lastFrame, lastFrameLen, "AL00")) {
    DBGLN("CID seen -> ACK OK");
    panelSendOKAck();
  }
}

void pollPanelRx() {
  while (panelRx.available()) {
    uint8_t b = (uint8_t)panelRx.read();
    lastByteMs = millis();
    if (frameLen < sizeof(frameBuf)) frameBuf[frameLen++] = b;
    if (isTerm(b)) {
      handleIncomingFrame(frameBuf, frameLen);
      frameLen = 0;
    }
  }
  if (frameLen > 0 && (millis() - lastByteMs) > IDLE_FLUSH_MS) {
    handleIncomingFrame(frameBuf, frameLen);
    frameLen = 0;
  }
}

void drainPanel(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) { pollPanelRx(); yield(); }
}

bool waitForFramePrefix(const char* prefix, uint32_t timeoutMs) {
  uint32_t start = millis();
  haveFrame = false;
  while ((millis()-start) < timeoutMs) {
    pollPanelRx();
    mqtt.loop();
    if (settings.enableIdeOta) ArduinoOTA.handle();
    yield();
    if (haveFrame) {
      haveFrame=false;
      if (startsWith(lastFrame, lastFrameLen, prefix)) return true;
    }
    if (txLocked) return false;
  }
  return false;
}

bool requestWithRetry(const String& body, const char* expectPrefix, uint32_t timeoutMs, bool allowNoUserCode) {
  if (!canTransmitToPanel()) return false;

  drainPanel(10);
  panelSendChecked(body, allowNoUserCode);
  if (waitForFramePrefix(expectPrefix, timeoutMs)) return true;

  drainPanel(10);
  panelSendChecked(body, allowNoUserCode);
  return waitForFramePrefix(expectPrefix, timeoutMs);
}

bool sendCommandExpectOk(const String& body, bool allowNoUserCode) {
  if (!canTransmitToPanel()) return false;
  if (isQueryBody(body)) return false;

  for (int attempt=1; attempt<=3; attempt++) {
    drainPanel(10);
    panelSendChecked(body, allowNoUserCode);
    if (waitForFramePrefix("OK", 800)) return true;
    DBGLN(String("No OK for command '") + body + "' attempt " + attempt + "/3");
    if (!canTransmitToPanel()) return false;
  }
  return false;
}
