#include <Arduino.h>
#include "StateType.h"
#include "Pin.h"
#include "logging.h"

Pin::Pin() {
    this->pinNumber = -1;
    this->pinName = "Uninitialized Pin";
    this->inverted = false;
}

Pin::Pin(int pinNumber, const char* pinName, int pinType, bool inverted) 
    : pinNumber(pinNumber), pinName(pinName), inverted(inverted) {
    pinMode(pinNumber, pinType);
}
int Pin::read() {
    int pinValue = digitalRead(this->pinNumber);
    log_debug("%s reads %s", this->pinName, statusString(pinValue));
    return pinValue;
}

int Pin::set(StateType newState) {
    log_debug("Setting %s (%d) to %s (%d)", this->pinName, this->pinNumber, newState.Name, newState.Value);
    digitalWrite(this->pinNumber, newState.Value);
    return this->read();
}

int Pin::set(int newValue) {
    log_debug("Setting %s (%d) to %s", this->pinName, this->pinNumber, statusString(newValue));
    digitalWrite(this->pinNumber, newValue);
    return this->read();
}
