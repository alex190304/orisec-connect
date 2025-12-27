#include "OrisecUtil.h"

uint8_t oriChecksum(const uint8_t* data, size_t len) {
  uint16_t s = 0;
  for (size_t i = 0; i < len; i++) s += data[i];
  return (uint8_t)(s & 0xFF) ^ 0xFF;
}

bool oriValidateFrame(const uint8_t* data, size_t len) {
  if (len < 2) return false;
  uint8_t got = data[len - 1];
  uint8_t calc = oriChecksum(data, len - 1);
  return got == calc;
}

bool startsWith(const uint8_t* d, size_t n, const char* s) {
  size_t m = strlen(s);
  if (n < m) return false;
  for (size_t i=0;i<m;i++) if (d[i] != (uint8_t)s[i]) return false;
  return true;
}

String jsonEscape(const String& in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i=0;i<in.length();i++) {
    char c = in[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c >= 0x20) out += c;
        break;
    }
  }
  return out;
}

String sanitizeForObjectId(String s) {
  s.toLowerCase();
  String out;
  out.reserve(s.length() + 8);
  for (size_t i=0;i<s.length();i++) {
    char c = s[i];
    if ((c>='a' && c<='z') || (c>='0' && c<='9')) out += c;
    else if (c==' ' || c=='_' || c=='-') out += '_';
  }
  while (out.indexOf("__") >= 0) out.replace("__","_");
  out.trim();
  if (!out.length()) out = "entity";
  return out;
}

String topic(const String& suffix) { return String(BASE_TOPIC) + "/" + suffix; }
