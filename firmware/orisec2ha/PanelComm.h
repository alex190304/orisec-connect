#pragma once
#include <Arduino.h>
#include "Globals.h"

void pollPanelRx();
void drainPanel(uint32_t ms);

bool waitForFramePrefix(const char* prefix, uint32_t timeoutMs);
bool requestWithRetry(const String& body, const char* expectPrefix, uint32_t timeoutMs, bool allowNoUserCode=false);
bool sendCommandExpectOk(const String& body, bool allowNoUserCode=false);

void panelSendChecked(const String& body, bool allowNoUserCode=false);
void panelSendOKAck();
