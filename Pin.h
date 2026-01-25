#pragma once
#include "StateType.h"

class Pin {
    bool inverted = false;
public:
    int pinNumber;
    const char* pinName;
    Pin();
    Pin(int pinNumber, const char* pinName, int pinType, bool inverted = false);

    int read();
    int set(StateType newState);
    int set(int newValue);
};
