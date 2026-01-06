#include "Portal.h"
#include "Log.h"
#include "WifiMqtt.h"
#include "ConfigStore.h"
#include "OrisecUtil.h"

#include <LittleFS.h>
#include <ArduinoOTA.h>

// ----------------------------- helpers -----------------------------

static String htmlEscape(const String& s) {
  String o; o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;";
    else o += c;
  }
  return o;
}

static String pageHeader(const String& title) {
  String h;
  h.reserve(1400);
  h += "<!doctype html><html><head><meta charset='utf-8'>";
  h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h += "<title>" + title + "</title>";
  h += "<style>"
       "body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;margin:0;background:#0b1220;color:#e6e6e6}"
       "a{color:#9ad0ff;text-decoration:none}"
       ".wrap{max-width:920px;margin:0 auto;padding:18px}"
       ".card{background:#121a2b;border:1px solid #1f2a44;border-radius:14px;padding:16px;margin:14px 0}"
       ".muted{color:#b0b7c5;font-size:13px}"
       ".nav{margin-top:8px}"
       ".nav a{display:inline-block;margin-right:10px}"
       ".row{display:grid;grid-template-columns:1fr 1fr;gap:12px}"
       "@media(max-width:720px){.row{grid-template-columns:1fr}}"
       ".field{margin-top:12px}"
       ".field label{display:block;font-size:13px;color:#b0b7c5;margin:0 0 6px 2px}"
       "input,select,textarea{width:100%;box-sizing:border-box;padding:10px 12px;border-radius:10px;border:1px solid #2b3b60;background:#0b1220;color:#e6e6e6;outline:none}"
       "input:focus,textarea:focus,select:focus{border-color:#3f8ee6}"
       ".buttons{display:flex;flex-wrap:wrap;gap:10px;margin-top:14px}"
       ".btn{padding:10px 14px;border-radius:10px;border:0;background:#2f7dd1;color:white;font-weight:650;cursor:pointer}"
       ".btn.secondary{background:#23324f;color:#e6e6e6;border:1px solid #2b3b60}"
       ".btn:disabled{opacity:.65;cursor:not-allowed}"
       ".check{display:flex;align-items:flex-start;gap:10px;flex-wrap:wrap;margin-top:10px}"
       ".check input{width:auto;margin-top:2px}"
       ".check span{white-space:normal;word-break:break-word;flex:1;min-width:0}"
       ".pre{max-height:420px;overflow:auto;background:#0b1220;border:1px solid #1f2a44;border-radius:10px;padding:12px;font-size:12px;line-height:1.35}"
       "</style></head><body><div class='wrap'>";
  h += "<h2 style='margin:6px 0 8px 0'>OrisecConnect / Orisec2HA Setup</h2>";
  h += "<div class='muted'>AP expires after 5 minutes.</div>";
  h += "<div class='muted nav'>"
       "Pages: "
       "<a href='/'>Home</a> · "
       "<a href='/wifi'>WiFi</a> · "
       "<a href='/mqtt'>MQTT</a> · "
       "<a href='/sec'>Security</a> · "
       "<a href='/ota'>OTA</a> · "
       "<a href='/logs'>Logs</a>"
       "</div>";
  return h;
}

static String pageFooter() {
  return "</div></body></html>";
}

static void sendHtmlPage(const String& title, const String& bodyHtml) {
  String page;
  page.reserve(pageHeader(title).length() + bodyHtml.length() + 64);
  page += pageHeader(title);
  page += bodyHtml;
  page += pageFooter();

  web.sendHeader("Cache-Control", "no-store");
  web.sendHeader("Pragma", "no-cache");
  web.sendHeader("Connection", "close");
  web.send(200, "text/html", page);
}

// ----------------------------- pages -----------------------------

static void handleHome() {
  String b;
  b.reserve(800);
  b += "<div class='card'>"
       "<h3 style='margin:0 0 6px 0'>Homepage</h3>"
       "<div class='muted'>Choose a page to configure the bridge.</div>"
       "<div class='buttons' style='margin-top:12px'>"
       "<a href='/wifi'><button class='btn'>WiFi</button></a>"
       "<a href='/mqtt'><button class='btn'>MQTT</button></a>"
       "<a href='/sec'><button class='btn'>Security</button></a>"
       "<a href='/ota'><button class='btn'>OTA</button></a>"
       "<a href='/logs'><button class='btn secondary'>Logs</button></a>"
       "</div>"
       "</div>";
  sendHtmlPage("Home", b);
}

