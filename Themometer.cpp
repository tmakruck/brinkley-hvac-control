#include "thermometer.h"
#include "logging.h"
#include <DallasTemperature.h>

Thermometer::Thermometer(const char* name, DeviceAddress address) : name(name)
{
    log_info("Initializing Thermometer %s", name);
    memcpy(this->address, address, sizeof(DeviceAddress));
    this->lastTemperature = INITIAL_TEMPERATURE;
}

bool Thermometer::getCallState(int setPointF, int temperatureSwing) {
    bool hysteresisMode = this->getHysteresisMode(setPointF, temperatureSwing); // Update hysteresis mode first
    // Call is active when in LOW hysteresis mode
    return hysteresisMode == LOW;
}

bool Thermometer::getHysteresisMode(int setPointF, int temperatureSwing) {
    int lastTemp = this->lastTemperature;
    int currentTemp = this->retrieveTemperature();
    if (this->hysteresisMode == LOW) {
        // Currently in It's Too Cold mode - need temp to rise above UPPER threshold to switch
        if (currentTemp >= setPointF || lastTemp == INITIAL_TEMPERATURE) {
            log_info("%s temperature is now above %dF", this->name, setPointF);
            this->hysteresisMode = HIGH;
        }
        else {
            log_debug("%s temperature remains below %dF", this->name, setPointF);
        }
    } else {
        // Currently in It's Warm  Enough mode - need temp to drop below LOWER threshold to switch
        if (currentTemp < setPointF-temperatureSwing || lastTemp == INITIAL_TEMPERATURE) {
            log_info("%s temperature is now below %dF", this->name, setPointF-temperatureSwing);
            this->hysteresisMode = LOW;
        }
        else {
            log_debug("%s temperature remains above %dF", this->name, setPointF-temperatureSwing);
        }
    }

    return this->hysteresisMode;  
}

int Thermometer::retrieveTemperature() {
    // call sensors.requestTemperatures() to issue a global temperature
    // request to all devices on the bus
    // After we got the temperatures, we can print them here.
    // We use the function ByIndex, and as an example get the temperature from the first sensor only.
    if (this->sensors == nullptr) {
        log_info("Error: Sensor not initialized for %s", this->name);
        this->printAddress();
        return this->lastTemperature;  // Return cached value
    }
    
    float temperature = this->sensors->getTempF(this->address);
    int tempInt = static_cast<int>(round(temperature));
    if (tempInt != this->lastTemperature) {
        this->lastTemperature = tempInt;

        // Check if reading was successful
        if (temperature != DEVICE_DISCONNECTED_F)
        {
            log_debug("Temperature for %s is %d", this->name, tempInt);
        }
        else
        {
            log_info("Error: Could not read temperature data for %s", this->name);
            this->printAddress();
        }
    }

    return this->lastTemperature;
}

void Thermometer::setSensor(DallasTemperature& sensors) {
    this->sensors = &sensors;
}

void Thermometer::printAddress() {
    Serial.print("Thermometer Address for ");
    Serial.print(this->name);
    Serial.print(": ");
    for (uint8_t i = 0; i < 8; i++) {
        if (this->address[i] < 16) Serial.print("0");
        Serial.print(this->address[i], HEX);
    }
    Serial.println();
}