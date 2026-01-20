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
    // Initialize all Heat Pump outputs HIGH (relays de‑energized → NC pass‑through)
    this->zoneConfig.pinFanLoOut.set(Passthrough);
    this->zoneConfig.pinFanHiOut.set(Passthrough);
    this->zoneConfig.pinACOut.set(Passthrough);
    this->zoneConfig.pinHPOut.set(Passthrough);
    // And turn off the SSR

    this->zoneConfig.pinSSR.set(SSROff);

    this->lastState = SignalState(false, false, false, false, false, false, false);
}

SignalState HVACZone::readSignalState() {
    SignalState currentSignalState = SignalState(
        zoneConfig.pinFanLoSense.read(),
        zoneConfig.pinFanHiSense.read(),
        zoneConfig.pinACSense.read(),
        zoneConfig.pinHPSense.read(),
        zoneConfig.hasFurnace ? zoneConfig.pinFurnaceSense.read() : false,
        zoneConfig.hasFurnace ? UnderbellyThermometer.requiresHeatingMode(zoneConfig.underbellyThreshold) : false,
        OutsideThermometer.requiresHeatingMode(zoneConfig.hysteresisSetPoint_F)
    );
    log_debug("Current Signal State: 0x%02X", currentSignalState.bits);
    return currentSignalState;
}

OutputState HVACZone::readOutputState() {
    OutputState currentOutputState = OutputState(
        zoneConfig.pinFanLoOut.read(),
        zoneConfig.pinFanHiOut.read(),
        zoneConfig.pinACOut.read(),
        zoneConfig.pinHPOut.read(),
        zoneConfig.hasFurnace ? zoneConfig.pinFurnaceOut.read() : false,
        zoneConfig.hasFurnace ? zoneConfig.pinFurnacePower.read() : false,
        zoneConfig.hasFurnace ? zoneConfig.pinHPPower.read() : false,
        zoneConfig.pinSSR.read()
    );
    log_debug("Current Output State: 0x%02X", currentOutputState.bits);
    return currentOutputState;
}

void HVACZone::DoLoop(){
    SignalState prevState = lastState;
    SignalState currState = this->readSignalState();

    if (currState.bits != prevState.bits) {
        log_info("Zone %s signal state changed: 0x%02X -> 0x%02X", zoneConfig.roomName, prevState.bits, currState.bits);
        lastState = currState;

        // Handle state change logic here
        this->CalculateNewOutputState(currState);
        this->printPinStates();
    }
}

void HVACZone::printPinStates() {
  //TODO get single-character status for each zone and print to LCD
}

void HVACZone::updateOutputStates(OutputState newState){
    this->zoneConfig.pinFanLoOut.set(newState.get(OutputState::OutputBit::FAN_LO));
    this->zoneConfig.pinFanHiOut.set(newState.get(OutputState::OutputBit::FAN_HI));
    this->zoneConfig.pinACOut.set(newState.get(OutputState::OutputBit::AC));
    this->zoneConfig.pinHPOut.set(newState.get(OutputState::OutputBit::HEAT_PUMP));
    this->zoneConfig.pinSSR.set(newState.get(OutputState::OutputBit::SSR));
    if (this->zoneConfig.hasFurnace){
        this->zoneConfig.pinFurnaceOut.set(newState.get(OutputState::OutputBit::FURNACE));
        this->zoneConfig.pinFurnacePower.set(newState.get(OutputState::OutputBit::FURNACE_POWER));
        this->zoneConfig.pinHPPower.set(newState.get(OutputState::OutputBit::HEAT_PUMP_POWER));
    }
}

void HVACZone::fallbackToDefaultBehavior(){
  log_debug("Falling back to default behavior for %s", this->zoneConfig.roomName);
  OutputState newOutputState = OutputState(
    Passthrough, // fanLo
    Passthrough, // fanHi
    Passthrough, // ac
    Passthrough, // heatPump
    NoPassthrough, // furnace
    NoSupply12V,  // furnacePower
    NoSupply12V,  // heatPumpPower
    SSROff       // SSR
  );
  this->updateOutputStates(newOutputState);
}

void HVACZone::CalculateNewOutputState(SignalState currentSignalState){
  bool hasFurnace = this->zoneConfig.hasFurnace;
  bool isFurnaceCall = currentSignalState.get(InputBit::FURNACE);
  bool furnaceCallActive = hasFurnace && isFurnaceCall;

  bool isHeatPumpCall = currentSignalState.get(InputBit::HEAT_PUMP);
  bool heatPumpCallActive = isHeatPumpCall;

  bool underbellyTooCold = currentSignalState.get(InputBit::UNDERBELLY);
  bool hasCallForHeat = (furnaceCallActive || heatPumpCallActive);

  StateType newFanLoRelayState = Passthrough;
  StateType newFanHiRelayState = Passthrough;
  StateType newACRelayState = Passthrough;
  StateType newHeatPumpRelayState = Passthrough;
  StateType newFurnaceRelayState = Passthrough;
  StateType newFurnacePowerRelayState = NoSupply12V;
  StateType newHeatPumpPowerRelayState = NoSupply12V;
  StateType newSSRState = SSROff;

  if (isHysteresisLowMode){
    // the temps are cold enough to use the space heaters
    // control the space heater.
    // don't pass the heat pump through
    newFanLoRelayState = NoPassthrough;
    newFanHiRelayState = NoPassthrough;
    newACRelayState = NoPassthrough;
    newHeatPumpRelayState = NoPassthrough;

    if (hasCallForHeat){
      // turn on the ssr for the zone
      newSSRState = SSROn;
      // 
      if(hasFurnace) {
        newFurnaceRelayState = Passthrough;
        newFurnacePowerRelayState = NoSupply12V;
      }
    }
    else {
      newSSRState = SSROff;
      if (hasFurnace && underbellyTooCold){
        newFurnaceRelayState = NoPassthrough;
        newFurnacePowerRelayState = Supply12V;
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
      newFanHiRelayState = NoPassthrough;
      newHeatPumpRelayState = NoPassthrough;
      //setFurnaceRelayState(OutputState::NoPassthrough);
      newFurnaceRelayState = NoPassthrough;

      if (underbellyTooCold) {
        newFurnacePowerRelayState = Supply12V;
      } else {
        newFurnacePowerRelayState = NoSupply12V;
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
          newFurnaceRelayState = NoPassthrough;
          newFurnacePowerRelayState = Supply12V;
        } else {
          
          newFurnaceRelayState = Passthrough;
          newFurnacePowerRelayState = NoSupply12V;
        }
        
      }

    }     
  }
}
