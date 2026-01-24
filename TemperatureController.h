#pragma once
#include <OneWire.h>
#include "Thermometer.h"
#include "logging.h"

class TemperatureController {
    int dataPin;
    OneWire oneWire;
    DallasTemperature sensors;
    
public:
    TemperatureController(){};
    
    template<size_t N>
    TemperatureController(int dataPin, Thermometer (&thermometers)[N]) 
        : dataPin(dataPin), oneWire(dataPin), sensors(&oneWire) {
        log_info("Initializing Temperature Controller on pin %d with %d thermometers", dataPin, N);
        log_debug("Setting Sensors on Onewire");
        this->discoverThermometers();

        for (size_t i = 0; i < N; i++) {
            thermometers[i].setSensor(sensors);
            this->printAddress(thermometers[i]);
        }

        this->sensors.begin();
        this->sensors.requestTemperatures();
    }
    
    void requestTemperatures();
    void discoverThermometers(); 
    void printAddress(Thermometer t);
};