#include "HvacZone.h"
#include "Thermometer.h"
#include "HVACZoneConfig.h"
#include "logging.h"
#include "SignalState.h"
#include "OutputState.h"
#include "StateType.h"

extern Thermometer OutsideThermometer;
extern Thermometer UnderbellyThermometer;

int zoneID=0;

HVACZone::HVACZone(HVACZoneConfig config) : zoneConfig(config){
    log_info("Initializing HVACZone for %s", config.roomName);
    // Initialize all Heat Pump outputs HIGH (relays de‑energized → NC pass‑through)
    this->zoneConfig.pinFanLoOut.set(Passthrough);
    this->zoneConfig.pinFanHiOut.set(Passthrough);
    this->zoneConfig.pinACOut.set(Passthrough);
    this->zoneConfig.pinHPOut.set(Passthrough);
    // And turn off the SSR

    this->zoneConfig.pinSSR.set(SSROff);

    this->previousInputState = SignalState(false, false, false, false, false, false, false);
}

SignalState HVACZone::readSignalState() {
    SignalState currentSignalState = SignalState(
        zoneConfig.pinFanLoSense.read(),
        zoneConfig.pinFanHiSense.read(),
        zoneConfig.pinACSense.read(),
        zoneConfig.pinHPSense.read(),
        OutsideThermometer.requiresHeatingMode(zoneConfig.hysteresisSetPoint_F),
        zoneConfig.hasFurnace ? zoneConfig.pinFurnaceSense.read() : false,
        zoneConfig.hasFurnace ? UnderbellyThermometer.requiresHeatingMode(zoneConfig.underbellyThreshold) : false
    );

    currentSignalState.consoleLine();
    log_debug(currentSignalState.consoleData());
    return currentSignalState;
}

OutputState HVACZone::readOutputState() {
    OutputState currentOutputState = OutputState(
        zoneConfig.pinFanLoOut.read(),
        zoneConfig.pinFanHiOut.read(),
        zoneConfig.pinACOut.read(),
        zoneConfig.pinHPOut.read(),
        zoneConfig.pinSSR.read(),
        zoneConfig.hasFurnace ? zoneConfig.pinFurnaceOut.read() : false,
        zoneConfig.hasFurnace ? zoneConfig.pinFurnacePower.read() : false,
        zoneConfig.hasFurnace ? zoneConfig.pinHPPower.read() : false
    );
    log_debug(currentOutputState.consoleData());
    return currentOutputState;
}

OutputState HVACZone::validateNewState(OutputState expectedNewState){
  OutputState actualNewState = this->readOutputState();
  if (actualNewState.bits != expectedNewState.bits){
    this->fallbackToDefaultBehavior();
    actualNewState = this->readOutputState();
  }
  return actualNewState;
}

void HVACZone::DoLoop(){
    SignalState currentInputState = this->readSignalState();

    if (currentInputState.bits != this->previousInputState.bits) {
        log_info("Zone %s signal state changed: 0x%02X -> 0x%02X", zoneConfig.roomName, previousInputState.bits, currentInputState.bits);
        // Handle state change logic here
        OutputState newCalculatedState = this->CalculateNewOutputState(currentInputState);
        this->updateOutputStates(newCalculatedState);
        OutputState actualNewState = this->validateNewState(newCalculatedState);

        this->printPinStates(currentInputState, newCalculatedState, actualNewState);
        this->previousInputState = currentInputState;

    }
}

void HVACZone::printPinStates(SignalState currentInputState, OutputState calculatedState, OutputState actualState) {
  //TODO get single-character status for each zone and print to LCD
  log_info("Current Input State   = %s", currentInputState.consoleData());
  log_info("Expected Output State = %s", calculatedState.consoleData());
  log_info("Actual Output State   = %s", actualState.consoleData()); 
}

void HVACZone::updateOutputStates(OutputState newState){
    this->zoneConfig.pinFanLoOut.set(newState.get(OutputState::Bit::FAN_LO));
    this->zoneConfig.pinFanHiOut.set(newState.get(OutputState::Bit::FAN_HI));
    this->zoneConfig.pinACOut.set(newState.get(OutputState::Bit::AC));
    this->zoneConfig.pinHPOut.set(newState.get(OutputState::Bit::HEAT_PUMP));
    this->zoneConfig.pinSSR.set(newState.get(OutputState::Bit::SSR));
    if (this->zoneConfig.hasFurnace){
        this->zoneConfig.pinFurnaceOut.set(newState.get(OutputState::Bit::FURNACE));
        this->zoneConfig.pinFurnacePower.set(newState.get(OutputState::Bit::FURNACE_POWER));
        this->zoneConfig.pinHPPower.set(newState.get(OutputState::Bit::HEAT_PUMP_POWER));
    }
}

