#include <Arduino.h>
#include "StateType.h"
#include "Pin.h"
#include "logging.h"

Pin::Pin() {
    this->pinNumber = -1;
    this->pinName = "Uninitialized Pin";
    this->inverted = false;
}

Pin::Pin(int pinNumber, char* pinName, int pinType, bool inverted = false) : pinNumber(pinNumber), pinName(pinName), inverted(inverted) {
    pinMode(pinNumber, pinType);
}
bool Pin::read() {
    bool isHigh = digitalRead(this->pinNumber) == HIGH;
    log_debug("%s reads %s", this->pinName, statusString(isHigh));
    return isHigh;
}

bool Pin::set(StateType state) {
    log_debug("Setting %s (%d) to %s (%d)", this->pinName, this->pinNumber, state.Name, state.Value);
    digitalWrite(this->pinNumber, state.Value);
    return this->read();
}

bool Pin::set(bool isHigh) {
    log_debug("Setting %s (%d) to %s", this->pinName, this->pinNumber, statusString(isHigh));
    digitalWrite(this->pinNumber, isHigh ? HIGH : LOW);
    return this->read();
}
