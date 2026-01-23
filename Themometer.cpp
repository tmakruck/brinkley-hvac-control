#include "thermometer.h"
#include "logging.h"
#include <DallasTemperature.h>

Thermometer::Thermometer(char* name, DeviceAddress address) : name(name)
{
    memcpy(this->address, address, sizeof(DeviceAddress));
    this->lastTemperature = 150;
}

bool Thermometer::requiresHeatingMode(int setPointF, int temperatureSwing = 1) {
    int outdoorTemp = this->retrieveTemperature();
    if (this->isHysteresisLowMode) {
        // Currently in spaceHeater/Furnace mode - need temp to rise above UPPER threshold to switch
        if (outdoorTemp >= setPointF+temperatureSwing) {
            log_debug("%s Temperature rising above %dF", this->name, setPointF+temperatureSwing);
            this->isHysteresisLowMode = false;
        }
        } else {
        // Currently in heat pump mode - need temp to drop below LOWER threshold to switch
        if (outdoorTemp < setPointF-temperatureSwing) {
            log_debug("%s Temperature dropped below %dF", this->name, setPointF-temperatureSwing);
            this->isHysteresisLowMode = true;
        }
    }    
}

int Thermometer::retrieveTemperature() {
    // call sensors.requestTemperatures() to issue a global temperature
    // request to all devices on the bus
    // After we got the temperatures, we can print them here.
    // We use the function ByIndex, and as an example get the temperature from the first sensor only.

    float temperature = sensors->getTempF(this->address);
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
        log_debug("Error: Could not read temperature data");
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