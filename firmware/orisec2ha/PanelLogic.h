#pragma once
#include <Arduino.h>
#include "Globals.h"

void probePanelVersionAndSerial();
void discoverPartitions();
void discoverZones();
void discoverOutputs();

void pollGETZ();
void pollGETP();
void pollROPS();
void pollGETSYS();

void relearnAll();
void initialDiscoveryAndPoll();
void periodicPollLoop();

void handleHaAlarmCommand(int p, const String& msg);
void sendPanelReset(int partition);
void sendRemoteOutputSet(int o, bool on);
