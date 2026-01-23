// For Menu
#include <menu.h>
#include <menuIO/liquidCrystalOut.h>
#include <menuIO/serialIO.h>
#include <menuIO/stringIn.h>
// For temp Sensors
#include <OneWire.h>
#include <DallasTemperature.h>

using namespace Menu;
using namespace std;

bool debugState = false;
bool deepDebug = true;
bool isHysteresisLowMode = false;
int outdoorTemp = 100;
int underbellyTemp = 100;
int thirdTemp = 100;
unsigned long temperatureReadTimer = 0;
unsigned long lastDailyResetTime = 0;
unsigned long TWENTY_FOUR_HOURS_MS = 86400000UL;  // 24 hours in milliseconds

#pragma region Enumerations
enum class Thermometer {
  Outside,
  Underbelly, 
  Third
};

enum RoomName {
    BEDROOM,
    LIVING_ROOM,
    GARAGE
};

enum OutputState {
  Passthrough = HIGH,
  NoPassthrough = LOW
};

enum PowerState {
  Supply12V = LOW,
  NoSupply12V = HIGH
};

enum SSRState {
  SSROn = HIGH,
  SSROff = LOW
};

const char* getOutputStateName(OutputState t) {
  log_debug("Getting name for OutputState %d", (int)t);
  switch (t) {
    case OutputState::Passthrough:    return "Passthrough";
    case OutputState::NoPassthrough:  return "Use Custom Control";
    default:                          return "Unknown";
  }
}
const char* getOutputStateName(PowerState t) {
  log_debug("Getting name for PowerState %d", (int)t);
  switch (t) {
    case PowerState::Supply12V:      return "Supply 12V";
    case PowerState::NoSupply12V:    return "Don't Supply 12V";
    default:                         return "Unknown";
  }
}
const char* getOutputStateName(SSRState t) {
  log_debug("Getting name for SSRState %d", (int)t);
  switch (t) {
    case SSRState::SSROn:          return "SSR On";
    case SSRState::SSROff:         return "SSR Off";
  }
}

const int getOutputStateValue(OutputState t) {
  switch (t) {
    case OutputState::Passthrough:    return HIGH;
    case OutputState::NoPassthrough:  return LOW;
    default:                          return -1;
  }
}
const int getOutputStateValue(PowerState t) {
  switch (t) {
    case PowerState::Supply12V:      return LOW;
    case PowerState::NoSupply12V:    return HIGH;
    default:                         return -1;
  }
}
const int getOutputStateValue(SSRState t) {
  switch (t) {
    case SSRState::SSROn:          return HIGH;
    case SSRState::SSROff:         return LOW;
    default:                       return -1;
  }
}

#pragma endregion

#pragma region logging

void log_info(const char* fmt, ...){
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Serial.println(buffer);
}

void log_debug(const char* fmt, ...){
  if (!debugState) return;

  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Serial.println(buffer);
}

const char* statusString(bool isHigh) {
  return isHigh ? "High" : "Low";
}

#pragma endregion 

#ifndef CONSTANTS
  #define HEAT_PUMP_HYSTERESIS_LOWER_F 34  // Turn OFF heat pump below this
  #define HEAT_PUMP_HYSTERESIS_UPPER_F 36  // Turn ON heat pump above this
  #define UNDERBELLY_TEMP_THRESHOLD_F 40
  
  // Must be > 750 ms for Dallas Temp sensors to work properly
  const int LOOP_DELAY_MS = 800; // Dallas sensors: 750ms + 50ms buffer

  #pragma region LCD
    #define btnNONE 0
    #define btnRIGHT 1
    #define btnUP 2
    #define btnDOWN 3
    #define btnLEFT 4
    #define btnSELECT 5
    #define FIRST_LINE 0   //text position for first line
    #define SECOND_LINE 1  //text position for second line
    #define LCD_LINE_LENGTH 16
    #define SOFT_DEBOUNCE_MS 100
  #pragma endregion LCD

  #pragma region Menu
    #define MAX_DEPTH 3
  #pragma endregion Menu

  #pragma region Pin Definitions
    #pragma region LCD Pins
      #define D04_LCD_DataPin0 4
      #define D05_LCD_DataPin1 5
      #define D06_LCD_DataPin2 6
      #define D07_LCD_DataPin3 7
      #define D08_LCD_RS_PIN 8
      #define D09_LCD_ENABLE_PIN 9
      #define D10_LCD_BACKLIGHT_PIN 10
    #pragma endregion LCD

    #pragma region Custom Controls
      const int SSR1_HEAT   = 18;
      const int SSR2_HEAT   = 19;
      const int SSR3_HEAT   = 20;
      const int Temperature_Data = 21;
    #pragma endregion Custom Controls

    #pragma region Zone Pin Definitions
      const int ZONE1_START = 22;
      const int ZONE2_START = 38;
      const int ZONE3_START = 39;

      const int FURN_SENSE  = 23;
      const int FURN_OUT    = 31;
      const int FURN_12V    = 33;
      const int HP_12V      = 35;
    #pragma endregion Zone Pin Definitions
  #pragma endregion Pin Definitions

  #pragma region Other
    #define SpaceHeater HIGH
    #define DefaultBehavior LOW
    #define CONSTANTS // To consider it initialized
  #pragma endregion Other
