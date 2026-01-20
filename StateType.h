#pragma once

#include <Arduino.h>

struct StateType{
  char* Name;
  int Value;
};

extern const StateType Passthrough;
extern const StateType NoPassthrough;
extern const StateType Supply12V;
extern const StateType NoSupply12V;
extern const StateType SSROn;
extern const StateType SSROff;

