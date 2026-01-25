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
    SignalState readSignalState();
    OutputState readOutputState();
    void updateOutputStates(OutputState newState);
    OutputState validateNewState(OutputState newCalculatedState);
    char* printPinStates(SignalState currentInputState, OutputState calculatedState, OutputState actualState);
    OutputState CalculateNewOutputState(SignalState currentSignalState);
public:
    HVACZone(){};
    HVACZone(HVACZoneConfig config);
    char* DoLoop();
    void fallbackToDefaultBehavior();
    int lcdOffset() { return zoneConfig.lcdOffset; }
};

 