#endif
#define LCD true

#pragma region Menu Setup
  #if LCD == true  //menu setup
    LiquidCrystal lcd(D08_LCD_RS_PIN, D09_LCD_ENABLE_PIN, D04_LCD_DataPin0, D05_LCD_DataPin1, D06_LCD_DataPin2, D07_LCD_DataPin3);
    
    // debounce a button
    int key_press_counter = 0;           // how many times we have seen new value
    long key_press_previous_time = 0;    // the last time the output pin was sampled
    byte key_press_debounce_count = 10;  // number of millis/samples to consider before declaring a debounced input
    byte current_state = 0;              // the debounced input value
    bool navigateMenu = false;
    
    
    TOGGLE(debugState, debugMenu, "Debug: ", doNothing, noEvent, wrapStyle,
          VALUE("No", true, doNothing, noEvent),
          VALUE("Yes", false, doNothing, noEvent));

    MENU(mainMenu, "Main menu", doNothing, noEvent, wrapStyle,
        SUBMENU(debugMenu),
        EXIT("< Back"));

    MENU_OUTPUTS(out, MAX_DEPTH, LIQUIDCRYSTAL_OUT(lcd, { 0, 0, 16, 2 }), NONE);

    stringIn<0> strIn;  //buffer size: 2^5 = 32 bytes, eventually use 0 for a single byte
    serialIn serial(Serial);

    NAVROOT(nav, mainMenu, MAX_DEPTH, serial, out);

    byte read_LCD_buttons() {          // read the buttons
      int adc_key_in = analogRead(0);  // read the value from the sensor
      //value read: 0(0V), 130(0.64V), 306(1.49V), 479(2.33V), 722(3.5V), 1023(4.97V)
      int rightMax = 64;
      int upMax = 128;
      int downMax = 256;
      int leftMax = 512;
      int selectMax = 768;
      if (adc_key_in<1020) log_debug("ADC Value = %d", adc_key_in);
      if (adc_key_in <= rightMax) return btnRIGHT; // 0
      if (adc_key_in <= upMax) return btnUP; // 99
      if (adc_key_in <= downMax) return btnDOWN; // 256
      if (adc_key_in <= leftMax) return btnLEFT; // 410
      if (adc_key_in <= selectMax) return btnSELECT; //641
      if (adc_key_in > selectMax) return btnNONE;
    }

    byte key_press() {
      // If we have gone on to the next millisecond
      if (millis() != key_press_previous_time) {
        byte this_button = read_LCD_buttons();

        if (this_button == current_state && key_press_counter > 0) key_press_counter--;

        if (this_button != current_state) key_press_counter++;

        // If the Input has shown the same value for long enough let's switch it
        if (key_press_counter >= key_press_debounce_count) {
          key_press_counter = 0;
          current_state = this_button;
          return this_button;
        }
        key_press_previous_time = millis();
      }
      return 0;
    }

    void handleButtonPress() {
      //log_debug("Handling Button Presses");
      byte lcd_key = key_press();  // read the buttons

      switch (lcd_key) {
        case btnRIGHT:
          log_debug("Pressed Right Button");
          nav.doNav(enterCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          break;
        case btnLEFT:
          log_debug("Pressed Left Button");
          nav.doNav(escCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          // Do something like getting back to normal run mode
          break;
        case btnUP:
          log_debug("Pressed Up Button");
          nav.doNav(upCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          break;
        case btnDOWN:
          log_debug("Pressed Down Button");
          nav.doNav(downCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          break;
        case btnSELECT:
          log_debug("Pressed Select Button");
          nav.doNav(enterCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          break;
        case btnNONE:
          //log_debug("no button press found");
          if (!navigateMenu) {
          }
          break;
      }
    }

    void writeError(int line, uint8_t value) {
      char stringBuffer[LCD_LINE_LENGTH] = {};
      snprintf(stringBuffer, LCD_LINE_LENGTH, "Error: 0x%02X", value);
      writeLCD(line, stringBuffer);
    }


    void writeLCD(int line, char* fmt, ...){
      char buffer[LCD_LINE_LENGTH] = {};
      va_list args;
      va_start(args, fmt);
      vsnprintf(buffer, sizeof(buffer), fmt, args);
      va_end(args);

      snprintf(buffer, sizeof(buffer), "%-16s", buffer);
      
      #if LCD == true
        lcd.setCursor(0, line);
        lcd.print(buffer);
      #endif
    }

    void writeLCD(int line, int position, const char *input) {

      #if LCD == true
        lcd.setCursor(position, line);
        lcd.print(input);
      #endif
    }
  #endif  //menu setup
#pragma endregion

#pragma region Thermometer Setup
  OneWire oneWire(Temperature_Data);
  DallasTemperature sensors(&oneWire);

  // Store each sensor's unique 64-bit address
  DeviceAddress outsideAddr    = { 0x28, 0xEC, 0x81, 0x87, 0x00, 0xE0, 0x49, 0xBC };
  DeviceAddress underbellyAddr = { 0x28, 0xA6, 0x0B, 0x87, 0x00, 0xA6, 0x0C, 0x38 };
  DeviceAddress thirdAddr      = { 0x28, 0x44, 0x71, 0x87, 0x00, 0x6B, 0x20, 0x12 };

  // Function to get address by enum
  DeviceAddress* getAddress(Thermometer t) {
    switch (t) {
      case Thermometer::Outside:    return &outsideAddr;
      case Thermometer::Underbelly: return &underbellyAddr;
      case Thermometer::Third:      return &thirdAddr;
      default: return nullptr;
    }
  }

  const char* getName(Thermometer t) {
    switch (t) {
      case Thermometer::Outside:    return "Outside";
      case Thermometer::Underbelly: return "Underbelly";
      case Thermometer::Third:      return "Third";
      default:                      return "Unknown";
    }
  }

  int retrieveTemperature(Thermometer t) {
    // call sensors.requestTemperatures() to issue a global temperature
    // request to all devices on the bus
    // After we got the temperatures, we can print them here.
    // We use the function ByIndex, and as an example get the temperature from the first sensor only.
    DeviceAddress* address = getAddress(t);
    if (!address) {
      log_debug("Error: Invalid thermometer %d", (int)t);
      return -999;
    }
    float temperature = sensors.getTempF(*address);
    int tempInt = static_cast<int>(round(temperature));
    // Check if reading was successful
    if (temperature != DEVICE_DISCONNECTED_F)
    {
      log_debug("Temperature for %s is %d", getName(t), tempInt);
    }
    else
    {
      log_debug("Error: Could not read temperature data");
    }

    return tempInt;
  }

  void printAddress(Thermometer t) {
    DeviceAddress* deviceAddress = getAddress(t);
    for (uint8_t i = 0; i < 8; i++)
    {
      if ((*deviceAddress)[i] < 16) Serial.print("0");
      Serial.print((*deviceAddress)[i], HEX);
    }
  }

  void startSensors(){
    sensors.begin();
    sensors.requestTemperatures();
    delay(LOOP_DELAY_MS);
  }

#pragma endregion

#pragma region HVAC setup
class HVAC {
public:    
    // Room identifier
    RoomName room;

    // Sense input pins
    int pinFanLoSense;
    int pinFanHiSense;
    int pinACSense;
    int pinHPSense;

    // Relay output pins (control coils)
    int pinFanLoOut;
    int pinFanHiOut;
    int pinACOut;
    int pinHPOut;

    // SSR output pin
    int pinSSR;
    
    // track current call status
    bool isFanLoCall    = false;
    bool isFanHiCall    = false;
    bool isACCall       = false;
    bool isHeatPumpCall = false;
    bool isFurnaceCall  = false;

    // track current control status
    bool is_passthough_fanLo = true;
    bool is_passthrough_fanHi = true;
    bool is_passthrough_AC = true;
    bool is_passthrough_HP   = true;
    bool is_passthrough_Furnace = true;
    bool is_SSROn = false;
    bool is_furnace_12V_Bypassed = false;
    bool is_heatPump_12V_Bypassed = true;

    // Whether this HVAC instance uses a furnace
    bool hasFurnace;


    int zoneID=0;

    HVAC(int startPin, bool furnaceEnabled, int ssrPin, RoomName roomName, int zoneID)
        : room(roomName)
    {
        this->zoneID = zoneID;
        hasFurnace = furnaceEnabled;
        pinSSR = ssrPin;

        // Inputs (sense pins)
        pinFanLoSense = startPin;          // +0
        pinFanHiSense = startPin + 2;      // +2
        pinACSense    = startPin + 4;      // +4
        pinHPSense    = startPin + 6;      // +6

        // Outputs (relay coil pins)
        pinFanLoOut   = startPin + 8;      // +8
        pinFanHiOut   = startPin + 10;     // +10
        pinACOut      = startPin + 12;     // +12
        pinHPOut      = startPin + 14;     // +14
    }

    void begin() {
        // Sense inputs
        pinMode(pinFanLoSense, INPUT_PULLUP);
        pinMode(pinFanHiSense, INPUT_PULLUP);
        pinMode(pinACSense,    INPUT_PULLUP);
        pinMode(pinHPSense,    INPUT_PULLUP);

        // Relay outputs
        pinMode(pinFanLoOut, OUTPUT);
        pinMode(pinFanHiOut, OUTPUT);
        pinMode(pinACOut,    OUTPUT);
        pinMode(pinHPOut,    OUTPUT);

        // SSR output
        pinMode(pinSSR, OUTPUT);

        // Initialize all Heat Pump outputs HIGH (relays de‑energized → NC pass‑through)
        setFanLoRelayState(OutputState::Passthrough);
        setFanHiRelayState(OutputState::Passthrough);
        setACRelayState(OutputState::Passthrough);
        setHeatPumpRelayState(OutputState::Passthrough);
        // And turn off the SSR
        digitalWrite(pinSSR,getOutputStateValue(SSRState::SSROff));
    }

    bool readPin(char* pinName, int pinID) {
      bool isHigh = digitalRead(pinID) == HIGH;
      log_debug("%s reads %s", pinName, statusString(isHigh));
      return isHigh;
    }

    // --- Sense helpers ---
    bool fanLoCall()    { return readPin("Call: Fan Lo", pinFanLoSense); }
    bool fanHiCall()    { return readPin("Call: Fan Hi", pinFanHiSense); }
    bool acCall()       { return readPin("Call: Air Conditioner", pinACSense); }
    bool heatPumpCall() { return readPin("Call: Heat Pump", pinHPSense); }
    bool furnaceCall()  { return readPin("Call: Furnace", FURN_SENSE); }

    template<typename EnumType>
    bool readPin(char* pinName, int pinID, EnumType ofEnumeration) {
      int numericValue = digitalRead(pinID);
      log_debug("%s raw value: %d", pinName, numericValue);
      
      bool isHigh = numericValue == HIGH;

      // convert the numeric value to the string representation
      EnumType enumValue = static_cast<EnumType>(numericValue);
      log_debug("%s enum cast to int: %d", pinName, (int)enumValue);
      
      const char* resultingText = getOutputStateName(enumValue);
      log_debug("%s reads %s (numeric: %d, enum: %d)", pinName, resultingText, numericValue, (int)enumValue);
      return isHigh;
    }

    // --- Output readers ---
    bool read_fanLoOut()      { return readPin("Output: Fan Lo", pinFanLoOut, OutputState{}); }
    bool read_fanHiOut()      { return readPin("Output: Fan Hi", pinFanHiOut, OutputState{}); }
    bool read_acOut()         { return readPin("Output: Air Conditioner", pinACOut, OutputState{}); }
    bool read_heatPumpOut()   { return readPin("Output: Heat Pump", pinHPOut, OutputState{}); }
    bool read_furnaceOut()    { return readPin("Output: Furnace", FURN_OUT, OutputState{}); }
    bool read_furnacePower()  { return readPin("Output Power for: Furnace", FURN_12V, PowerState{}); }
    bool read_HeatPumpPower() { return readPin("Output Power for: Heat Pump", HP_12V, PowerState{}); }
    bool read_ssrOut()        { return readPin("Output Activate: Space Heater SSR", pinSSR, SSRState{}); }

    // --- Relay control helpers ---
    void setPin(char* pinName, int pinID, OutputState outputState){
      char* stateName = getOutputStateName(outputState);
      int stateValue =  getOutputStateValue(outputState);
      log_debug("Setting %s (%d) to %s (%d)", pinName, pinID, stateName, stateValue);
      digitalWrite(pinID, stateValue);
    }

    void setPin(char* pinName, int pinID, PowerState outputState){
      char* stateName = getOutputStateName(outputState);
      int stateValue =  getOutputStateValue(outputState);
      log_debug("Setting %s (%d) to %s (%d)", pinName, pinID, stateName, stateValue);
      digitalWrite(pinID, stateValue);
    }

    void setPin(char* pinName, int pinID, SSRState outputState){
      char* stateName = getOutputStateName(outputState);
      int stateValue =  getOutputStateValue(outputState);
      log_debug("Setting %s (%d) to %s (%d)", pinName, pinID, stateName, stateValue);
      digitalWrite(pinID, stateValue);
    }

    // --- Output Controls ---
    void setFanLoRelayState(OutputState outputState)    { setPin("Fan Lo", pinFanLoOut, outputState);  }
    void setFanHiRelayState(OutputState outputState)    { setPin("Fan Hi", pinFanHiOut, outputState); }
    void setACRelayState(OutputState outputState)       { setPin("Air Conditioner", pinACOut, outputState); }
    void setHeatPumpRelayState(OutputState outputState) { setPin("Heat Pump", pinHPOut, outputState); }
    void setFurnaceRelayState(OutputState outputState)  { setPin("Furnace", FURN_OUT, outputState); }
    
    // --- SSR control ---
    void setSSR(SSRState ssrState)                      { setPin("Space Heater", pinSSR, ssrState); }

    // --- Power Controls ---
    void setFurnacePowerRelayState(PowerState powerState)   { setPin("Manual Furnace Power", FURN_12V, powerState); } 
    void setHeatPumpPowerRelayState(PowerState powerState)  { setPin("Manual Heat Pump Power", HP_12V, powerState); }


    // convert enum → readable string
    const char* roomNameString() const {
        switch (room) {
            case BEDROOM:     return "Bedroom";
            case LIVING_ROOM: return "Living Room";
            case GARAGE:      return "Garage";
            default:          return "Unknown";
        }
    }

    void VerifyCalls(){
        isFanLoCall = fanLoCall();
        isFanHiCall = fanHiCall();
        isACCall = acCall();
        isHeatPumpCall = heatPumpCall();
        if (hasFurnace){
          isFurnaceCall = furnaceCall();
        }

    }

    void VerifyOutputs(){
        is_passthough_fanLo = read_fanLoOut();
        is_passthrough_fanHi = read_fanHiOut();
        is_passthrough_AC = read_acOut();
        is_passthrough_HP = read_heatPumpOut();
        if (hasFurnace) { 
          is_heatPump_12V_Bypassed = read_HeatPumpPower();
          is_passthrough_Furnace = read_furnaceOut(); 
          is_furnace_12V_Bypassed = read_furnacePower();
        }
        is_SSROn = read_ssrOut();


    }
    
    void printPinStates() {
      VerifyOutputs();
      
      
      char zoneHeatPumpStatus[LCD_LINE_LENGTH] = {};
      for (int i = 0; i < LCD_LINE_LENGTH; i++) {
        zoneHeatPumpStatus[i] = ' ';
      }

      zoneHeatPumpStatus[0] = ('0' + zoneID); // Zone ID as char
      if (hasFurnace && isFurnaceCall) {
        zoneHeatPumpStatus[1] = 'F';
      }
      else if (isHeatPumpCall) {
        zoneHeatPumpStatus[1] = 'H';
      }
      else if (!isHeatPumpCall) {
        zoneHeatPumpStatus[1] = '-';
      }
      else {
        zoneHeatPumpStatus[1] = 'U';
      }

      if(is_SSROn) {
        zoneHeatPumpStatus[2] = 'S'; // Space Heater is ON
      } else if(is_passthrough_HP) {
        // Heat Pump is PassThrough
        if (is_heatPump_12V_Bypassed){
          zoneHeatPumpStatus[2] = 'P'; 
        }
        else
        {
          zoneHeatPumpStatus[2] = 'C'; // But still using custom power
        }
      } else if(!is_passthrough_HP) {
        if (is_heatPump_12V_Bypassed) {
          
          zoneHeatPumpStatus[2] = 'X'; // Heat Pump is not powered
        } else {
          zoneHeatPumpStatus[2] = 'H'; // power passed to heat pump
        }
      } else if (hasFurnace && underbellyTemp < UNDERBELLY_TEMP_THRESHOLD_F && !isFurnaceCall) {
        zoneHeatPumpStatus[2] = 'U'; // Underbelly too cold
      } 
      else {
        zoneHeatPumpStatus[2] = '-'; // assume its low, nothing is on
      }

      zoneHeatPumpStatus[3] = '\0'; // Null-terminate the string

      log_debug("Zone status: %s", zoneHeatPumpStatus);
      writeLCD(SECOND_LINE, 4*zoneID-4, zoneHeatPumpStatus);
      
    }

    void fallbackToDefaultBehavior(){
      log_debug("Falling back to default behavior for %s", roomNameString());
      setFanLoRelayState(OutputState::Passthrough);
      setFanHiRelayState(OutputState::Passthrough);
      setACRelayState(OutputState::Passthrough);
      setHeatPumpRelayState(OutputState::Passthrough);
      setSSR(SSRState::SSROff);
      if (hasFurnace){
        setFurnaceRelayState(OutputState::Passthrough);
        setFurnacePowerRelayState(PowerState::NoSupply12V);
        setHeatPumpPowerRelayState(PowerState::NoSupply12V);
      }
    }

    void ApplyCallForHeat(){
      bool furnaceCallActive = hasFurnace && isFurnaceCall;
      bool heatPumpCallActive = isHeatPumpCall;
      bool underbellyTooCold = hasFurnace && underbellyTemp < UNDERBELLY_TEMP_THRESHOLD_F;
      bool hasCallForHeat = (furnaceCallActive || heatPumpCallActive);
      bool isFrigid = outdoorTemp < 15;
  
      if (isHysteresisLowMode){
        // the temps are cold enough to use the space heaters
        // control the space heater.
        // don't pass the heat pump through

        setFanLoRelayState(OutputState::NoPassthrough);
        setFanHiRelayState(OutputState::NoPassthrough);
        setACRelayState(OutputState::NoPassthrough);
        setHeatPumpRelayState(OutputState::NoPassthrough);

        if (hasCallForHeat){
          // turn on the ssr for the zone
          setSSR(SSRState::SSROn);
          // 
          if(hasFurnace) {
            setFurnaceRelayState(OutputState::Passthrough);
            setFurnacePowerRelayState(PowerState::NoSupply12V);
          }
        }
        else {
          
          if (hasFurnace){
            if (underbellyTooCold){
              setFurnaceRelayState(OutputState::NoPassthrough);
              setFurnacePowerRelayState(PowerState::Supply12V);
            }
            else {
              setFurnaceRelayState(OutputState::Passthrough);
              setFurnacePowerRelayState(PowerState::NoSupply12V);
            }
            if (isFrigid){
              setSSR(SSRState::SSROn);
            }
            else{
              setSSR(SSRState::SSROff);
            }
          }
          else
          {
            setSSR(SSRState::SSROff);
          }
        }
      }
      else { // NOT HysteresisLowMode
        // never run space heater above threshold
        // never run furnace above threshold
        // always pass heat pump through
        setSSR(SSRState::SSROff);
        
        if (furnaceCallActive) {
          log_debug("Redirecting furnace call to heat pump");
          setHeatPumpPowerRelayState(PowerState::Supply12V);
          setFanHiRelayState(OutputState::NoPassthrough);
          setHeatPumpRelayState(OutputState::NoPassthrough);
          //setFurnaceRelayState(OutputState::NoPassthrough);
          setFurnaceRelayState(OutputState::NoPassthrough);

          if (underbellyTooCold) {
            setFurnacePowerRelayState(PowerState::Supply12V);
          } else {
            setFurnacePowerRelayState(PowerState::NoSupply12V);
          }
          
        }
        else{
          setFanLoRelayState(OutputState::Passthrough);
          setFanHiRelayState(OutputState::Passthrough);
          setACRelayState(OutputState::Passthrough);
          setHeatPumpRelayState(OutputState::Passthrough);

          if (hasFurnace){
            setHeatPumpPowerRelayState(PowerState::NoSupply12V);
            if (underbellyTooCold && !isFurnaceCall){
              setFurnaceRelayState(OutputState::NoPassthrough);
              setFurnacePowerRelayState(PowerState::Supply12V);
            } else {
              setFurnaceRelayState(OutputState::Passthrough);
              setFurnacePowerRelayState(PowerState::NoSupply12V);
            }
            
          }

        }     
      }
    }
    // --- Make appropriate adjustments ---
    void Adjust(){
      log_debug("------------------");
      log_debug("Adjusting %s", roomNameString());
      VerifyCalls();

      if (outdoorTemp < -100 or underbellyTemp < -100) {
      }
      else{
        ApplyCallForHeat();
      }
      
      printPinStates();
    }
};

  bool controlsFurnace = true;
  HVAC Zone1(ZONE1_START, controlsFurnace, SSR1_HEAT, BEDROOM, 1);  
  HVAC Zone2(ZONE2_START, !controlsFurnace, SSR2_HEAT, LIVING_ROOM, 2);  
  HVAC Zone3(ZONE3_START, !controlsFurnace, SSR3_HEAT, GARAGE, 3);
#pragma endregion

void loop() {
  unsigned long currentTime = millis();
  // Reset timer every 24 hours
  if (currentTime - lastDailyResetTime >= TWENTY_FOUR_HOURS_MS) {
    log_debug("Resetting daily timer");
    temperatureReadTimer = 0;
    lastDailyResetTime = currentTime;
  }

  unsigned long timeDiff = currentTime - temperatureReadTimer;
  // Check if 10 seconds (10000 ms) have passed since last read
  if (timeDiff >= 5000) {
    log_debug("current Time: %lu minus %lu = %lu", currentTime, temperatureReadTimer, timeDiff);
    log_info("Requesting temperatures...");
    outdoorTemp     = retrieveTemperature(Thermometer::Outside);
    underbellyTemp  = retrieveTemperature(Thermometer::Underbelly);
    thirdTemp       = retrieveTemperature(Thermometer::Third);

    // Hysteresis logic: use different thresholds for on/off
    if (isHysteresisLowMode) {
      // Currently in heat mode - need temp to rise above UPPER threshold to switch
      if (outdoorTemp >= HEAT_PUMP_HYSTERESIS_UPPER_F) {
        log_debug("Temperature rising above %dF, switching to heat pump mode", HEAT_PUMP_HYSTERESIS_UPPER_F);
        isHysteresisLowMode = false;
      }
    } else {
      // Currently in heat pump mode - need temp to drop below LOWER threshold to switch
      if (outdoorTemp < HEAT_PUMP_HYSTERESIS_LOWER_F) {
        log_debug("Temperature dropped below %dF, switching to furnace + space heater mode", HEAT_PUMP_HYSTERESIS_LOWER_F);
        isHysteresisLowMode = true;
      }
    }

    if (outdoorTemp < -100 or underbellyTemp < -100) {
      log_info("Invalid temperature readings detected, skipping adjustments");
      Zone1.fallbackToDefaultBehavior();
      Zone2.fallbackToDefaultBehavior();
      Zone3.fallbackToDefaultBehavior();
    }
            

    
    writeLCD(FIRST_LINE, "O:%d%s U:%d", outdoorTemp, isHysteresisLowMode ? "L" : "H", underbellyTemp);
    sensors.requestTemperatures();
    temperatureReadTimer = currentTime;  // Reset the timer
  }

  #if LCD == true
    handleButtonPress();
  #endif
  
  Zone1.Adjust();
  Zone2.Adjust();
  Zone3.Adjust();

}



void setup() {
  Serial.begin(9600);
  log_debug("------------------------------------------------------------------------------------------");
  lcd.begin(16, 2);
  writeLCD(FIRST_LINE, "Initializing");

  Zone1.begin();
  Zone2.begin();
  Zone3.begin();


  pinMode(FURN_SENSE,     INPUT_PULLUP);
  pinMode(FURN_OUT,       OUTPUT);
  digitalWrite(FURN_OUT,  HIGH);
  pinMode(FURN_12V,       OUTPUT);
  digitalWrite(FURN_12V,  HIGH);
  pinMode(HP_12V,         OUTPUT);
  digitalWrite(HP_12V,    HIGH);
  
  startSensors();
  temperatureReadTimer = millis()-20000;  // Initialize timer
  lastDailyResetTime = millis()-20000;    // Initialize daily reset timer

  writeLCD(SECOND_LINE, " ");

}