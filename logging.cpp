#include <Arduino.h>
#include "logging.h"

extern bool debugState;

void log_info(const char* fmt, ...){
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Serial.println(buffer);
}

void log_debug(const const char* fmt, ...){
  if (!debugState) return;

  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Serial.println(buffer);
}

const char* statusString(bool isHigh) {
  return isHigh ? "High" : "Low";
}
