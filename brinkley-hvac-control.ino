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

#ifndef CONSTANTS
  #define HEAT_PUMP_THRESHOLD_F 34
  #define HEAT_PUMP_HYSTERESIS_LOWER_F 33  // Turn OFF heat pump below this
  #define HEAT_PUMP_HYSTERESIS_UPPER_F 35  // Turn ON heat pump above this
  #define UNDERBELLY_TEMP_THRESHOLD_F 50
  
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
    #pragma region LCD
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
    #pragma endregion

    #pragma Zone Starts
      const int ZONE1_START = 22;
      const int ZONE2_START = 38;
      const int ZONE3_START = 39;

      const int FURN_SENSE  = 23;
      const int FURN_OUT    = 31;
    #pragma endregion

    // Temperature Probes
  #pragma endregion Pin Definitions

  #define SpaceHeater HIGH
  #define DefaultBehavior LOW
  #define CONSTANTS // To consider it initialized
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
      int rightMax = 75;
      int downMax = 218;
      int upMax = 392;
      int leftMax = 600;
      int selectMax = 800;
      int noneMin = 1000;
      if (adc_key_in > noneMin) return btnNONE;
      if (adc_key_in < rightMax) return btnRIGHT;
      if (adc_key_in < downMax) return btnDOWN;
      if (adc_key_in < upMax) return btnUP;
      if (adc_key_in < leftMax) return btnLEFT;
      if (adc_key_in < selectMax) return btnSELECT;
      return btnNONE;
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
      byte lcd_key = key_press();  // read the buttons

      switch (lcd_key) {
        case btnRIGHT:
          nav.doNav(enterCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          break;
        case btnLEFT:
          nav.doNav(escCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          // Do something like getting back to normal run mode
          break;
        case btnUP:
          nav.doNav(upCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          break;
        case btnDOWN:
          nav.doNav(downCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          break;
        case btnSELECT:
          nav.doNav(enterCmd);
          delay(SOFT_DEBOUNCE_MS);
          nav.doOutput();
          break;
        case btnNONE:
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

    void writeLCD(int line, char *format, int value1, int value2 = -1) {
      char stringBuffer[LCD_LINE_LENGTH] = {};
      snprintf(stringBuffer, LCD_LINE_LENGTH, format, value1, value2);
      writeLCD(line, stringBuffer);
    }

    void writeLCD(int line, char *input) {
      char stringBuffer[LCD_LINE_LENGTH] = {};
      snprintf(stringBuffer, sizeof(stringBuffer), "%-16s", input);
      #if LCD == true
        lcd.setCursor(0, line);
        lcd.print(stringBuffer);
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
    bool isSSROn = false;

    // Whether this HVAC instance uses a furnace
    bool hasFurnace;

    // Track the current heating mode state for hysteresis
    bool isInHeatMode = false;  // Add this member variable

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
        digitalWrite(pinFanLoOut, HIGH);
        digitalWrite(pinFanHiOut, HIGH);
        digitalWrite(pinACOut,    HIGH);
        digitalWrite(pinHPOut,    HIGH);
        // And turn off the SSR
        digitalWrite(pinSSR,      LOW);
    }

    bool readPin(char* pinName, int pinID) {
      bool isHigh = digitalRead(pinID) == HIGH;
      log_debug("%s reads %s", pinName, statusString(isHigh));
      return isHigh;
    }

    // --- Sense helpers ---
    bool fanLoCall()    { return readPin("Fan Lo", pinFanLoSense); }
    bool fanHiCall()    { return readPin("Fan Hi", pinFanHiSense); }
    bool acCall()       { return readPin("Air Conditioner", pinACSense); }
    bool heatPumpCall() { return readPin("Heat Pump", pinHPSense); }
    bool furnaceCall() { return readPin("Furnace", FURN_SENSE); }

    // --- Relay control helpers ---
    void setPin(char* pinName, int pinID, bool toNC){
      log_debug("Setting %s (%d) to %d", pinName, pinID, toNC);
      digitalWrite(pinID, toNC ? HIGH : LOW);
    }
    void passThrough_FanLo(bool toNC)    { setPin("Fan Lo", pinFanLoOut, toNC);  }
    void passThrough_FanHi(bool toNC)    { setPin("Fan Hi", pinFanHiOut, toNC); }
    void passThrough_AirConditioner(bool toNC)       { setPin("Air Conditioner", pinACOut, toNC); }
    void passThrough_HeatPump(bool toNC) { setPin("Heat Pump", pinHPOut, toNC); }

    // --- SSR control ---
    void setSSR(bool on)        { isSSROn = on; setPin("Space Heater", pinSSR, on); }
    void passThrough_Furnace(bool toNC)  { setPin("Furnace", FURN_OUT, toNC);}

    // convert enum → readable string
    const char* roomNameString() const {
        switch (room) {
            case BEDROOM:     return "Bedroom";
            case LIVING_ROOM: return "Living Room";
            case GARAGE:      return "Garage";
            default:          return "Unknown";
        }
    }

    void preferHeatPump(){
      // Allow all calls to pass through
      passThrough_FanLo(true);
      passThrough_FanHi(true);
      passThrough_AirConditioner(true);
      passThrough_HeatPump(true);

      // but disable the SSR and furnace
      setSSR(false);
      if (hasFurnace){
        passThrough_Furnace(false);
      }
    }

    void denyHeatPump(){
      bool needsHeat = false;
      if (hasFurnace){
        needsHeat = heatPumpCall() || furnaceCall();
      }
      else{
        needsHeat = heatPumpCall();
      }

      if (needsHeat){
        log_debug("%s is asking for heat", roomNameString());
        passThrough_FanLo(false);
        passThrough_FanHi(false);
        passThrough_AirConditioner(false);
        passThrough_HeatPump(false);

        setSSR(true);
        if (hasFurnace){
          passThrough_Furnace(true);
        }
      }
      else{
        log_debug("%s is NOT asking for heat", roomNameString());
        passThrough_FanLo(true);
        passThrough_FanHi(true);
        passThrough_AirConditioner(true);
        passThrough_HeatPump(true);
        setSSR(false);
        if (hasFurnace){
          passThrough_Furnace(false);
        }
      }
    }

    void VerifyCalls(){
        fanLoCall();
        fanHiCall();
        acCall();
        heatPumpCall();
        if (hasFurnace){
          furnaceCall();
        }
    }
    
    void printPinStates() {
      char zoneHeatPumpStatus[LCD_LINE_LENGTH] = {};
      for (int i = 0; i < LCD_LINE_LENGTH; i++) {
        zoneHeatPumpStatus[i] = ' ';
      }

     // log_debug("Zone status: %s", zoneHeatPumpStatus);
      
      zoneHeatPumpStatus[0] = ('0' + zoneID); // Zone ID as char
      if (hasFurnace && furnaceCall()) {
        zoneHeatPumpStatus[1] = 'F';
      }
      else if (heatPumpCall()) {
        zoneHeatPumpStatus[1] = 'H';
      }
      else if (!heatPumpCall()) {
        zoneHeatPumpStatus[1] = 'N';
      }
      else {
        zoneHeatPumpStatus[1] = 'U';
      }

      if(isSSROn) {
        zoneHeatPumpStatus[2] = 'S'; // Space Heater is ON
      } else if(digitalRead(pinHPOut) == HIGH) {
        zoneHeatPumpStatus[2] = 'P'; // Heat Pump is PassThrough
      } else {
        zoneHeatPumpStatus[2] = 'N'; // assume its low, nothing is on
      }

      zoneHeatPumpStatus[3] = '\0'; // Null-terminate the string

      log_debug("Zone status: %s", zoneHeatPumpStatus);
      writeLCD(SECOND_LINE, 4*zoneID-4, zoneHeatPumpStatus);

    }

    // --- Make appropriate adjustments ---
    void Adjust(){
      log_debug("Requesting temperatures...");
      int outdoorTemp     = retrieveTemperature(Thermometer::Outside);
      int underbellyTemp  = retrieveTemperature(Thermometer::Underbelly);
      int thirdTemp       = retrieveTemperature(Thermometer::Third);

      writeLCD(FIRST_LINE, "O:%d  U:%d", outdoorTemp, underbellyTemp);
      log_debug("------------------");
      log_debug("Adjusting %s", roomNameString());
      VerifyCalls();

      bool shouldRunHeatPumps = outdoorTemp >= HEAT_PUMP_THRESHOLD_F;
      bool underbellyNeedsHeat = underbellyTemp < UNDERBELLY_TEMP_THRESHOLD_F;
      
      // Hysteresis logic: use different thresholds for on/off
      if (isInHeatMode) {
        // Currently in heat mode - need temp to rise above UPPER threshold to switch
        if (outdoorTemp >= HEAT_PUMP_HYSTERESIS_UPPER_F) {
          log_debug("Temperature rising above %dF, switching to heat pump mode", HEAT_PUMP_HYSTERESIS_UPPER_F);
          isInHeatMode = false;
          preferHeatPump();
        }
      } else {
        // Currently in heat pump mode - need temp to drop below LOWER threshold to switch
        if (outdoorTemp < HEAT_PUMP_HYSTERESIS_LOWER_F) {
          log_debug("Temperature dropped below %dF, switching to furnace + space heater mode", HEAT_PUMP_HYSTERESIS_LOWER_F);
          isInHeatMode = true;
          denyHeatPump();
        }
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
  delay(LOOP_DELAY_MS);
  
  log_debug("------------------------------------------------------------------------------------------");
  #if LCD == true
    handleButtonPress();
  #endif
  Zone1.Adjust();
  Zone2.Adjust();
  Zone3.Adjust();

  sensors.requestTemperatures();
  
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
  log_debug("Pins Off");
  
  startSensors();

  writeLCD(SECOND_LINE, " ");

}