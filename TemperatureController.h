#pragma once
#include <OneWire.h>
#include "Thermometer.h"

class TemperatureController {
    int dataPin;
    OneWire oneWire;
    DallasTemperature sensors;
    
public:
    TemperatureController(){};
    TemperatureController(int dataPin, Thermometer thermometers[]);
    void requestTemperatures();
    void discoverThermometers(); 
    void printAddress(Thermometer t);
};