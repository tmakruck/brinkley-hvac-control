#pragma once
enum InputBit {
    FAN_LO      = 0,
    FAN_HI      = 1,
    AC          = 2,
    HEAT_PUMP   = 3,
    FURNACE     = 4, 
    UNDERBELLY  = 5,
    HYSTERESIS  = 6
};
struct SignalState {

    uint8_t bits = 0;

    SignalState() = default;
    SignalState(bool fanLo, bool fanHi, bool ac, bool heatPump, bool furnace, bool underbelly = false, bool hysteresis = false) {
        bits =
            (fanLo      << FAN_LO)    |
            (fanHi      << FAN_HI)    |
            (ac         << AC)        |
            (heatPump   << HEAT_PUMP) |
            (furnace    << FURNACE)   |
            (underbelly << UNDERBELLY)|
            (hysteresis << HYSTERESIS);
    }

    bool get(InputBit b) const {
        return (bits >> b) & 1;
    }
};
