#pragma once

struct SignalState {
    enum Bit {
        FAN_LO      = 0,
        FAN_HI      = 1,
        AC          = 2,
        HEAT_PUMP   = 3,
        FURNACE     = 4, 
        UNDERBELLY  = 5,
        HYSTERESIS  = 6
    };
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

    bool get(Bit b) const {
        return (bits >> b) & 1;
    }

    String encode() {
        uint8_t b = bits;

        // Extract 3-bit high pattern
        uint8_t highBits =
            ((b >> SignalState::HYSTERESIS) & 1) << 2 |
            ((b >> SignalState::FURNACE)    & 1) << 1 |
            ((b >> SignalState::UNDERBELLY) & 1);

        // Extract 4-bit low pattern
        uint8_t lowBits = b & 0x0F;

        char highChar = HIGH_LIST[highBits];
        char lowChar  = LOW_LIST[lowBits];

        String out;
        out += highChar;
        out += lowChar;
        return out;
    }


    char HIGH_LIST[8] = {
        'h', // 000
        'u', // 001
        'f', // 010
        'b', // 011
        'H', // 100
        'U', // 101
        'F', // 110
        'B', // 111
    };

    char LOW_LIST[16] = {
        '-', // 0000
        'z', // 0001
        'y', // 0010
        'x', // 0011
        'F', // 0100
        'H', // 0101
        'C', // 0110
        'w', // 0111
        'f', // 1000
        'h', // 1001
        'c', // 1010
        'r', // 1011
        'v', // 1100
        'u', // 1101
        't', // 1110
        's', // 1111
    };



};
