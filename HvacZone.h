#pragma once

#include <Arduino.h>
#include "StateType.h"
#include "HVACZoneConfig.h"
#include "SignalState.h"
#include "OutputState.h"
struct ZoneOutputs {
    StateType fanLo = Passthrough;
    StateType fanHi = Passthrough;
    StateType ac = Passthrough;
    StateType heatPump = Passthrough;

    StateType spaceHeater = SpaceHeaterOff;

    StateType furnace = Passthrough;
    StateType furnacePower = NoSupply12V;
    StateType heatPumpPower = NoSupply12V;
};
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
    void applyHysteresisLowRules(const SignalState& s, ZoneOutputs& o, bool hasFurnace);
    void applyNormalModeRules(const SignalState& s, ZoneOutputs& o, bool hasFurnace);
public:
    HVACZone(){};
    HVACZone(HVACZoneConfig config);
    String DoLoop();
    void fallbackToDefaultBehavior();
    int lcdOffset() { return zoneConfig.lcdOffset; }
};

 
