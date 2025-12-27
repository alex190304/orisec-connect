#include "Ota.h"

void setupArduinoIdeOta() {
  ArduinoOTA.setHostname(settings.deviceId.c_str());
  if (settings.otaPass.length()) ArduinoOTA.setPassword(settings.otaPass.c_str());

  ArduinoOTA.onStart([](){ DBGLN("[OTA] Start"); });
  ArduinoOTA.onEnd([](){ DBGLN("[OTA] End"); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t){
    if (t) {
      static uint32_t last=0;
      uint32_t now=millis();
      if (now-last > 1000) { last=now; DBGLN(String("[OTA] Progress ") + (p*100)/t + "%"); }
    }
  });
  ArduinoOTA.onError([](ota_error_t e){
    DBGLN(String("[OTA] Error ") + (int)e);
  });
  ArduinoOTA.begin();
  DBGLN("[OTA] Arduino IDE OTA enabled");
}