static void handleWiFiPage() {
  String b;
  b.reserve(3200);

  b += "<div class='card'><h3 style='margin:0 0 6px 0'>WiFi</h3>"
       "<div class='muted'>Set WiFi SSID and password (used when Ethernet is disabled). Config portal always uses WiFi AP.</div>"
       "<div class='field' style='margin-top:10px'>"
       "<label><input type='checkbox' id='useEth' ";
  b += (settings.useEthernet ? "checked" : "");
  b += "> Use Ethernet (W5500) for MQTT</label></div>";

  b += "<div class='row'>"
       "<div class='field'><label for='ssid'>SSID</label>"
       "<input id='ssid' value='";
  b += htmlEscape(settings.wifiSsid);
  b += "'></div>"
       "<div class='field'><label for='pass'>Password (leave blank to keep)</label>"
       "<input type='password' id='pass' placeholder='Leave blank to keep existing'></div>"
       "</div>";

  b += "<div class='buttons'>"
       "<button id='saveBtn' class='btn' onclick='save(false)'>Save</button>"
       "<button id='applyBtn' class='btn secondary' onclick='save(true)'>Apply &amp; Reboot</button>"
       "</div>"
       "<div class='muted' id='msg' style='margin-top:10px'></div>";

  b += "<script>"
       "async function save(reboot){"
       "const saveBtn=document.getElementById('saveBtn');"
       "const applyBtn=document.getElementById('applyBtn');"
       "saveBtn.disabled=true;applyBtn.disabled=true;"
       "document.getElementById('msg').textContent='Saving...';"
       "try{"
       "let fd=new FormData();"
       "fd.append('useEthernet',document.getElementById('useEth').checked?'1':'0');"
       "fd.append('wifiSsid',document.getElementById('ssid').value);"
       "fd.append('wifiPass',document.getElementById('pass').value);"
       "if(reboot)fd.append('reboot','1');"
       "let r=await fetch('/save_wifi',{method:'POST',body:fd});"
       "let t=await r.text();"
       "alert(t);"
       "document.getElementById('msg').textContent=t;"
       "if(reboot)setTimeout(()=>location.href='/',1500);"
       "}catch(e){alert('Save failed');}"
       "saveBtn.disabled=false;applyBtn.disabled=false;"
       "}"
       "</script>";

  b += "</div>";
  sendHtmlPage("WiFi", b);
}

static void handleMqttPage() {
  String b;
  b.reserve(3400);

  b += "<div class='card'><h3 style='margin:0 0 6px 0'>MQTT</h3>"
       "<div class='muted'>Broker settings.</div>";

  b += "<div class='row'>"
       "<div class='field'><label for='host'>Host / IP</label>"
       "<input id='host' value='";
  b += htmlEscape(settings.mqttHost);
  b += "'></div>"
       "<div class='field'><label for='port'>Port</label>"
       "<input id='port' value='";
  b += String(settings.mqttPort);
  b += "'></div>"
       "</div>";

  b += "<div class='row'>"
       "<div class='field'><label for='user'>Username</label>"
       "<input id='user' value='";
  b += htmlEscape(settings.mqttUser);
  b += "'></div>"
       "<div class='field'><label for='pass'>Password (leave blank to keep)</label>"
       "<input type='password' id='pass' placeholder='Leave blank to keep existing'></div>"
       "</div>";

  b += "<div class='buttons'>"
       "<button id='saveBtn' class='btn' onclick='save(false)'>Save</button>"
       "<button id='applyBtn' class='btn secondary' onclick='save(true)'>Apply &amp; Reboot</button>"
       "</div>"
       "<div class='muted' id='msg' style='margin-top:10px'></div>";

  b += "<script>"
       "async function save(reboot){"
       "const saveBtn=document.getElementById('saveBtn');"
       "const applyBtn=document.getElementById('applyBtn');"
       "saveBtn.disabled=true;applyBtn.disabled=true;"
       "document.getElementById('msg').textContent='Saving...';"
       "try{"
       "let fd=new FormData();"
       "fd.append('mqttHost',document.getElementById('host').value);"
       "fd.append('mqttPort',document.getElementById('port').value);"
       "fd.append('mqttUser',document.getElementById('user').value);"
       "fd.append('mqttPass',document.getElementById('pass').value);"
       "if(reboot)fd.append('reboot','1');"
       "let r=await fetch('/save_mqtt',{method:'POST',body:fd});"
       "let t=await r.text();"
       "alert(t);"
       "document.getElementById('msg').textContent=t;"
       "if(reboot)setTimeout(()=>location.href='/',1500);"
       "}catch(e){alert('Save failed');}"
       "saveBtn.disabled=false;applyBtn.disabled=false;"
       "}"
       "</script>";

  b += "</div>";
  sendHtmlPage("MQTT", b);
}

