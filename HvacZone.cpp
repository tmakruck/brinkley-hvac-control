#include <Arduino.h>
#include "HvacZone.h"
#include "Thermometer.h"
#include "HVACZoneConfig.h"
#include "logging.h"
#include "SignalState.h"
#include "OutputState.h"
#include "StateType.h"

extern Thermometer OutsideThermometer;
extern Thermometer UnderbellyThermometer;

int zoneID = 0;

HVACZone::HVACZone(HVACZoneConfig config) : zoneConfig(config)
{
    log_debug("Initializing HVACZone for %s", config.roomName);
    // Initialize all Heat Pump outputs HIGH (relays de‑energized → NC pass‑through)
    this->zoneConfig.pinFanLoRelay.set(Passthrough);
    this->zoneConfig.pinFanHiRelay.set(Passthrough);
    this->zoneConfig.pinACRelay.set(Passthrough);
    this->zoneConfig.pinHPRelay.set(Passthrough);
    // And turn off the SSR

    this->zoneConfig.pinSpaceHeaterRelay.set(SpaceHeaterOff);

    this->previousSignalState = SignalState();
}

SignalState HVACZone::readSignalState()
{
    SignalState currentSignalState = SignalState(
        zoneConfig.pinFanLoSense.read(),
        zoneConfig.pinFanHiSense.read(),
        zoneConfig.pinACSense.read(),
        zoneConfig.pinHPSense.read(),
        OutsideThermometer.getHysteresisMode(zoneConfig.hysteresisSetPoint_F),
        zoneConfig.hasFurnace ? zoneConfig.pinFurnaceSense.read() : false,
        zoneConfig.hasFurnace ? UnderbellyThermometer.getCallState(zoneConfig.underbellyThreshold) : false);

    log_debug(currentSignalState.consoleData(zoneConfig.hasFurnace));
    return currentSignalState;
}

OutputState HVACZone::readOutputState()
{
    OutputState currentOutputState = OutputState(
        zoneConfig.pinFanLoRelay.read(),
        zoneConfig.pinFanHiRelay.read(),
        zoneConfig.pinACRelay.read(),
        zoneConfig.pinHPRelay.read(),
        zoneConfig.pinSpaceHeaterRelay.read(),
        zoneConfig.hasFurnace ? zoneConfig.pinFurnaceRelay.read() : false,
        zoneConfig.hasFurnace ? zoneConfig.pinFurnacePowerRelay.read() : false,
        zoneConfig.hasFurnace ? zoneConfig.pinHPPowerRelay.read() : false);
    log_debug(currentOutputState.consoleData(zoneConfig.hasFurnace));
    return currentOutputState;
}

OutputState HVACZone::validateNewState(OutputState expectedNewState)
{
    OutputState actualNewState = this->readOutputState();
    if (actualNewState.bits != expectedNewState.bits)
    {
        this->fallbackToDefaultBehavior();
        actualNewState = this->readOutputState();
    }
    return actualNewState;
}

String HVACZone::DoLoop()
{
    SignalState currentSignalState = this->readSignalState();

    if (currentSignalState.bits != this->previousSignalState.bits)
    {
        log_info("Zone %s signal state changed: 0x%02X -> 0x%02X", zoneConfig.roomName, previousSignalState.bits, currentSignalState.bits);
        // Handle state change logic here
        OutputState newCalculatedState = this->CalculateNewOutputState(currentSignalState);
        this->updateOutputStates(newCalculatedState);
        OutputState actualNewState = this->validateNewState(newCalculatedState);
        this->previousSignalState = currentSignalState;
        String printableString = this->printPinStates(currentSignalState, newCalculatedState, actualNewState);
        return printableString;
    }
    else{
        return "";
    }
}

String HVACZone::printPinStates(SignalState currentSignalState, OutputState calculatedState, OutputState actualState)
{
    log_info("Signal: %s", currentSignalState.consoleData(zoneConfig.hasFurnace));
    log_debug("Expected Output = %s", calculatedState.consoleData(zoneConfig.hasFurnace));
    log_info("Output: %s", actualState.consoleData(zoneConfig.hasFurnace));

    int lcdLine = 2;
    int position = this->zoneConfig.lcdOffset;

    int bufferSize = 16;
    String signalStateStr = "";
    signalStateStr += currentSignalState.encode(zoneConfig.hasFurnace);
    signalStateStr += ":";
    signalStateStr += actualState.encode(zoneConfig.hasFurnace);
    signalStateStr += '\0';

    log_debug("Zone %s LCD Output: %s", this->zoneConfig.roomName, signalStateStr.c_str());
    return signalStateStr;
}

