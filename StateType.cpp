#include <Arduino.h>
#include "StateType.h"

// RELAY HIGH trigger Names - relays in NC position
const StateType Passthrough = {"Passthrough", LOW};
const StateType NoSupply12V = {"No Supply 12V", LOW};
const StateType Relax_Relay = { "Relax the Relay", LOW };

// RELAY LOW trigger names - turns relays to NO position
const StateType BlockSignal = {"Block Signal", HIGH};
const StateType Supply12V = {"Supply 12V", HIGH};

// SSR is not inverted logic so they get their own set
const StateType SpaceHeaterOn = {"Space Heater On", HIGH};
const StateType SpaceHeaterOff = {"Space Heater Off", LOW};