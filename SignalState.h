#pragma once
#include <Arduino.h>

namespace SignalStateEncoding {
    struct HighBitInfo {
        uint8_t bits;
        char character;
        const char* description;
    };

    struct LowBitInfo {
        uint8_t bits;
        char character;
        const char* description;
    };

    static const HighBitInfo HIGH_BIT_TABLE[] = {
        { 0b000, '-', "No Furnace or Underbelly Call" },
        { 0b001, 'F', "Furnace Call" },
        { 0b010, 'U', "Underbelly Call" },
        { 0b011, 'B', "Both are Calling" }
    };

    static const LowBitInfo LOW_BIT_TABLE[] = {
        { 0b00000, '_', "Low Hysteresis, Nothing On" },
        { 0b01000, 'f', "Low Hysteresis, Low Fan Only" },
        { 0b00100, 'F', "Low Hysteresis, Hi Fan Only" },
        { 0b01010, 'c', "Low Hysteresis, Low Fan Cool" },
        { 0b00110, 'C', "Low Hysteresis, Hi Fan Cool" },
        { 0b01001, 'h', "Low Hysteresis, Low Fan Heat" },
        { 0b00101, 'H', "Low Hysteresis, Hi Fan Heat" },

        { 0b10000, '-', "High Hysteresis, Nothing On" },
        { 0b11000, 'x', "High Hysteresis, Low Fan Only" },
        { 0b10100, 'X', "High Hysteresis, Hi Fan Only" },
        { 0b11010, 'y', "High Hysteresis, Low Fan Cool" },
        { 0b10110, 'Y', "High Hysteresis, Hi Fan Cool" },
        { 0b11001, 'z', "High Hysteresis, Low Fan Heat" },
        { 0b10101, 'Z', "High Hysteresis, Hi Fan Heat" }
    };
}

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

    // 3-bit high table
    char getHighBitCharacter() {
        return getHighBitInfo().character;
    }

    // 5-bit low table
    char getLowBitCharacter() {
        return getLowBitInfo().character;
    }

    String getHighBitString() {
        SignalStateEncoding::HighBitInfo info = getHighBitInfo();
        String result = "";
        result += info.character;
        result += " - ";
        result += info.description;
        return result;
    }

    String getLowBitString() {
        SignalStateEncoding::LowBitInfo info = getLowBitInfo();
        String result = "";
        result += info.character;
        result += " - ";
        result += info.description;
        return result;
    }

    SignalStateEncoding::HighBitInfo getHighBitInfo() const {
        uint8_t highBits =
            ((bits >> UNUSED)     & 1) << 2 |
            ((bits >> UBELLYCALL) & 1) << 1 |
            ((bits >> FURNACE)    & 1);

        for (uint8_t i = 0; i < sizeof(SignalStateEncoding::HIGH_BIT_TABLE) / sizeof(SignalStateEncoding::HighBitInfo); i++) {
            if (SignalStateEncoding::HIGH_BIT_TABLE[i].bits == highBits)
                return SignalStateEncoding::HIGH_BIT_TABLE[i];
        }

        return { highBits, '?', "Invalid Furnace/Underbelly State" };
    }

    SignalStateEncoding::LowBitInfo getLowBitInfo() const {
        uint8_t lowBits = bits & 0x1F;

        for (uint8_t i = 0; i < sizeof(SignalStateEncoding::LOW_BIT_TABLE) / sizeof(SignalStateEncoding::LowBitInfo); i++) {
            if (SignalStateEncoding::LOW_BIT_TABLE[i].bits == lowBits)
                return SignalStateEncoding::LOW_BIT_TABLE[i];
        }

        return { lowBits, '?', "Invalid Heat Pump Inputs" };
    }

    static const SignalStateEncoding::HighBitInfo* findHighBitInfo(char character) {
        uint8_t count = sizeof(SignalStateEncoding::HIGH_BIT_TABLE) / sizeof(SignalStateEncoding::HighBitInfo);
        for (uint8_t i = 0; i < count; i++) {
            if (SignalStateEncoding::HIGH_BIT_TABLE[i].character == character)
                return &SignalStateEncoding::HIGH_BIT_TABLE[i];
        }
        return nullptr;
    }

    static const SignalStateEncoding::LowBitInfo* findLowBitInfo(char character) {
        uint8_t count = sizeof(SignalStateEncoding::LOW_BIT_TABLE) / sizeof(SignalStateEncoding::LowBitInfo);
        for (uint8_t i = 0; i < count; i++) {
            if (SignalStateEncoding::LOW_BIT_TABLE[i].character == character)
                return &SignalStateEncoding::LOW_BIT_TABLE[i];
        }
        return nullptr;
    }
};