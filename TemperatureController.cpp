#include "TemperatureController.h"
#include "logging.h"

TemperatureController::TemperatureController(int dataPin, Thermometer thermometers[]) : dataPin(dataPin) {
    log_info("Initializing Temperature Controller on pin %d", dataPin);
    OneWire oneWire=OneWire(dataPin);
    this->oneWire = oneWire;
    log_debug("Setting Sensors on Onewire");
    this->sensors = DallasTemperature(&oneWire);
    log_debug("moving on");
    this->discoverThermometers();

    for (int i = 0; i < sizeof(thermometers) / sizeof(thermometers[0]); i++) {
        thermometers[i].setSensor(sensors);
        this->printAddress(thermometers[i]);
    }

    this->sensors.begin();
    this->sensors.requestTemperatures();
}

void TemperatureController::requestTemperatures() {
    this->sensors.requestTemperatures();
}

void TemperatureController::discoverThermometers() {
    log_debug("Discovering thermometers on the bus...");
    int deviceCount = this->sensors.getDeviceCount();
    log_info("Found %d devices on the bus", deviceCount);
    for (int i = 0; i < deviceCount; i++) {
        DeviceAddress address;
        if (this->sensors.getAddress(address, i)) {
            log_debug("Device %d address: ", i);
            for (uint8_t j = 0; j < 8; j++) {
                if (address[j] < 16) Serial.print("0");
                Serial.print(address[j], HEX);
            }
            Serial.println();
        } else {
            log_debug("Could not find address for device %d", i);
        }
    }
}

void TemperatureController::printAddress(Thermometer t) {
    DeviceAddress* deviceAddress;
    memcpy(deviceAddress, t.address, sizeof(DeviceAddress));
    for (uint8_t i = 0; i < 8; i++)
    {
        if ((*deviceAddress)[i] < 16) Serial.print("0");
        Serial.print((*deviceAddress)[i], HEX);
    }
}