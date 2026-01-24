#include "thermometer.h"
#include "logging.h"
#include <DallasTemperature.h>

Thermometer::Thermometer(const char* name, DeviceAddress address) : name(name)
{
    log_info("Initializing Thermometer %s", name);
    memcpy(this->address, address, sizeof(DeviceAddress));
    this->lastTemperature = 150;
}

bool Thermometer::getHysteresisMode(int setPointF, int temperatureSwing) {
    int outdoorTemp = this->retrieveTemperature();
    if (this->hysteresisMode == LOW) {
        // Currently in spaceHeater/Furnace mode - need temp to rise above UPPER threshold to switch
        if (outdoorTemp >= setPointF+temperatureSwing) {
            log_debug("%s Temperature rising above %dF", this->name, setPointF+temperatureSwing);
            this->hysteresisMode = HIGH;
        }
        } else {
        // Currently in heat pump mode - need temp to drop below LOWER threshold to switch
        if (outdoorTemp < setPointF-temperatureSwing) {
            log_debug("%s Temperature dropped below %dF", this->name, setPointF-temperatureSwing);
            this->hysteresisMode = LOW;
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

bool Thermometer::IsFrigid(){
    int outdoorTemp = this->retrieveTemperature();
    return outdoorTemp < 10;
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