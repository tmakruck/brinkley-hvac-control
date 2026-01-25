#include <Arduino.h>
#include "StateType.h"

// RELAY HIGH trigger Names - relays in NC position
const StateType Passthrough = {"Passthrough", LOW};
const StateType NoSupply12V = {"NoSupply12V", LOW};
const StateType Relax_Relay = { "Relax the Relay", LOW };

// RELAY LOW trigger names - turns relays to NO position
const StateType BlockSignal = {"BlockSignal", HIGH};
const StateType Supply12V = {"Supply12V", HIGH};

// SSR is not inverted logic so they get their own set
const StateType SpaceHeaterOn = {"SpaceHeaterOn", HIGH};
const StateType SSROff = {"SSROff", LOW};