#pragma once
#include <DallasTemperature.h>

class Thermometer
{

    DallasTemperature *sensors; // DallasTemperature instance
    unsigned long lastReadTime = 0;
    int lastTemperature;
    bool isHysteresisLowMode = false;
public:
    char* name;
    DeviceAddress address;
    Thermometer(char* name, DeviceAddress address);

    bool requiresHeatingMode(int thresholdF, int temperatureSwing = 1);
    bool IsFrigid();
    int retrieveTemperature();
    void setSensor(DallasTemperature &sensors);
};
