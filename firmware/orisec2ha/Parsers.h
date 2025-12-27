#pragma once
#include <Arduino.h>
#include "Globals.h"

int deriveZoneLimitFromModel(const String& model);

bool parseGETVER();
bool parseGETSERIAL();
bool parseGETSYS(String& v0, String& v1, String& v2);

bool parseGETP(String& statusChars);
bool parsePINFO(String& label);
bool parseZINFO(String& label);

bool parseROPS(String& bits);
bool parseOINFO(String& label);

bool zoneProgrammed(uint8_t b);
String zoneStateFromByte(uint8_t b);
String partCharToHAAlarmState(char c);
