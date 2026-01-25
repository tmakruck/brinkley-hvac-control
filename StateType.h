#pragma once
#include <Arduino.h>

struct StateType{
  const char* Name;
  int Value;
};

// RELAY HIGH trigger Names - relays in NC position
extern const StateType Passthrough;
extern const StateType NoSupply12V;
extern const StateType Relax_Relay;

// RELAY LOW trigger names - turns relays to NO position
extern const StateType BlockSignal;
extern const StateType Supply12V;

// SSR is not inverted logic so they get their own set
extern const StateType SpaceHeaterOn;
extern const StateType SSROff;