static void handleSecPage() {
  String b;
  b.reserve(4200);

  b += "<div class='card'><h3 style='margin:0 0 6px 0'>Security</h3>"
       "<div class='muted'>Panel/HA codes and remote control toggles.</div>";

  b += "<div class='row'>"
       "<div class='field'><label for='dn'>Device Name</label>"
       "<input id='dn' value='";
  b += htmlEscape(settings.deviceName);
  b += "'></div>"
       "<div class='field'><label for='did'>Device ID</label>"
       "<input id='did' value='";
  b += htmlEscape(settings.deviceId);
  b += "'></div>"
       "</div>";

  b += "<div class='row'>"
       "<div class='field'><label for='puc'>Panel User Code (leave blank to keep)</label>"
       "<input type='password' id='puc' placeholder='Leave blank to keep existing'></div>"
       "<div class='field'><label for='hac'>HA Alarm Code (leave blank to keep)</label>"
       "<input type='password' id='hac' placeholder='Leave blank to keep existing'></div>"
       "</div>";

  b += "<label class='check'><input type='checkbox' id='ea' ";
  b += (settings.enableRemoteArming ? "checked" : "");
  b += "><span>Enable remote arming</span></label>";

  b += "<label class='check'><input type='checkbox' id='ed' ";
  b += (settings.enableRemoteDisarming ? "checked" : "");
  b += "><span>Enable remote disarming</span></label>";

  b += "<label class='check'><input type='checkbox' id='hr' ";
  b += (settings.requireHaCodeForArmDisarm ? "checked" : "");
  b += "><span>Require HA code for arm/disarm actions</span></label>";

  b += "<div class='buttons'>"
       "<button id='saveBtn' class='btn' onclick='save(false)'>Save</button>"
       "<button id='applyBtn' class='btn secondary' onclick='save(true)'>Apply &amp; Reboot</button>"
       "</div>"
       "<div class='muted' id='msg' style='margin-top:10px'></div>";

  b += "<script>"
       "async function save(reboot){"
       "const saveBtn=document.getElementById('saveBtn');"
       "const applyBtn=document.getElementById('applyBtn');"
       "saveBtn.disabled=true;applyBtn.disabled=true;"
       "document.getElementById('msg').textContent='Saving...';"
       "try{"
       "let fd=new FormData();"
       "fd.append('deviceName',document.getElementById('dn').value);"
       "fd.append('deviceId',document.getElementById('did').value);"
       "fd.append('panelUserCode',document.getElementById('puc').value);"
       "fd.append('haAlarmCode',document.getElementById('hac').value);"
       "fd.append('enableRemoteArming',document.getElementById('ea').checked?'1':'0');"
       "fd.append('enableRemoteDisarming',document.getElementById('ed').checked?'1':'0');"
       "fd.append('requireHaCodeForArmDisarm',document.getElementById('hr').checked?'1':'0');"
       "if(reboot)fd.append('reboot','1');"
       "let r=await fetch('/save_sec',{method:'POST',body:fd});"
       "let t=await r.text();"
       "alert(t);"
       "document.getElementById('msg').textContent=t;"
       "if(reboot)setTimeout(()=>location.href='/',1500);"
       "}catch(e){alert('Save failed');}"
       "saveBtn.disabled=false;applyBtn.disabled=false;"
       "}"
       "</script>";

  b += "</div>";
  sendHtmlPage("Security", b);
}

