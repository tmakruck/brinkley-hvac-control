#pragma once

#include <Arduino.h>
#include "StateType.h"
#include "HVACZoneConfig.h"
#include "SignalState.h"
#include "OutputState.h"
#include "Thermometer.h"

extern Thermometer OutsideThermometer;
extern Thermometer UnderbellyThermometer;

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

struct LastTemperature {
    Thermometer* whichThermometer; 
    int lastTemp; 
    int hysteresisMode;
};

class HVACZone {
    HVACZoneConfig zoneConfig;
    SignalState previousSignalState;
    OutputState lastOutputState;
    LastTemperature lastTemperatures[2] = {
        { &OutsideThermometer, Thermometer::INITIAL_TEMPERATURE, HIGH },
        { &UnderbellyThermometer, Thermometer::INITIAL_TEMPERATURE, HIGH }
    };

    SignalState readSignalState();
    OutputState readOutputState();
    void updateOutputStates(OutputState newState);
    OutputState validateNewState(OutputState newCalculatedState);
    String printPinStates(SignalState currentSignalState, OutputState calculatedState, OutputState actualState);
    OutputState CalculateNewOutputState(SignalState currentSignalState);
    void applyHysteresisLowRules(const SignalState& s, ZoneOutputs& o, bool hasFurnace);
    void applyNormalModeRules(const SignalState& s, ZoneOutputs& o, bool hasFurnace);
    bool getHysteresisMode(Thermometer& whichThermometer, int setPointF, int temperatureSwing = 1);
    bool getCallState(Thermometer& whichThermometer, int setPointF, int temperatureSwing = 1);
    
public:
    HVACZone(){};
    HVACZone(HVACZoneConfig config);
    String DoLoop();
    void fallbackToDefaultBehavior();
    int lcdOffset() { return zoneConfig.lcdOffset; }
};

 
