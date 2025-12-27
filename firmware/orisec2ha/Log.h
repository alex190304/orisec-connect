#pragma once
#include <Arduino.h>
#include "Globals.h"

extern const size_t LOG_BUF_SZ;
extern char logBuf[];
extern size_t logLen;

void logAppend(const String& s);

#if DEBUG_SERIAL
  #define DBG(x)     do { Serial.print(x); } while(0)
  #define DBGLN(x)   do { Serial.println(x); logAppend(String(x)); } while(0)
  #define DBGHEX(b)  do { if ((b) < 16) Serial.print('0'); Serial.print((b), HEX); } while (0)
#else
  #define DBG(x)
  #define DBGLN(x)   do { logAppend(String(x)); } while(0)
  #define DBGHEX(b)
#endif