static void handleOtaPage() {
  String b;
  b.reserve(2600);

  b += "<div class='card'><h3 style='margin:0 0 6px 0'>OTA</h3>"
       "<div class='muted'>Arduino IDE OTA toggle and password.</div>";

  b += "<label class='check'><input type='checkbox' id='ide' ";
  b += (settings.enableIdeOta ? "checked" : "");
  b += "><span>Enable Arduino IDE OTA</span></label>";

  b += "<div class='field'><label for='op'>OTA Password (leave blank to keep)</label>"
       "<input type='password' id='op' placeholder='Leave blank to keep existing'></div>";

  b += "<div class='buttons'>"
       "<button id='saveBtn' class='btn' onclick='save(false)'>Save</button>"
       "<button id='applyBtn' class='btn secondary' onclick='save(true)'>Apply &amp; Reboot</button>"
       "</div>"
       "<div class='muted' id='msg' style='margin-top:10px'></div>";

  b += "<script>"
       "async function save(reboot){"
       "const saveBtn=document.getElementById('saveBtn');"
       "const applyBtn=document.getElementById('applyBtn');"
       "saveBtn.disabled=true;applyBtn.disabled=true;"
       "document.getElementById('msg').textContent='Saving...';"
       "try{"
       "let fd=new FormData();"
       "fd.append('enableIdeOta',document.getElementById('ide').checked?'1':'0');"
       "fd.append('otaPass',document.getElementById('op').value);"
       "if(reboot)fd.append('reboot','1');"
       "let r=await fetch('/save_ota',{method:'POST',body:fd});"
       "let t=await r.text();"
       "alert(t);"
       "document.getElementById('msg').textContent=t;"
       "if(reboot)setTimeout(()=>location.href='/',1500);"
       "}catch(e){alert('Save failed');}"
       "saveBtn.disabled=false;applyBtn.disabled=false;"
       "}"
       "</script>";

  b += "</div>";
  sendHtmlPage("OTA", b);
}

static void handleLogsPage() {
  String b;
  b.reserve(1400 + LOG_BUF_SZ);

  b += "<div class='card'><h3 style='margin:0 0 6px 0'>Device Log</h3>"
       "<div class='muted'>Log buffer size: ";
  b += String(LOG_BUF_SZ - 1);
  b += " bytes (RAM).</div>"
       "<div class='buttons' style='margin-top:12px'>"
       "<a href='/logs.txt'><button class='btn'>Download .txt</button></a>"
       "</div>";

  if (logLen == 0 || logBuf[0] == 0) {
    b += "<div class='muted' style='margin-top:12px'>No log entries yet.</div>";
  } else {
    b += "<pre class='pre'>";
    b += htmlEscape(String(logBuf));
    b += "</pre>";
  }

  b += "</div>";
  sendHtmlPage("Logs", b);
}

static void handleLogsDownload() {
  web.sendHeader("Content-Disposition", "attachment; filename=\"device-log.txt\"");
  web.sendHeader("Cache-Control", "no-store");
  web.send(200, "text/plain", String(logBuf));
}

// ----------------------------- save handlers -----------------------------

static void maybeRebootAfterSave() {
  if (web.hasArg("reboot") && web.arg("reboot") == "1") {
    delay(250); // allow response to flush
    ESP.restart();
  }
}

static void handleSaveWifi() {
  Settings s = settings;
  if (web.hasArg("useEthernet")) s.useEthernet = (web.arg("useEthernet") == "1");
  if (web.hasArg("wifiSsid")) s.wifiSsid = web.arg("wifiSsid");
  String wp = web.arg("wifiPass");
  if (wp.length()) s.wifiPass = wp;

  bool ok = saveSettings(s);
  if (ok) settings = s;
  web.send(ok ? 200 : 500, "text/plain", ok ? "Saved" : "Save failed");
  if (ok) DBGLN("Portal: Saved WiFi settings");
  maybeRebootAfterSave();
}

static void handleSaveMqtt() {
  Settings s = settings;
  if (web.hasArg("mqttHost")) s.mqttHost = web.arg("mqttHost");
  if (web.hasArg("mqttPort")) s.mqttPort = (uint16_t) web.arg("mqttPort").toInt();
  if (web.hasArg("mqttUser")) s.mqttUser = web.arg("mqttUser");
  String mp = web.arg("mqttPass");
  if (mp.length()) s.mqttPass = mp;
  if (s.mqttPort == 0) s.mqttPort = 1883;

  bool ok = saveSettings(s);
  if (ok) settings = s;
  web.send(ok ? 200 : 500, "text/plain", ok ? "Saved" : "Save failed");
  if (ok) DBGLN("Portal: Saved MQTT settings");
  maybeRebootAfterSave();
}

