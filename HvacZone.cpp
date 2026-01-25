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
    log_info("Initializing HVACZone for %s", config.roomName);
    // Initialize all Heat Pump outputs HIGH (relays de‑energized → NC pass‑through)
    this->zoneConfig.pinFanLoRelay.set(Passthrough);
    this->zoneConfig.pinFanHiRelay.set(Passthrough);
    this->zoneConfig.pinACRelay.set(Passthrough);
    this->zoneConfig.pinHPRelay.set(Passthrough);
    // And turn off the SSR

    this->zoneConfig.pinSpaceHeaterRelay.set(SSROff);

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

    log_debug(currentSignalState.consoleData());
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
    log_debug(currentOutputState.consoleData());
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
    log_info("Current Signal State   = %s", currentSignalState.consoleData());
    log_info("Expected Output State = %s", calculatedState.consoleData());
    log_info("Actual Output State   = %s", actualState.consoleData());

    int lcdLine = 2;
    int position = this->zoneConfig.lcdOffset;

    int bufferSize = 16;
    String signalStateStr = "";
    if (this->zoneConfig.hasFurnace)
    {
        signalStateStr += currentSignalState.getHighBitCharacter();
    }
    signalStateStr += currentSignalState.getLowBitCharacter();
    signalStateStr += ":";
    signalStateStr += actualState.encode();
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
        SSROff       // SSR
    );
    this->updateOutputStates(newOutputState);
}

OutputState HVACZone::CalculateNewOutputState(SignalState currentSignalState)
{
    bool hasFurnace = this->zoneConfig.hasFurnace;
    bool isFurnaceCall = currentSignalState.get(SignalState::Bit::FURNACE);
    bool isHeatPumpCall = currentSignalState.get(SignalState::Bit::HEAT_PUMP);
    
    bool furnaceCallActive = hasFurnace && isFurnaceCall;

    bool heatPumpCallActive = isHeatPumpCall;

    bool isUnderbellyCall = currentSignalState.get(SignalState::Bit::UBELLYCALL);
    bool hasCallForHeat = (furnaceCallActive || isHeatPumpCall);

    StateType newFanLoRelayState = Passthrough;
    StateType newFanHiRelayState = Passthrough;
    StateType newACRelayState = Passthrough;
    StateType newHeatPumpRelayState = Passthrough;
    StateType newSpaceHeaterState = SSROff;
    StateType newFurnaceRelayState = Passthrough;
    StateType newFurnacePowerRelayState = NoSupply12V;
    StateType newHeatPumpPowerRelayState = NoSupply12V;

    if (currentSignalState.get(SignalState::Bit::HYSTERESIS) == LOW)
    {
        // the temps are cold enough to use the space heaters
        // don't pass the heat pump through
        newFanLoRelayState = BlockSignal;
        newFanHiRelayState = BlockSignal;
        newACRelayState = BlockSignal;
        newHeatPumpRelayState = BlockSignal;

        if (hasCallForHeat)
        {
            // turn on the ssr for the zone
            newSpaceHeaterState = SpaceHeaterOn;
            if (hasFurnace)
            {
                newFurnaceRelayState = Passthrough;
                newFurnacePowerRelayState = NoSupply12V;
            }
        }
        else
        {

            if (hasFurnace && isUnderbellyCall)
            {
                newFurnaceRelayState = Supply12V;
                newFurnacePowerRelayState = Supply12V;
            }
            newSpaceHeaterState = SSROff;
        }
    }

    else
    { // NOT HysteresisLowMode
        // never run space heater above threshold
        // never run furnace above threshold
        // always pass heat pump through
        newSpaceHeaterState = SSROff;

        if (furnaceCallActive)
        {
            log_debug("Redirecting furnace call to heat pump");
            newHeatPumpPowerRelayState = Supply12V;
            newFanHiRelayState = Supply12V;
            newHeatPumpRelayState = Supply12V;
            newACRelayState = Passthrough;

            if (isUnderbellyCall)
            {
                newFurnacePowerRelayState = Supply12V;
                newFurnaceRelayState = Supply12V;
            }
            else
            {
                newFurnacePowerRelayState = NoSupply12V;
                newFurnaceRelayState = NoSupply12V;
            }
        }
        else
        {
            newFanLoRelayState = Passthrough;
            newFanHiRelayState = Passthrough;
            newACRelayState = Passthrough;
            newHeatPumpRelayState = Passthrough;

            if (hasFurnace)
            {
                newHeatPumpPowerRelayState = NoSupply12V;
                if (isUnderbellyCall && !isFurnaceCall)
                {
                    newFurnaceRelayState = Supply12V;
                    newFurnacePowerRelayState = Supply12V;
                }
                else
                {
                    newFurnaceRelayState = Passthrough;
                    newFurnacePowerRelayState = NoSupply12V;
                }
            }
        }
    }

    OutputState newOutputState = OutputState(newFanLoRelayState, newFanHiRelayState, newACRelayState, newHeatPumpRelayState, newSpaceHeaterState,
                                             newFurnaceRelayState, newFurnacePowerRelayState, newHeatPumpPowerRelayState);
    return newOutputState;
}
