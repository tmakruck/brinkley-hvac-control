#pragma once
#include "StateType.h"

class Pin {
    bool inverted = false;
public:
    int pinNumber;
    const char* pinName;
    Pin();
    Pin(int pinNumber, const char* pinName, int pinType, bool inverted = false);

    bool read();
    bool set(StateType state);
    bool set(bool isHigh);
};