void HVACZone::fallbackToDefaultBehavior(){
  log_info("Falling back to default behavior for %s", this->zoneConfig.roomName);
  OutputState newOutputState = OutputState(
    Passthrough, // fanLo
    Passthrough, // fanHi
    Passthrough, // ac
    Passthrough, // heatPump
    Passthrough, // furnace
    NoSupply12V,  // furnacePower
    NoSupply12V,  // heatPumpPower
    SSROff       // SSR
  );
  this->updateOutputStates(newOutputState);
}

OutputState HVACZone::CalculateNewOutputState(SignalState currentSignalState){
  bool hasFurnace = this->zoneConfig.hasFurnace;
  bool isFurnaceCall = currentSignalState.get(SignalState::Bit::FURNACE);
  bool furnaceCallActive = hasFurnace && isFurnaceCall;

  bool isHeatPumpCall = currentSignalState.get(SignalState::Bit::HEAT_PUMP);
  bool heatPumpCallActive = isHeatPumpCall;

  bool underbellyTooCold = currentSignalState.get(SignalState::Bit::UNDERBELLY);
  bool hasCallForHeat = (furnaceCallActive || heatPumpCallActive);
  bool isHysteresisLowMode = currentSignalState.get(SignalState::Bit::HYSTERESIS);

  bool isFrigid = OutsideThermometer.IsFrigid();

  StateType newFanLoRelayState = Passthrough;
  StateType newFanHiRelayState = Passthrough;
  StateType newACRelayState = Passthrough;
  StateType newHeatPumpRelayState = Passthrough;
  StateType newSSRState = SSROff;
  StateType newFurnaceRelayState = Passthrough;
  StateType newFurnacePowerRelayState = NoSupply12V;
  StateType newHeatPumpPowerRelayState = NoSupply12V;

  if (isHysteresisLowMode){
    // the temps are cold enough to use the space heaters
    // don't pass the heat pump through
    newFanLoRelayState = BlockSignal;
    newFanHiRelayState = BlockSignal;
    newACRelayState = BlockSignal;
    newHeatPumpRelayState = BlockSignal;

    if (hasCallForHeat){
      // turn on the ssr for the zone
      newSSRState = SSROn;
      if(hasFurnace) {
        newFurnaceRelayState = Passthrough;
        newFurnacePowerRelayState = NoSupply12V;
      }
    }
    else {

      if (hasFurnace && underbellyTooCold){
        newFurnaceRelayState = Supply12V;
        newFurnacePowerRelayState = Supply12V;
      }
      if (hasFurnace && isFrigid){
        newSSRState = SSROn;
      } else {
        newSSRState = SSROff;
      }
    }
  }

  else{ // NOT HysteresisLowMode
    // never run space heater above threshold
    // never run furnace above threshold
    // always pass heat pump through
    newSSRState = SSROff;
    
    if (furnaceCallActive) {
      log_debug("Redirecting furnace call to heat pump");
      newHeatPumpPowerRelayState = Supply12V;
      newFanHiRelayState = Supply12V;
      newHeatPumpRelayState = Supply12V;
      newACRelayState = Passthrough;
      

      if (underbellyTooCold) {
        newFurnacePowerRelayState = Supply12V;
        newFurnaceRelayState = Supply12V;
      } else {
        newFurnacePowerRelayState = NoSupply12V;
        newFurnaceRelayState = NoSupply12V;
      }
      
    }
    else{
      newFanLoRelayState = Passthrough;
      newFanHiRelayState = Passthrough;
      newACRelayState = Passthrough;
      newHeatPumpRelayState = Passthrough;

      if (hasFurnace){
        newHeatPumpPowerRelayState = NoSupply12V;
        if (underbellyTooCold && !isFurnaceCall){
          newFurnaceRelayState = Supply12V;
          newFurnacePowerRelayState = Supply12V;
        } else {
          newFurnaceRelayState = Passthrough;
          newFurnacePowerRelayState = NoSupply12V;
        }
        
      }

    }     
  }

  OutputState newOutputState = OutputState(newFanLoRelayState, newFanHiRelayState, newACRelayState, newHeatPumpRelayState, newSSRState, 
              newFurnaceRelayState, newFurnacePowerRelayState, newHeatPumpPowerRelayState);
  return newOutputState;
}
