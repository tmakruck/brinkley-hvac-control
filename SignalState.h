#pragma once

struct SignalState {
    enum Bit {
        FAN_LO      = 0,
        FAN_HI      = 1,
        AC          = 2,
        HEAT_PUMP   = 3,
        HYSTERESIS  = 4,
        FURNACE     = 5, 
        UNDERBELLY  = 6
    };
    uint8_t bits = 0;

    SignalState() = default;
    SignalState(bool fanLo, bool fanHi, bool ac, bool heatPump, bool hysteresis=false, bool furnace=false, bool underbelly = false) {
        bits =
            (fanLo      << FAN_LO)    |
            (fanHi      << FAN_HI)    |
            (ac         << AC)        |
            (heatPump   << HEAT_PUMP) |
            (hysteresis << HYSTERESIS)|
            (furnace    << FURNACE)   |
            (underbelly << UNDERBELLY);
    }

    bool get(Bit b) const {
        return (bits >> b) & 1;
    }

    String encode() {
        uint8_t b = bits;

        // Extract 2-bit high pattern
        uint8_t highBits =
            ((b >> SignalState::FURNACE)    & 1) << 1 |
            ((b >> SignalState::UNDERBELLY) & 1);

        // Extract 5-bit low pattern
        uint8_t lowBits = b & 0x1F;

        char highChar = HIGH_LIST[highBits];
        char lowChar  = getLowbitCharacter(lowBits);

        String out;
        out += highChar;
        out += lowChar;
        return out;
    }


    char HIGH_LIST[4] = {
        '-', // 00
        'F', // 01
        'U', // 10
        'B', // 11
    };

    char getHighBitCharacter(uint8_t bits){
        switch (bits){

            case 0b00: return '_'; // no furnace or underbelly calls
            case 0b01: return 'F'; //Furnace Call
            case 0b10: return 'U'; // Underbelly Call
            case 0b11: return 'B'; // Both Furnace AND Underbelly Call
            default:      return 'X'; // Invalid State
        };
    }

    char getLowbitCharacter(uint8_t bits){
        switch (bits){
            case 0b0000: return '_'; // Everything off
            case 0b0100: return 'F'; // Fan Hi
            case 0b0101: return 'H'; // Heat + Fan Hi
            case 0b0110: return 'C'; // Cool + Fan Hi
            case 0b1000: return 'f'; // Fan Lo
            case 0b1001: return 'h'; // Heat + Fan Lo
            case 0b1010: return 'c'; // Cool + Fan Lo
            
            // High Hysteresis
            case 0b10000: return '-'; // Everything off
            case 0b10100: return 'F'; // Fan Hi
            case 0b10101: return 'H'; // Heat + Fan Hi
            case 0b10110: return 'C'; // Cool + Fan Hi
            case 0b11000: return 'f'; // Fan Lo
            case 0b11001: return 'h'; // Heat + Fan Lo
            case 0b11010: return 'c'; // Cool + Fan Lo

            default:      return 'X'; // Unknown or invalid State

        };
    }
};
