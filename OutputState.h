#pragma once
#include "StateType.h"


struct OutputState {
    enum Bit {
        HEAT_PUMP_POWER = 7,
        FURNACE_POWER   = 6,
        FURNACE         = 5, 
        SSR             = 4,
        FAN_LO          = 3,
        FAN_HI          = 2,
        AC              = 1,
        HEAT_PUMP       = 0,
    };

    uint8_t bits = 0;

    OutputState() = default;

    OutputState(bool fanLo, bool fanHi, bool ac, bool heatPump, bool spaceHeater = false, 
        bool furnace=false, bool furnacePower = false, bool heatPumpPower = false) {
        bits =
            (heatPumpPower << HEAT_PUMP_POWER) |
            (furnacePower  << FURNACE_POWER)|
            (furnace       << FURNACE)      |
            (spaceHeater   << SSR)          |
            (fanLo      << FAN_LO)     |
            (fanHi      << FAN_HI)     |
            (ac         << AC)         |
            (heatPump   << HEAT_PUMP);
    }

    OutputState(StateType fanLo, StateType fanHi, StateType ac, StateType heatPump, StateType spaceHeater = SSROff,
                StateType furnace = Passthrough, StateType furnacePower = NoSupply12V, StateType heatPumpPower = NoSupply12V) {
        bits =
            (fanLo.Value         << FAN_LO)       |
            (fanHi.Value         << FAN_HI)       |
            (ac.Value            << AC)           |
            (heatPump.Value      << HEAT_PUMP)    |
            (spaceHeater.Value   << SSR)          |
            (furnace.Value       << FURNACE)      |
            (furnacePower.Value  << FURNACE_POWER)|
            (heatPumpPower.Value << HEAT_PUMP_POWER);
    }

    int get(Bit b) const {
        return (bits >> b) & 1;
    }

    // Human-readable description
    String BitByBit() {
        String result = "";

        result += "[";
        // Group A (left)
        result += (get(HEAT_PUMP_POWER)==NoSupply12V.Value ? "HPPwr- " : "HPPwr+ ");
        result += (get(FURNACE_POWER)==NoSupply12V.Value   ? "FPwr-"   : "FPwr+ ");
        result += (get(FURNACE)==Passthrough.Value         ? "Furn:P " : "Furn:Alt ");
         result += "][";
        // Group B (right)
        result += (get(SSR)==SSROn.Value                   ? "SSR:On " : "SSR:Off ");  // SSR is direct logic
        result += (get(FAN_LO)==Passthrough.Value          ? "FanLo:P " : "FanLo:Alt ");
        result += (get(FAN_HI)==Passthrough.Value          ? "FanHi:P " : "FanHi:Alt/12V ");
        result += (get(AC)==Passthrough.Value              ? "AC:P " : "AC:Alt ");
        result += (get(HEAT_PUMP)==Passthrough.Value       ? "HP:P " : "HP:Alt/12V ");
        result += "]";
        return result;
    }

    // Binary representation (MSB → LSB)
    String toBinary() {
        String result = "0b";
        for (int i = 7; i >= 0; i--) {
            result += (bits & (1 << i)) ? '1' : '0';
        }
        return result;
    }
    // Safe console output
    const char* consoleData() {
        static char buffer[200];
        snprintf(buffer, sizeof(buffer),
                 "0x%02X | %s | [%s] | '%s' | \"%s\"",
                 bits,
                 toBinary().c_str(),
                 BitByBit().c_str(),
                 encode().c_str(),
                 Description().c_str());
        return buffer;
    }

    // Encoding
    String encode() {
        uint8_t b = bits;

        // 3-bit high group
        uint8_t highBits =
            ((b >> HEAT_PUMP_POWER) & 1) << 2 |
            ((b >> FURNACE_POWER)   & 1) << 1 |
            ((b >> FURNACE)         & 1);

        // 5-bit low group
        uint8_t lowBits = b & 0x1F;

        char highChar = getHighBitCharacter(highBits);
        char lowChar  = getLowBitCharacter(lowBits);

        String out;
        out += highChar;
        out += lowChar;
        return out;
    }

    String Description() {
        uint8_t b = bits;

        // 3-bit high group
        uint8_t highBits =
            ((b >> HEAT_PUMP_POWER) & 1) << 2 |
            ((b >> FURNACE_POWER)   & 1) << 1 |
            ((b >> FURNACE)         & 1);

        // 5-bit low group
        uint8_t lowBits = b & 0x1F;

        String highString = getHighBitString(highBits);
        String lowString  = getLowBitString(lowBits);

        String out;
        out += highString;
        out += "/";
        out += lowString;
        return out;
    }

    // 3-bit high table (8 entries)
    char getHighBitCharacter(uint8_t bits) {
        switch (bits) {
            case 0b000: return 'P'; // Furnace Passthrough
            case 0b001: return 'F'; // Fur
            case 0b010: return 'U';
            case 0b011: return 'B';
            case 0b100: return '/';
            case 0b101: return '/';
            case 0b110: return '/';
            case 0b111: return '/';
            default:    return '?';
        }
    }
    
    // 5-bit low table (32 entries)
    char getLowBitCharacter(uint8_t bits) {
        switch (bits) {
            // Low Hystersis mode
            case 0b00000: return '_';
            case 0b01000: return 'f';
            case 0b00100: return 'F';
            case 0b01010: return 'c';
            case 0b00110: return 'C';
            case 0b01001: return 'h';
            case 0b00101: return 'H';

            // High hysteresis versions
            case 0b10000: return '-';
            case 0b11000: return 'x';
            case 0b10100: return 'X';
            case 0b11010: return 'y';
            case 0b10110: return 'Y';
            case 0b11001: return 'z';
            case 0b10101: return 'Z';

            default: return '?';
        }
    }

    String getHighBitString(uint8_t bits){        
        char highBitChar = getHighBitCharacter(bits);
        String result ="";
        result += highBitChar;
        result += " - ";
        switch (highBitChar) {
            // Low Hystersis mode
            case '-': result += "No Furnace or Underbelly Call"; break;
            case 'F': result += "Furnace Call"; break;
            case 'U': result += "Underbelly Call"; break;
            case 'B': result += "Both are Calling"; break;

            default: result += "Invalid Case";
        }

        return result;
        
    }
   
    String getLowBitString(uint8_t bits){       
        char lowBitChar = getLowBitCharacter(bits);
        String result ="";
        result += lowBitChar;
        result += " - ";
        switch (lowBitChar) {
            // Low Hystersis mode
            case '_': result += "Low Hystersis, Nothing On"; break;
            case 'f': result += "Low Hystersis, Low Fan Only"; break;
            case 'F': result += "Low Hystersis, Hi Fan Only"; break;
            case 'c': result += "Low Hystersis, Low Fan Cool"; break;
            case 'C': result += "Low Hystersis, Hi Fan Cool"; break;
            case 'h': result += "Low Hystersis, Low Fan Heat"; break;
            case 'H': result += "Low Hystersis, Hi Fan Heat"; break;

            // High hysteresis versions
            case '-': result += "High Hystersis, Nothing On"; break;
            case 'x': result += "High Hystersis, Low Fan Only"; break;
            case 'X': result += "High Hystersis, Hi Fan Only"; break;
            case 'y': result += "High Hystersis, Low Fan Cool"; break;
            case 'Y': result += "High Hystersis, Hi Fan Cool"; break;
            case 'z': result += "High Hystersis, Low Fan Heat"; break;
            case 'Z': result += "High Hystersis, Hi Fan Heat"; break;

            default: result += "Invalid Heat Pump Inputs";
        }

        return result;
        
    }
};