#include "Globals.h"

Settings settings;
const char* CFG_FILE = "/config.txt";

// Buttons
const int CONFIG_BTN_PIN = D5;
const uint32_t BTN_DEBOUNCE_MS = 50;

const int FACTORY_BTN_PIN = D6;
const uint32_t FACTORY_HOLD_MS = 10000;

// Panel / limits / poll
const uint32_t PANEL_BAUD = 9600;
const bool PREFIX_USER_CODE = true;

const int MAX_ZONES_HARD      = 200;
const int MAX_PARTITIONS_HARD = 8;
const int MAX_OUTPUTS_HARD    = 64;

const uint32_t GETZ_POLL_MS   = 300;
const uint32_t GETP_POLL_MS   = 1000;
const uint32_t ROPS_POLL_MS   = 5000;
const uint32_t GETSYS_POLL_MS = 30000;
const uint32_t HEARTBEAT_MS   = 30000;

// UART pins
const int PANEL_RX_PIN = D7;
SoftwareSerial panelRx(PANEL_RX_PIN, -1);

// Captive portal
bool configModeActive = false;
DNSServer dnsServer;
ESP8266WebServer web(80);
IPAddress apIP(192, 168, 4, 1);
uint32_t portalStartMs = 0;
const uint32_t PORTAL_TIMEOUT_MS = 5UL * 60UL * 1000UL;
String apPassword;

// TX lock
volatile bool txLocked = false;

// MQTT
WiFiClient espClient;
PubSubClient mqtt(espClient);
const uint16_t MQTT_BUF_SZ = 2200;

const char* DISCOVERY_PREFIX = "homeassistant";
const char* BASE_TOPIC       = "orisec";
const char* AVAIL_TOPIC      = "orisec/status";

const char* CMD_RELEARN_TOPIC = "orisec/cmd/relearn";
const char* CMD_RESTART_TOPIC = "orisec/cmd/restart";

// Orisec protocol
const uint8_t ORI_CR = 0x13;
const uint8_t ORI_LF = 0x0A;

// RX framing
uint8_t frameBuf[900];
size_t frameLen = 0;
uint32_t lastByteMs = 0;
const uint32_t IDLE_FLUSH_MS = 30;

volatile bool haveFrame = false;
uint8_t lastFrame[900];
size_t lastFrameLen = 0;

// Panel info
String panelModel   = "Unknown";
String panelVersion = "";
String panelSerial  = "";
int zoneLimit = MAX_ZONES_HARD;

// Tables
ZoneInfo zones[MAX_ZONES_HARD + 1];
String lastZoneState[MAX_ZONES_HARD + 1];
uint8_t bootZoneStatus[MAX_ZONES_HARD + 1];

PartInfo parts[MAX_PARTITIONS_HARD + 1];
String lastPartAlarmState[MAX_PARTITIONS_HARD + 1];
int partitionCount = 0;

OutInfo outs[MAX_OUTPUTS_HARD + 1];
int outputCount = 0;
String lastOutState[MAX_OUTPUTS_HARD + 1];

String lastVolt0="", lastVolt1="", lastVolt2="";

volatile bool doRelearn = false;
volatile bool forceRopsSoon = false;
uint32_t forceRopsAtMs = 0;

uint32_t nextGETZ=0, nextGETP=0, nextROPS=0, nextGETSYS=0;
