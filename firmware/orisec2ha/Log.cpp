#include "Log.h"
#include <string.h>

const size_t LOG_BUF_SZ = 8192;
char logBuf[LOG_BUF_SZ];
size_t logLen = 0;

void logAppend(const String& s) {
  String line = s;
  if (!line.endsWith("\n")) line += "\n";
  size_t need = line.length();
  if (need >= LOG_BUF_SZ) {
    memcpy(logBuf, line.c_str() + (need - (LOG_BUF_SZ-1)), LOG_BUF_SZ-1);
    logBuf[LOG_BUF_SZ-1] = 0;
    logLen = LOG_BUF_SZ-1;
    return;
  }
  if (logLen + need >= LOG_BUF_SZ) {
    size_t drop = (logLen + need) - (LOG_BUF_SZ-1);
    if (drop > logLen) drop = logLen;
    memmove(logBuf, logBuf + drop, logLen - drop);
    logLen -= drop;
  }
  memcpy(logBuf + logLen, line.c_str(), need);
  logLen += need;
  logBuf[logLen] = 0;
}