static void handleSaveSec() {
  Settings s = settings;
  if (web.hasArg("deviceName")) s.deviceName = web.arg("deviceName");
  if (web.hasArg("deviceId")) s.deviceId = web.arg("deviceId");

  String pc = web.arg("panelUserCode");
  if (pc.length()) s.panelUserCode = pc;
  String ha = web.arg("haAlarmCode");
  if (ha.length()) s.haAlarmCode = ha;

  if (web.hasArg("enableRemoteArming")) s.enableRemoteArming = (web.arg("enableRemoteArming") == "1");
  if (web.hasArg("enableRemoteDisarming")) s.enableRemoteDisarming = (web.arg("enableRemoteDisarming") == "1");
  if (web.hasArg("requireHaCodeForArmDisarm")) s.requireHaCodeForArmDisarm = (web.arg("requireHaCodeForArmDisarm") == "1");

  if (s.deviceName.length() == 0) s.deviceName = "orisec-bridge";
  if (s.deviceId.length() == 0) s.deviceId = "orisec_bridge_1";

  bool ok = saveSettings(s);
  if (ok) settings = s;
  web.send(ok ? 200 : 500, "text/plain", ok ? "Saved" : "Save failed");
  if (ok) DBGLN("Portal: Saved security settings");
  maybeRebootAfterSave();
}

static void handleSaveOta() {
  Settings s = settings;
  if (web.hasArg("enableIdeOta")) s.enableIdeOta = (web.arg("enableIdeOta") == "1");
  String op = web.arg("otaPass");
  if (op.length()) s.otaPass = op;

  bool ok = saveSettings(s);
  if (ok) settings = s;
  web.send(ok ? 200 : 500, "text/plain", ok ? "Saved (reboot recommended)" : "Save failed");
  if (ok) DBGLN("Portal: Saved OTA settings");
  maybeRebootAfterSave();
}

// ----------------------------- lifecycle -----------------------------

void startConfigPortal() {
  if (configModeActive) return;

  configModeActive = true;
  portalStartMs = millis();
  DBGLN("=== CONFIG PORTAL START ===");
  delay(50);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  WiFi.softAP("Orisec Setup", apPassword.c_str());
  delay(100);

  dnsServer.start(53, "*", apIP);

  // Some cores support this; harmless if it doesn't exist (guarded by ifdef below).
  #if defined(ESP32)
    // If enableDelay exists, it improves reliability with larger pages.
    // Not all WebServer builds expose it.
    // We'll call it via a preprocessor guard that won't break compilation if absent:
    // (If you still get a compile error here, delete these 2 lines.)
    web.enableDelay(true);
  #endif

  // Pages
  web.on("/", HTTP_GET, handleHome);
  web.on("/wifi", HTTP_GET, handleWiFiPage);
  web.on("/mqtt", HTTP_GET, handleMqttPage);
  web.on("/sec", HTTP_GET, handleSecPage);
  web.on("/ota", HTTP_GET, handleOtaPage);
  web.on("/logs", HTTP_GET, handleLogsPage);
  web.on("/logs.txt", HTTP_GET, handleLogsDownload);

  // Saves
  web.on("/save_wifi", HTTP_POST, handleSaveWifi);
  web.on("/save_mqtt", HTTP_POST, handleSaveMqtt);
  web.on("/save_sec", HTTP_POST, handleSaveSec);
  web.on("/save_ota", HTTP_POST, handleSaveOta);

  web.onNotFound([]() {
    web.sendHeader("Location", "/", true);
    web.send(302, "text/plain", "");
  });

  web.begin();

  DBGLN(String("AP SSID: Orisec Setup"));
  DBGLN(String("AP PASS: ") + apPassword);
  DBGLN(String("Portal IP: ") + WiFi.softAPIP().toString());
}

void stopConfigPortal() {
  if (!configModeActive) return;
  DBGLN("=== CONFIG PORTAL STOP (timeout) ===");

  configModeActive = false;
  dnsServer.stop();
  web.stop();

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
}

void configPortalLoop() {
  if (portalStartMs && (millis() - portalStartMs) > PORTAL_TIMEOUT_MS) {
    stopConfigPortal();
    return;
  }
  dnsServer.processNextRequest();
  web.handleClient();
  yield();
}

String makeApPassword() {
  uint32_t id = static_cast<uint32_t>(ESP.getEfuseMac());
  uint32_t mix = id ^ 0xA5C3F19D;
  char buf[13];
  snprintf(buf, sizeof(buf), "%08lX%04X",
           (unsigned long)mix,
           (unsigned int)((mix >> 8) & 0xFFFF));
  return String(buf);
}
