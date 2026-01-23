#include <Arduino.h>
#include "StateType.h"

// RELAY HIGH trigger Names - relays in NC position
const StateType Passthrough = {"Passthrough", HIGH};
const StateType NoSupply12V = {"NoSupply12V", HIGH};
const StateType Relax_Relay = { "Relax the Relay", HIGH };

// RELAY LOW trigger names - turns relays to NO position
const StateType BlockSignal = {"BlockSignal", LOW};
const StateType Supply12V = {"Supply12V", LOW};

// SSR is not inverted logic so they get their own set
const StateType SSROn = {"SSROn", HIGH};
const StateType SSROff = {"SSROff", LOW};