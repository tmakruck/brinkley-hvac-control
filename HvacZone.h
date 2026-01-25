#pragma once

#include <Arduino.h>
#include "StateType.h"
#include "HVACZoneConfig.h"
#include "SignalState.h"
#include "OutputState.h"

class HVACZone {
    HVACZoneConfig zoneConfig;
    SignalState previousSignalState;
    OutputState lastOutputState;
    SignalState readSignalState();
    OutputState readOutputState();
    void updateOutputStates(OutputState newState);
    OutputState validateNewState(OutputState newCalculatedState);
    String printPinStates(SignalState currentSignalState, OutputState calculatedState, OutputState actualState);
    OutputState CalculateNewOutputState(SignalState currentSignalState);
public:
    HVACZone(){};
    HVACZone(HVACZoneConfig config);
    String DoLoop();
    void fallbackToDefaultBehavior();
    int lcdOffset() { return zoneConfig.lcdOffset; }
};

 
