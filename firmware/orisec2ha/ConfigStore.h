#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "Globals.h"

bool loadSettings();
bool saveSettings(const Settings& s);
void factoryReset();
