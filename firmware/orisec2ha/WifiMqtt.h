#pragma once
#include <Arduino.h>
#include "Globals.h"

bool mqttPublishRetained(const String& t, const String& payload);
bool mqttPublishRetainedIfChanged(String& cache, const String& t, const String& payload);

void wifiEnsure();
void mqttEnsure();
void heartbeatLoop();

void mqttCallback(char* tpc, byte* payload, unsigned int length);
