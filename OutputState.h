#pragma once
#include "StateType.h"

namespace OutputStateEncoding {
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
        { 0b000, 'P', "Furnace Passthrough" },
        { 0b001, 'F', "Furnace Bypass" },
        { 0b011, 'M', "Force Furnace" },
        { 0b100, 'H', "Force Heat Pump" },
        { 0b101, 'O', "Force Heat Pump, Furnace Bypass" },
        { 0b111, 'B', "Force Both" }
    };

    static const LowBitInfo LOW_BIT_TABLE[] = {
        { 0b00000, '_', "SSR Off; All Passthrough" },
        { 0b00001, 'a', "SSR Off; Low Fan Passthrough; Hi Fan Passthrough; AC Passthrough; Heat Pump Alternate" },
        { 0b00010, 'b', "SSR Off; Low Fan Passthrough; Hi Fan Passthrough; AC Alternate; Heat Pump Passthrough" },
        { 0b00011, 'c', "SSR Off; Low Fan Passthrough; Hi Fan Passthrough; AC Alternate; Heat Pump Alternate" },
        { 0b00100, 'd', "SSR Off; Low Fan Passthrough; Hi Fan Alternate; AC Passthrough; Heat Pump Passthrough" },
        { 0b00101, 'e', "SSR Off; Override Hi Heat Pump" },
        { 0b00110, 'f', "SSR Off; Low Fan Passthrough; Hi Fan Alternate; AC Alternate; Heat Pump Passthrough" },
        { 0b00111, 'g', "SSR Off; Low Fan Passthrough; Hi Fan Alternate; AC Alternate; Heat Pump Alternate" },
        { 0b01000, 'h', "SSR Off; Low Fan Alternate; Hi Fan Passthrough; AC Passthrough; Heat Pump Passthrough" },
        { 0b01001, 'i', "SSR Off; Low Fan Alternate; Hi Fan Passthrough; AC Passthrough; Heat Pump Alternate" },
        { 0b01010, 'j', "SSR Off; Low Fan Alternate; Hi Fan Passthrough; AC Alternate; Heat Pump Passthrough" },
        { 0b01011, 'k', "SSR Off; Low Fan Alternate; Hi Fan Passthrough; AC Alternate; Heat Pump Alternate" },
        { 0b01100, 'l', "SSR Off; Low Fan Alternate; Hi Fan Alternate; AC Passthrough; Heat Pump Passthrough" },
        { 0b01101, 'm', "SSR Off; Low Fan Alternate; Hi Fan Alternate; AC Passthrough; Heat Pump Alternate" },
        { 0b01110, 'n', "SSR Off; Low Fan Alternate; Hi Fan Alternate; AC Alternate; Heat Pump Passthrough" },
        { 0b01111, 'o', "SSR Off; All Alternate" },

        { 0b10000, 'S', "SSR On; All Passthrough" },
        { 0b10001, 'A', "SSR On; Low Fan Passthrough; Hi Fan Passthrough; AC Passthrough; Heat Pump Alternate" },
        { 0b10010, 'B', "SSR On; Low Fan Passthrough; Hi Fan Passthrough; AC Alternate; Heat Pump Passthrough" },
        { 0b10011, 'C', "SSR On; Low Fan Passthrough; Hi Fan Passthrough; AC Alternate; Heat Pump Alternate" },
        { 0b10100, 'D', "SSR On; Low Fan Passthrough; Hi Fan Alternate; AC Passthrough; Heat Pump Passthrough" },
        { 0b10101, 'E', "SSR On; Low Fan Passthrough; Hi Fan Alternate; AC Passthrough; Heat Pump Alternate" },
        { 0b10110, 'F', "SSR On; Low Fan Passthrough; Hi Fan Alternate; AC Alternate; Heat Pump Passthrough" },
        { 0b10111, 'G', "SSR On; Low Fan Passthrough; Hi Fan Alternate; AC Alternate; Heat Pump Alternate" },
        { 0b11000, 'H', "SSR On; Low Fan Alternate; Hi Fan Passthrough; AC Passthrough; Heat Pump Passthrough" },
        { 0b11001, 'I', "SSR On; Low Fan Alternate; Hi Fan Passthrough; AC Passthrough; Heat Pump Alternate" },
        { 0b11010, 'J', "SSR On; Low Fan Alternate; Hi Fan Passthrough; AC Alternate; Heat Pump Passthrough" },
        { 0b11011, 'K', "SSR On; Low Fan Alternate; Hi Fan Passthrough; AC Alternate; Heat Pump Alternate" },
        { 0b11100, 'L', "SSR On; Low Fan Alternate; Hi Fan Alternate; AC Passthrough; Heat Pump Passthrough" },
        { 0b11101, 'M', "SSR On; Low Fan Alternate; Hi Fan Alternate; AC Passthrough; Heat Pump Alternate" },
        { 0b11110, 'N', "SSR On; Low Fan Alternate; Hi Fan Alternate; AC Alternate; Heat Pump Passthrough" },
        { 0b11111, 'O', "SSR On; All Alternate" }
    };
}

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

    OutputState(bool fanLo=Passthrough.Value, bool fanHi=Passthrough.Value, bool ac=Passthrough.Value, bool heatPump=Passthrough.Value, bool spaceHeater=SpaceHeaterOff.Value, 
        bool furnace=0, bool furnacePower=NoSupply12V.Value, bool heatPumpPower=NoSupply12V.Value) {
        bits = 0;

        bitWrite(bits, HEAT_PUMP_POWER, heatPumpPower);
        bitWrite(bits, FURNACE_POWER,   furnacePower);
        bitWrite(bits, FURNACE,         furnace);
        bitWrite(bits, SSR,             spaceHeater);
        bitWrite(bits, FAN_LO,          fanLo);
        bitWrite(bits, FAN_HI,          fanHi);
        bitWrite(bits, AC,              ac);
        bitWrite(bits, HEAT_PUMP,       heatPump);
    }

    OutputState(StateType fanLo, StateType fanHi, StateType ac, StateType heatPump, StateType spaceHeater = SpaceHeaterOff,
                StateType furnace = Passthrough, StateType furnacePower = NoSupply12V, StateType heatPumpPower = NoSupply12V) {
        bits = 0;

        bitWrite(bits, HEAT_PUMP_POWER, heatPumpPower.Value);
        bitWrite(bits, FURNACE_POWER,   furnacePower.Value);
        bitWrite(bits, FURNACE,         furnace.Value);
        bitWrite(bits, SSR,             spaceHeater.Value);
        bitWrite(bits, FAN_LO,          fanLo.Value);
        bitWrite(bits, FAN_HI,          fanHi.Value);
        bitWrite(bits, AC,              ac.Value);
        bitWrite(bits, HEAT_PUMP,       heatPump.Value);
    }

    int get(Bit b) const {
        return (bits >> b) & 1;
    }

    // Human-readable description
    String BitByBit(bool hasFurnace) {
        String result = "[";

        if (hasFurnace) {
        // Group A
            result += get(HEAT_PUMP_POWER) ? "HPPwr+ " : "HPPwr- ";
            result += get(FURNACE_POWER)   ? "FPwr+ "  : "FPwr- ";
            result += get(FURNACE)         ? "Furn+ "  : "Furn- ";
            result += "][";
        }
        // Group B
        result += get(SSR)             ? "SSR+ "  : "SSR- ";  // SSR is direct logic
        result += get(FAN_LO)          ? "Lo+ "   : "Lo- ";
        result += get(FAN_HI)          ? "Hi+ "   : "Hi- ";
        result += get(AC)              ? "AC+ "   : "AC- ";
        result += get(HEAT_PUMP)       ? "HP+ "   : "HP- ";
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

    String getHighBitString(){        
        OutputStateEncoding::HighBitInfo info = getHighBitInfo();
        String result = "";
        result += info.character;
        result += " - ";
        result += info.description;
        return result;
    }

    String getLowBitString(){       
        OutputStateEncoding::LowBitInfo info = getLowBitInfo();
        String result = "";
        result += info.character;
        result += " - ";
        result += info.description;
        return result;
    }

    OutputStateEncoding::HighBitInfo getHighBitInfo() const {
        uint8_t highBits =
            ((bits >> HEAT_PUMP_POWER) & 1) << 2 |
            ((bits >> FURNACE_POWER)   & 1) << 1 |
            ((bits >> FURNACE)         & 1);

        uint8_t count = sizeof(OutputStateEncoding::HIGH_BIT_TABLE) / sizeof(OutputStateEncoding::HighBitInfo);
        for (uint8_t i = 0; i < count; i++) {
            if (OutputStateEncoding::HIGH_BIT_TABLE[i].bits == highBits)
                return OutputStateEncoding::HIGH_BIT_TABLE[i];
        }

        return { highBits, '?', "Invalid Furnace/Heat Pump Power State" };
    }

    OutputStateEncoding::LowBitInfo getLowBitInfo() const {
        uint8_t lowBits = bits & 0x1F;

        uint8_t count = sizeof(OutputStateEncoding::LOW_BIT_TABLE) / sizeof(OutputStateEncoding::LowBitInfo);
        for (uint8_t i = 0; i < count; i++) {
            if (OutputStateEncoding::LOW_BIT_TABLE[i].bits == lowBits)
                return OutputStateEncoding::LOW_BIT_TABLE[i];
        }

        return { lowBits, '?', "Unknown Output State" };
    }

    static const OutputStateEncoding::HighBitInfo* findHighBitInfo(char character) {
        uint8_t count = sizeof(OutputStateEncoding::HIGH_BIT_TABLE) / sizeof(OutputStateEncoding::HighBitInfo);
        for (uint8_t i = 0; i < count; i++) {
            if (OutputStateEncoding::HIGH_BIT_TABLE[i].character == character)
                return &OutputStateEncoding::HIGH_BIT_TABLE[i];
        }
        return nullptr;
    }

    static const OutputStateEncoding::LowBitInfo* findLowBitInfo(char character) {
        uint8_t count = sizeof(OutputStateEncoding::LOW_BIT_TABLE) / sizeof(OutputStateEncoding::LowBitInfo);
        for (uint8_t i = 0; i < count; i++) {
            if (OutputStateEncoding::LOW_BIT_TABLE[i].character == character)
                return &OutputStateEncoding::LOW_BIT_TABLE[i];
        }
        return nullptr;
    }
};