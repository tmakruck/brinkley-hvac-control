#pragma once
#include "StateType.h"


struct OutputState {
    enum Bit {
        FAN_LO          = 0,
        FAN_HI          = 1,
        AC              = 2,
        HEAT_PUMP       = 3,
        SSR             = 4,
        FURNACE         = 5, 
        FURNACE_POWER   = 6,
        HEAT_PUMP_POWER = 7
    };
    uint8_t bits = 0;

    OutputState() = default;
    OutputState(bool relay_fanLo, bool relay_fanHi, bool relay_ac, bool relay_heatPump, bool relay_spaceHeater = false, 
        bool relay_furnace=false, bool relay_furnacePower = false, bool relay_heatPumpPower = false) {
        bits =
            (relay_fanLo         << FAN_LO)       |
            (relay_fanHi         << FAN_HI)       |
            (relay_ac            << AC)           |
            (relay_heatPump      << HEAT_PUMP)    |
            (relay_spaceHeater   << SSR)          |
            (relay_furnace       << FURNACE)      |
            (relay_furnacePower  << FURNACE_POWER)|
            (relay_heatPumpPower << HEAT_PUMP_POWER);
    }

    OutputState(StateType relay_fanLo, StateType relay_fanHi, StateType relay_ac, StateType relay_heatPump, StateType relay_spaceHeater = SSROff,
                StateType relay_furnace = Passthrough, StateType relay_furnacePower = NoSupply12V, StateType relay_heatPumpPower = NoSupply12V) {
       
        OutputState(
            relay_fanLo.Value,
            relay_fanHi.Value,
            relay_ac.Value,
            relay_heatPump.Value,
            relay_spaceHeater.Value,
            relay_furnace.Value,
            relay_furnacePower.Value,
            relay_heatPumpPower.Value
        );
    }

    bool get(Bit b) const {
        return (bits >> b) & 1;
    }

    char encode(const OutputState& s) {
        return static_cast<char>(s.bits + 128);
    }



};
