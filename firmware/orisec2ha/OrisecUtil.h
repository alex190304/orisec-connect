#pragma once
#include <Arduino.h>
#include <string.h>
#include "Globals.h"

static inline bool isTerm(uint8_t b) { return (b == ORI_LF || b == ORI_CR); }
static inline bool isPrintable(uint8_t b) { return (b >= 0x20 && b <= 0x7E); }

uint8_t oriChecksum(const uint8_t* data, size_t len);
bool oriValidateFrame(const uint8_t* data, size_t len);

bool startsWith(const uint8_t* d, size_t n, const char* s);

String jsonEscape(const String& in);
String sanitizeForObjectId(String s);
String topic(const String& suffix);
