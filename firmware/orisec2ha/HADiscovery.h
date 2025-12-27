#pragma once
#include <Arduino.h>
#include "Globals.h"

String haDeviceBlock();

void publishDiscoveryButton(const String& objectId, const String& name, const String& commandTopic, const String& icon);
void publishDiscoverySensor(const String& objectId, const String& name, const String& stateTopicStr,
                            const String& icon, const String& unit="", const String& deviceClass="");
void publishDiscoverySwitch(const String& objectId,
                           const String& name,
                           const String& stateTopicStr,
                           const String& commandTopicStr,
                           const String& icon);

void publishDiscoveryAlarmPanel(int p, const String& name);
void publishCoreDiscovery();
void publishZoneDiscovery(int z);
void publishOutputDiscovery(int o);
