#pragma once

#include <Arduino.h>
#include "StateType.h"
#include "HVACZoneConfig.h"
#include "SignalState.h"
#include "OutputState.h"

class HVACZone {
    HVACZoneConfig zoneConfig;
    SignalState previousInputState;
    OutputState lastOutputState;
    bool isHysteresisLowMode = false;
    SignalState readSignalState();
    OutputState readOutputState();
    void updateOutputStates(OutputState newState);
    void validateNewState(newCalculatedState);
    void printPinStates();
    void CalculateNewOutputState(SignalState currentSignalState);
public:
    HVACZone(HVACZoneConfig config);
    void DoLoop();
    void fallbackToDefaultBehavior();
};

 
