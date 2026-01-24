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

int Pin::set(StateType state) {
    log_debug("Setting %s (%d) to %s (%d)", this->pinName, this->pinNumber, state.Name, state.Value);
    digitalWrite(this->pinNumber, state.Value);
    return this->read();
}

int Pin::set(int stateValue) {
    log_debug("Setting %s (%d) to %s", this->pinName, this->pinNumber, statusString(stateValue));
    digitalWrite(this->pinNumber, stateValue ? HIGH : LOW);
    return this->read();
}
