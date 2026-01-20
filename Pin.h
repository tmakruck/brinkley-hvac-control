#pragma once
#include "StateType.h"

class Pin {
    bool inverted = false;
public:
    int pinNumber;
    char* pinName;
    Pin();
    Pin(int pinNumber, char* pinName, int pinType, bool inverted = false);

    bool read();
    bool set(StateType state);
    bool set(bool isHigh);
};
