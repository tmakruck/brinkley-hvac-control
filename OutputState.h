#pragma once
#include "StateType.h"


struct OutputState {
    enum OutputBit {
        FAN_LO          = 0,
        FAN_HI          = 1,
        AC              = 2,
        HEAT_PUMP       = 3,
        FURNACE         = 4, 
        FURNACE_POWER   = 5,
        HEAT_PUMP_POWER = 6,
        SSR             = 7
    };
    uint8_t bits = 0;

    OutputState() = default;
    OutputState(bool fanLo, bool fanHi, bool ac, bool heatPump, bool furnace, bool furnacePower = false, bool heatPumpPower = false, bool ssr = false) {
        bits =
            (fanLo      << FAN_LO)    |
            (fanHi      << FAN_HI)    |
            (ac         << AC)        |
            (heatPump   << HEAT_PUMP) |
            (furnace    << FURNACE)   |
            (furnacePower << FURNACE_POWER)|
            (heatPumpPower << HEAT_PUMP_POWER) |
            (ssr << SSR);
    }

    OutputState(StateType fanLo, StateType fanHi, StateType ac, StateType heatPump, StateType furnace, 
                StateType furnacePower = NoSupply12V, StateType heatPumpPower = NoSupply12V, StateType ssr = SSROff) {
       
        OutputState(
            fanLo.Value,
            fanHi.Value,
            ac.Value,
            heatPump.Value,
            furnace.Value,
            furnacePower.Value,
            heatPumpPower.Value,
            ssr.Value
        );
    }

    bool get(OutputBit b) const {
        return (bits >> b) & 1;
    }
};
