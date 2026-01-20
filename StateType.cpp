#include <Arduino.h>
#include "StateType.h"

const StateType Passthrough = {"Passthrough", HIGH};
const StateType NoPassthrough = {"NoPassthrough", LOW};
const StateType Supply12V = {"Supply12V", LOW}; 
const StateType NoSupply12V = {"NoSupply12V", HIGH};
const StateType SSROn = {"SSROn", HIGH};
const StateType SSROff = {"SSROff", LOW};