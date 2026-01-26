#pragma once
#include <Arduino.h>

struct SignalState {

    // Bit positions (MSB → LSB)
    enum Bit {
        UNUSED      = 7,
        UBELLYCALL  = 6,
        FURNACE     = 5,
        HYSTERESIS  = 4,
        FAN_LO      = 3,
        FAN_HI      = 2,
        AC          = 1,
        HEAT_PUMP   = 0
    };

    uint8_t bits = 0;

    SignalState(bool fanLo=false, bool fanHi=false, bool ac=false, bool heatPump=false,
                bool hysteresis=true, bool furnace=false, bool underbelly=false, bool unused=false)
    {
        bits = 0;

        bitWrite(bits, UNUSED,     unused);
        bitWrite(bits, UBELLYCALL, underbelly);
        bitWrite(bits, FURNACE,    furnace);
        bitWrite(bits, HYSTERESIS, hysteresis);
        bitWrite(bits, FAN_LO,     fanLo);
        bitWrite(bits, FAN_HI,     fanHi);
        bitWrite(bits, AC,         ac);
        bitWrite(bits, HEAT_PUMP,  heatPump);
    }

    bool get(Bit b) const {
        return (bits >> b) & 1;
    }

    // Human-readable description
    String BitByBit(bool hasFurnace) {
        String result = "[";

        if (hasFurnace) {
            // Group A
            result += get(UNUSED)     ? "       " : "       ";
            result += get(UBELLYCALL) ? "UndB+ " : "UndB- ";
            result += get(FURNACE)    ? "Furn+ "  : "Furn- ";
            result += "][";
        }

        // Group B
        result += get(HYSTERESIS) ? "Hys+ "  : "Hys- ";
        result += get(FAN_LO)     ? "Lo+ "   : "Lo- ";
        result += get(FAN_HI)     ? "Hi+ "   : "Hi- ";
        result += get(AC)         ? "AC+ "   : "AC- ";
        result += get(HEAT_PUMP)  ? "HP+ "   : "HP- ";
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
    const char* consoleData(bool hasFurnace) {
        static char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "0x%02X | %s | [%s] | '%s' | \"%s\"",
                 bits,
                 toBinary().c_str(),
                 BitByBit(hasFurnace).c_str(),
                 encode(hasFurnace).c_str(),
                 Description(hasFurnace).c_str());
        return buffer;
    }

    // Encoding
    String encode(bool hasFurnace) {
        char lowChar  = getLowBitCharacter();

        if(hasFurnace) {
            char highChar = getHighBitCharacter();
            char out[3];
            out[0] = highChar;
            out[1] = lowChar;
            out[2] = '\0';
            return String(out);
        }
        else {
            char out[2];
            out[0] = lowChar;
            out[1] = '\0';
            return String(out);
        }
    }

    String Description(bool hasFurnace) {
        if (hasFurnace)
            return getHighBitString() + "/" + getLowBitString();
        else  
            return getLowBitString();
    }

    // 3-bit high table (8 entries)
    char getHighBitCharacter() {
        uint8_t b = bits;

        uint8_t highBits =
            ((b >> UNUSED)     & 1) << 2 |
            ((b >> UBELLYCALL) & 1) << 1 |
            ((b >> FURNACE)    & 1);

        switch (highBits) {
            case 0b000: return '-';
            case 0b001: return 'F';
            case 0b010: return 'U';
            case 0b011: return 'B';
            default:    return '?';
        }
    }

    // 5-bit low table (32 entries)
    char getLowBitCharacter() {
        uint8_t lowBits = bits & 0x1F;

        switch (lowBits) {
            // Low Hysteresis
            case 0b00000: return '_';
            case 0b01000: return 'f';
            case 0b00100: return 'F';
            case 0b01010: return 'c';
            case 0b00110: return 'C';
            case 0b01001: return 'h';
            case 0b00101: return 'H';

            // High Hysteresis
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

    String getHighBitString() {
        char highBitChar = getHighBitCharacter();
        String result = "";
        result += highBitChar;
        result += " - ";

        switch (highBitChar) {
            case '-': result += "No Furnace or Underbelly Call"; break;
            case 'F': result += "Furnace Call"; break;
            case 'U': result += "Underbelly Call"; break;
            case 'B': result += "Both are Calling"; break;
            default:  result += "Invalid Case";
        }
        return result;
    }

    String getLowBitString() {
        char lowBitChar = getLowBitCharacter();
        String result = "";
        result += lowBitChar;
        result += " - ";

        switch (lowBitChar) {
            // Low Hysteresis
            case '_': result += "Low Hysteresis, Nothing On"; break;
            case 'f': result += "Low Hysteresis, Low Fan Only"; break;
            case 'F': result += "Low Hysteresis, Hi Fan Only"; break;
            case 'c': result += "Low Hysteresis, Low Fan Cool"; break;
            case 'C': result += "Low Hysteresis, Hi Fan Cool"; break;
            case 'h': result += "Low Hysteresis, Low Fan Heat"; break;
            case 'H': result += "Low Hysteresis, Hi Fan Heat"; break;

            // High Hysteresis
            case '-': result += "High Hysteresis, Nothing On"; break;
            case 'x': result += "High Hysteresis, Low Fan Only"; break;
            case 'X': result += "High Hysteresis, Hi Fan Only"; break;
            case 'y': result += "High Hysteresis, Low Fan Cool"; break;
            case 'Y': result += "High Hysteresis, Hi Fan Cool"; break;
            case 'z': result += "High Hysteresis, Low Fan Heat"; break;
            case 'Z': result += "High Hysteresis, Hi Fan Heat"; break;      
            default: result += "Invalid Heat Pump Inputs";
        }
        return result;
    }
};