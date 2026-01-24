#include <Arduino.h>
#include "logging.h"

extern bool debugState;

void log_info(const char* fmt, ...){
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Serial.println(buffer);
}

void log_debug(const char* fmt, ...){
  if (!debugState) return;

  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Serial.println(buffer);
}

const char* statusString(int value) {
  return value==HIGH ? "High" : "Low";
}
