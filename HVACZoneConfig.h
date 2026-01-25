#pragma once

#include "Pin.h"
#include "logging.h"

class HVACZoneConfig
{
    int startPin;
public:
    const char *roomName;
    bool hasFurnace = false;
    int hysteresisSetPoint_F;
    int underbellyThreshold;
    int lcdOffset = 0;
    // Input Signals
    Pin pinFanLoSense;
    Pin pinFanHiSense;
    Pin pinACSense;
    Pin pinHPSense;
    Pin pinFurnaceSense = Pin();

    // Relay output pins (control coils)
    Pin pinFanLoOut;
    Pin pinFanHiOut;
    Pin pinACOut;
    Pin pinHPOut;
    Pin pinSSR;

    Pin pinFurnaceOut = Pin();
    Pin pinFurnacePower = Pin();
    Pin pinHPPower = Pin();

    HVACZoneConfig(){};
    HVACZoneConfig(const char *roomName, int lcdOffset, int startPin, int pinSSR, int hysteresisSetPoint_F, int furnaceSensePin = -1, int underbellyThreshold = 100)
        : roomName(roomName), lcdOffset(lcdOffset), startPin(startPin), hysteresisSetPoint_F(hysteresisSetPoint_F), hasFurnace(furnaceSensePin != -1), underbellyThreshold(underbellyThreshold)
    {
        log_info("Creating %s HVACZoneConfig", roomName);
        this->pinFanLoSense = Pin(startPin, "Fan Lo Sense", INPUT_PULLUP);
        this->pinFanHiSense = Pin(startPin + 2, "Fan Hi Sense", INPUT_PULLUP);
        this->pinACSense = Pin(startPin + 4, "AC Sense", INPUT_PULLUP);
        this->pinHPSense = Pin(startPin + 6, "HP Sense", INPUT_PULLUP);

        // Outputs (relay coil pins)
        this->pinFanLoOut = Pin(startPin + 8, "Fan Lo Out", OUTPUT);  // +8
        this->pinFanHiOut = Pin(startPin + 10, "Fan Hi Out", OUTPUT); // +10
        this->pinACOut = Pin(startPin + 12, "AC Out", OUTPUT);        // +12
        this->pinHPOut = Pin(startPin + 14, "HP Out", OUTPUT);        // +14

        this->pinSSR = Pin(pinSSR, "SSR Out", OUTPUT); // SSR pin

        if (hasFurnace)
        {
            this->pinFurnaceSense = Pin(furnaceSensePin, "Furnace Sense", INPUT_PULLUP);
            this->pinFurnaceOut = Pin(furnaceSensePin + 8, "Furnace Out", OUTPUT);
            this->pinFurnacePower = Pin(furnaceSensePin + 10, "Furnace Power", OUTPUT);
            this->pinHPPower = Pin(furnaceSensePin + 12, "Heat Pump Power", OUTPUT);
        }
    }
};
