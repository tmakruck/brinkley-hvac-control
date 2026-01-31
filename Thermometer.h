#pragma once
#include <DallasTemperature.h>

class Thermometer
{

    DallasTemperature *sensors = nullptr; // DallasTemperature instance
    unsigned long lastReadTime = 0;
    int hysteresisMode = HIGH;
public:
    static const int INITIAL_TEMPERATURE = 999; // An out-of-range initial temperature
    const char* name;
    DeviceAddress address;
    int lastTemperature = INITIAL_TEMPERATURE;
    Thermometer(){};
    Thermometer(const char* name, DeviceAddress address);

    int retrieveTemperature();
    void setSensor(DallasTemperature &sensors);
    void printAddress();
};
