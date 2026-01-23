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
    OutputState validateNewState(OutputState newCalculatedState);
    void printPinStates(SignalState currentInputState, OutputState calculatedState, OutputState actualState);
    OutputState CalculateNewOutputState(SignalState currentSignalState);
public:
    HVACZone(HVACZoneConfig config);
    void DoLoop();
    void fallbackToDefaultBehavior();
};

 