void HVACZone::updateOutputStates(OutputState newState)
{
    this->zoneConfig.pinFanLoRelay.set(newState.get(OutputState::Bit::FAN_LO));
    this->zoneConfig.pinFanHiRelay.set(newState.get(OutputState::Bit::FAN_HI));
    this->zoneConfig.pinACRelay.set(newState.get(OutputState::Bit::AC));
    this->zoneConfig.pinHPRelay.set(newState.get(OutputState::Bit::HEAT_PUMP));
    this->zoneConfig.pinSpaceHeaterRelay.set(newState.get(OutputState::Bit::SSR));
    if (this->zoneConfig.hasFurnace)
    {
        this->zoneConfig.pinFurnaceRelay.set(newState.get(OutputState::Bit::FURNACE));
        this->zoneConfig.pinFurnacePowerRelay.set(newState.get(OutputState::Bit::FURNACE_POWER));
        this->zoneConfig.pinHPPowerRelay.set(newState.get(OutputState::Bit::HEAT_PUMP_POWER));
    }
}

void HVACZone::fallbackToDefaultBehavior()
{
    log_info("Falling back to default behavior for %s", this->zoneConfig.roomName);
    OutputState newOutputState = OutputState(
        Passthrough, // fanLo
        Passthrough, // fanHi
        Passthrough, // ac
        Passthrough, // heatPump
        Passthrough, // furnace
        NoSupply12V, // furnacePower
        NoSupply12V, // heatPumpPower
        SpaceHeaterOff       // SSR
    );
    this->updateOutputStates(newOutputState);
}

OutputState HVACZone::CalculateNewOutputState(SignalState currentSignalState)
{
    ZoneOutputs o;
    bool hasFurnace    = this->zoneConfig.hasFurnace;
    bool hysteresisLow = (currentSignalState.get(SignalState::HYSTERESIS) == LOW);

    if (hysteresisLow) {
        this->applyHysteresisLowRules(currentSignalState, o, hasFurnace);
    } else {
        this->applyNormalModeRules(currentSignalState, o, hasFurnace);
    }

    return OutputState(
        o.fanLo,
        o.fanHi,
        o.ac,
        o.heatPump,
        o.spaceHeater,
        o.furnace,
        o.furnacePower,
        o.heatPumpPower
    );
}

void HVACZone::applyHysteresisLowRules(const SignalState& s, ZoneOutputs& o, bool hasFurnace) {
    bool furnaceCall    = s.get(SignalState::FURNACE);
    bool heatPumpCall   = s.get(SignalState::HEAT_PUMP);
    bool underbellyCall = s.get(SignalState::UBELLYCALL);

    // Block all HVAC signals
    o.fanLo   = BlockSignal;
    o.fanHi   = BlockSignal;
    o.ac      = BlockSignal;
    o.heatPump = BlockSignal;

    bool hasCallForHeat = (furnaceCall || heatPumpCall);

    if (hasCallForHeat) {
        o.spaceHeater = SpaceHeaterOn;
        if (hasFurnace) {
            o.furnace      = Passthrough;
            o.furnacePower = NoSupply12V;
        }
    } else {
        if (hasFurnace && underbellyCall) {
            o.furnace      = Supply12V;
            o.furnacePower = Supply12V;
        }
        o.spaceHeater = SpaceHeaterOff;
    }
}

void HVACZone::applyNormalModeRules(const SignalState& s, ZoneOutputs& o, bool hasFurnace) {
    bool furnaceCall    = s.get(SignalState::FURNACE);
    bool underbellyCall = s.get(SignalState::UBELLYCALL);

    // Never run space heater above threshold
    o.spaceHeater = SpaceHeaterOff;

    if (furnaceCall) {
        log_debug("Redirecting furnace call to heat pump");

        o.heatPumpPower = Supply12V;
        o.fanHi         = Supply12V;
        o.heatPump      = Supply12V;
        o.ac            = Passthrough;

        if (underbellyCall) {
            o.furnace      = Supply12V;
            o.furnacePower = Supply12V;
        } else {
            o.furnace      = NoSupply12V;
            o.furnacePower = NoSupply12V;
        }
    } else {
        // Pass everything through
        o.fanLo   = Passthrough;
        o.fanHi   = Passthrough;
        o.ac      = Passthrough;
        o.heatPump = Passthrough;

        if (hasFurnace) {
            o.heatPumpPower = NoSupply12V;

            if (underbellyCall) {
                o.furnace      = Supply12V;
                o.furnacePower = Supply12V;
            } else {
                o.furnace      = Passthrough;
                o.furnacePower = NoSupply12V;
            }
        }
    }
}