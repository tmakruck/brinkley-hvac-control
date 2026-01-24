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
        bits =
            (relay_fanLo.Value         << FAN_LO)       |
            (relay_fanHi.Value         << FAN_HI)       |
            (relay_ac.Value            << AC)           |
            (relay_heatPump.Value      << HEAT_PUMP)    |
            (relay_spaceHeater.Value   << SSR)          |
            (relay_furnace.Value       << FURNACE)      |
            (relay_furnacePower.Value  << FURNACE_POWER)|
            (relay_heatPumpPower.Value << HEAT_PUMP_POWER);
    }

    bool get(Bit b) const {
        return (bits >> b) & 1;
    }

    // Get human-readable description of all bits
    String describe() const {
        String result = "";
        // For most relays: HIGH = NC (Passthrough), LOW = NO (Active)
        result += get(FAN_LO)          ? "FanLo:P " : "FanLo:Alt ";
        result += get(FAN_HI)          ? "FanHi:P " : "FanHi:Alt/12V ";
        result += get(AC)              ? "AC:P " : "AC:Alt ";
        result += get(HEAT_PUMP)       ? "HP:P " : "HP:Alt/12V ";
        result += get(SSR)             ? "SSR:On " : "SSR:Off ";  // SSR is direct logic
        result += get(FURNACE)         ? "Furn:P " : "Furn:Alt ";
        result += get(FURNACE_POWER)   ? "FPwr:No12V" : "FPwr:12V ";
        result += get(HEAT_PUMP_POWER) ? "HPPwr:No12V " : "HPPwr:12V ";
        return result;
    }

    // Get binary string representation
    String toBinary() const {
        String result = "0b";
        for (int i = 7; i >= 0; i--) {
            result += (bits & (1 << i)) ? '1' : '0';
        }
        return result;
    }

    // Get formatted console output string
    const char* consoleData() const {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer), "0x%02X | %s | [%s]",
                 bits,
                 toBinary().c_str(),
                 describe().c_str());
        return buffer;
    }

    char encode(const OutputState& s) {
        return static_cast<char>(s.bits + 128);
    }



};
