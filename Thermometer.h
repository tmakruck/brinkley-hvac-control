#pragma once
#include <DallasTemperature.h>

class Thermometer
{

    DallasTemperature *sensors = nullptr; // DallasTemperature instance
    unsigned long lastReadTime = 0;
    int lastTemperature;
    int hysteresisMode = HIGH;
public:
    const char* name;
    DeviceAddress address;
    Thermometer(){};
    Thermometer(const char* name, DeviceAddress address);

    bool getHysteresisMode(int setPointF, int temperatureSwing = 1);
    bool IsFrigid();
    int retrieveTemperature();
    void setSensor(DallasTemperature &sensors);
    void printAddress();
};
