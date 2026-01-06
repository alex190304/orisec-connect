#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <WebServer.h>

// ===================== SETTINGS (PERSISTENT) =====================
struct Settings {
  String wifiSsid;
  String wifiPass;

  String mqttHost;
  uint16_t mqttPort = 1883;
  String mqttUser;
  String mqttPass;

  String haAlarmCode;     // sensitive (not viewable)
  String deviceName = "orisec-bridge";
  String deviceId   = "orisec_bridge_1";

  String panelUserCode;   // sensitive (not viewable)

  bool requireHaCodeForArmDisarm = true;
  bool enableRemoteArming = false;
  bool enableRemoteDisarming = false;

  bool enableIdeOta = false;
  String otaPass;

  bool useEthernet = true;
};

extern Settings settings;
extern const char* CFG_FILE;

// ===================== DEBUG =====================
#define DEBUG_SERIAL 1

// ===================== BUTTONS =====================
extern const int CONFIG_BTN_PIN;
extern const int FACTORY_BTN_PIN;
extern const uint32_t BTN_DEBOUNCE_MS;

extern const uint32_t FACTORY_HOLD_MS;

extern const int CONFIG_MODE_PIN;
extern const int POWER_RUN_PIN;
extern const int TX_LED_PIN;
extern const int RX_LED_PIN;
extern const int ETH_SCK_PIN;
extern const int ETH_MISO_PIN;
extern const int ETH_MOSI_PIN;
extern const int ETH_CS_PIN;
extern const int ETH_RST_PIN;
extern const int ETH_INT_PIN;

// ===================== PANEL / LIMITS / POLL =====================
extern const uint32_t PANEL_BAUD;
extern const bool PREFIX_USER_CODE;

extern const int MAX_ZONES_HARD;
extern const int MAX_PARTITIONS_HARD;
extern const int MAX_OUTPUTS_HARD;

extern const uint32_t GETZ_POLL_MS;
extern const uint32_t GETP_POLL_MS;
extern const uint32_t ROPS_POLL_MS;
extern const uint32_t GETSYS_POLL_MS;
extern const uint32_t HEARTBEAT_MS;

extern const int PANEL_TX_PIN;
extern const int PANEL_RX_PIN;
extern HardwareSerial panelSerial;

// UI
extern const char* PARTITION_ICON;
extern const char* STAY3_NAME;

// ===================== CAPTIVE PORTAL =====================
extern bool configModeActive;
extern DNSServer dnsServer;
extern WebServer web;
extern IPAddress apIP;
extern uint32_t portalStartMs;
extern const uint32_t PORTAL_TIMEOUT_MS;
extern String apPassword;

// ===================== TX LOCK =====================
extern volatile bool txLocked;

// ===================== MQTT =====================
extern WiFiClient wifiClient;
extern EthernetClient ethClient;
extern Client* netClient;
extern PubSubClient mqtt;
extern const uint16_t MQTT_BUF_SZ;

extern const char* DISCOVERY_PREFIX;

extern String BASE_TOPIC;
extern String AVAIL_TOPIC;
extern String CMD_RELEARN_TOPIC;
extern String CMD_RESTART_TOPIC;

// ===================== STATUS LEDS =====================
extern volatile bool factoryResetActive;

// ===================== ORISEC PROTOCOL =====================
extern const uint8_t ORI_CR;
extern const uint8_t ORI_LF;

// ===================== RX FRAMING =====================
extern uint8_t frameBuf[900];
extern size_t frameLen;
extern uint32_t lastByteMs;
extern const uint32_t IDLE_FLUSH_MS;

extern volatile bool haveFrame;
extern uint8_t lastFrame[900];
extern size_t lastFrameLen;

// ===================== PANEL INFO =====================
extern String panelModel;
extern String panelVersion;
extern String panelSerialStr;
extern int zoneLimit;

// ===================== TABLES =====================
struct ZoneInfo { bool present=false; String label; };
struct PartInfo { bool present=false; String label; };
struct OutInfo  { bool present=false; String label; };

extern ZoneInfo zones[];
extern String lastZoneState[];
extern uint8_t bootZoneStatus[];

extern PartInfo parts[];
extern String lastPartAlarmState[];
extern int partitionCount;

extern OutInfo outs[];
extern int outputCount;
extern String lastOutState[];

extern String lastVolt0, lastVolt1, lastVolt2;

extern volatile bool doRelearn;

extern volatile bool forceRopsSoon;
extern uint32_t forceRopsAtMs;

extern uint32_t nextGETZ, nextGETP, nextROPS, nextGETSYS;

void initTopics();
