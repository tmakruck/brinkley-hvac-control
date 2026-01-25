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

    OutputState(int fanLo=Passthrough.Value, int fanHi=Passthrough.Value, int ac=Passthrough.Value, int heatPump=Passthrough.Value, int spaceHeater=SSROff.Value, 
        int furnace=0, int furnacePower=NoSupply12V.Value, int heatPumpPower=NoSupply12V.Value) {
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
        char highChar = getHighBitCharacter();
        char lowChar  = getLowBitCharacter();

        String out;
        out += highChar;
        out += lowChar;
        return out;
    }

    String Description() {
        String highString = getHighBitString();
        String lowString  = getLowBitString();

        String out;
        out += highString;
        out += "/";
        out += lowString;
        return out;
    }

    // 3-bit high table (8 entries)
    char getHighBitCharacter() {
        uint8_t b = bits;

        // 3-bit high group
        uint8_t highBits =
            ((b >> HEAT_PUMP_POWER) & 1) << 2 |
            ((b >> FURNACE_POWER)   & 1) << 1 |
            ((b >> FURNACE)         & 1);

        switch (highBits) {
            case 0b000: return 'P'; // Furnace Passthrough
            case 0b001: return 'F'; // Furnace Bypass
            case 0b011: return 'M'; // Manually Run Furnace (Force On) like for Underbelly
            case 0b100: return 'H'; // Heat Pump Power Alternate, expected only if LowBit is also alternate
            case 0b111: return 'B'; // Heat Pump Power Alternate and Furnace Power Alternate (Probably High Hystersis but cold underbelly)
            
            //case 0b101: return 'N'; // Heat pump power alternate and no furnace - likely invalid
            //case 0b110: return '/'; // Invalid Combination - Heat Pump Power Alternate and Furnace Power but no Alternate
            //case 0b010: return 'A'; // Furnace Power Alternate - this alone doesn't make sense
            default:    return '?';
        }
    }
    
    // 5-bit low table (32 entries)
    char getLowBitCharacter() {
        uint8_t b = bits;

        // 5-bit low group
        uint8_t lowBits = b & 0x1F;
        switch (lowBits) {
            // SSR Off
            case 0b00000: return '_'; // Everything else is passthrough
            case 0b00001: return 'a';
            case 0b00010: return 'b';
            case 0b00011: return 'c';
            case 0b00100: return 'd';
            case 0b00101: return 'e';
            case 0b00110: return 'f';
            case 0b00111: return 'g';
            case 0b01000: return 'h';
            case 0b01001: return 'i';
            case 0b01010: return 'j';
            case 0b01011: return 'k';
            case 0b01100: return 'l';
            case 0b01101: return 'm';
            case 0b01110: return 'n';
            case 0b01111: return 'o'; // Everything else is alt state

            // SSR On
            case 0b10000: return 'S'; // Everything else is passthrough
            case 0b10001: return 'A';
            case 0b10010: return 'B';
            case 0b10011: return 'C';
            case 0b10100: return 'D';
            case 0b10101: return 'E';
            case 0b10110: return 'F';
            case 0b10111: return 'G';
            case 0b11000: return 'H';
            case 0b11001: return 'I';
            case 0b11010: return 'J';
            case 0b11011: return 'K';
            case 0b11100: return 'L';
            case 0b11101: return 'M';
            case 0b11110: return 'N';
            case 0b11111: return 'O'; // Everything else is alt state
            default: return '?';
        }
    }

    String getHighBitString(){        
        char highBitChar = getHighBitCharacter();
        String result ="";
        result += highBitChar;
        result += " - ";
        switch (highBitChar) {
            case 'P': result += "Furnace Passthrough"; break;
            case 'F': result += "Furnace Bypass"; break;
            case 'M': result += "Force Furnace"; break;
            case 'H': result += "Force Heat Pump"; break;
            case 'B': result += "Force Both"; break;

            default: result += "Invalid Case";
        }

        return result;
        
    }

    String getLowBitString(){       
        char lowBitChar = getLowBitCharacter();
        String result ="";
        result += lowBitChar;
        result += " - ";
        switch (lowBitChar) {
            // Low Hystersis mode
            case '_': result += "SSR Off, All Passthrough"; break;
            case 'S': result += "SSR On, All Passthrough"; break;
            case 'o': result += "SSR Off, All Alternate"; break;
            case 'O': result += "SSR On, All Alternate"; break;

            default: result += "Unknown Output State";
        }

        return result;
        
    }
};