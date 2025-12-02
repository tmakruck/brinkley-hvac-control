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

enum class Thermometer {
  Outside,
  Underbelly
};

#ifndef CONSTANTS
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
    #pragma endregion LCD

    

    // Zone thermostat inputs (yellow wires)
    #define Zone1_In 24   // Bedroom thermostat trigger (Y)
    #define Zone2_In 25   // Living Room thermostat trigger (Y)
    #define Zone3_In 26   // Garage thermostat trigger (Y)
    #define Furnace_In 27 // Furnace trigger (blue wire from Zone 1 thermostat)

    // Heat pump & furnace outputs
    #define Zone1_HeatPump_Out 30
    #define Zone2_HeatPump_Out 31
    #define Zone3_HeatPump_Out 32
    #define Furnace_Out        33

    // Space heater outputs
    #define Zone1_Space_Out 36
    #define Zone2_Space_Out 37
    #define Zone3_Space_Out 38

    // Temperature Probes
    #define Temperature_Data 30
  #pragma endregion Pin Definitions

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
    bool debugState = false;
    
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
      if (adc_key_in > 1000) return btnNONE;
      if (adc_key_in < 75) return btnRIGHT;
      if (adc_key_in < 218) return btnDOWN;
      if (adc_key_in < 392) return btnUP;
      if (adc_key_in < 600) return btnLEFT;
      if (adc_key_in < 800) return btnSELECT;
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

    void writeLCD(int line, int, uint8_t value) {
    }

    void writeLCD(int line, bool input) {
      char *stringBuffer = input ? "true" : "false";
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
      // Serial.print("LCD Line ");
      // Serial.print(line + 1);
      // Serial.print("=");
      // Serial.println(stringBuffer);
    }
  #endif  //menu setup
#pragma endregion


OneWire oneWire(Temperature_Data);
DallasTemperature sensors(&oneWire);

// Store each sensor's unique 64-bit address
DeviceAddress outsideAddr = { 0x28, 0xEC, 0x81, 0x87, 0x00, 0xE0, 0x49, 0xBC };
DeviceAddress underbellyAddr = { 0x28, 0xA6, 0x0B, 0x87, 0x00, 0xA6, 0x0C, 0x38 };

// Function to get address by enum
DeviceAddress* getAddress(Thermometer t) {
  switch (t) {
    case Thermometer::Outside:    return &outsideAddr;
    case Thermometer::Underbelly: return &underbellyAddr;
    default: return nullptr;
  }
}

const char* getName(Thermometer t) {
  switch (t) {
    case Thermometer::Outside:    return "Outside";
    case Thermometer::Underbelly: return "Underbelly";
    default:                      return "Unknown";
  }
}

int retrieveTemperature(Thermometer t) {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  DeviceAddress* address = getAddress(t);
  int temperature = static_cast<int>(round(sensors.getTempF(*address)));

  // Check if reading was successful
  if (temperature != DEVICE_DISCONNECTED_C)
  {
    Serial.print("Temperature for ");
    Serial.print(getName(t));
    Serial.print(" is: ");
    Serial.println(temperature);
  }
  else
  {
    Serial.println("Error: Could not read temperature data");
  }

  return temperature;
}

// Generic handler for zones 2 and 3
void handleZone(int triggerPin, int heatPumpPin, int spacePin, int outdoorTemp) {
  bool zoneActive = digitalRead(triggerPin) == HIGH;

  if (zoneActive) {
    if (outdoorTemp >= 35) {
      digitalWrite(heatPumpPin, HIGH);
      digitalWrite(spacePin, LOW);
    } else {
      digitalWrite(heatPumpPin, LOW);
      digitalWrite(spacePin, HIGH);
    }
  } else {
    digitalWrite(heatPumpPin, LOW);
    digitalWrite(spacePin, LOW);
  }
}


void loop() {
  #if LCD == true
    handleButtonPress();
  #endif

  // Read triggers
  const bool zone1Trigger   = digitalRead(Zone1_In) == HIGH;
  const bool furnaceTrigger = digitalRead(Furnace_In) == HIGH;
  const bool zone2Trigger   = digitalRead(Zone2_In) == HIGH;
  const bool zone3Trigger   = digitalRead(Zone3_In) == HIGH;

  // Read temperatures
  const int outdoorTemp    = retrieveTemperature(Thermometer::Outside);
  const int underbellyTemp = retrieveTemperature(Thermometer::Underbelly);

  writeLCD(FIRST_LINE, "Out:%2d Under:%2d", outdoorTemp, underbellyTemp);

  // -------------------------
  // Furnace requirement logic
  // -------------------------
  // Furnace is required if underbelly is cold OR Zone1 thermostat requests furnace
  bool zone1Active     = zone1Trigger || furnaceTrigger;
  bool furnaceRequired = (underbellyTemp < 45) || (zone1Active && outdoorTemp < 35);

  // -------------------------
  // Zone 1 (Bedroom)
  // -------------------------
  if (zone1Active) {
    if (outdoorTemp >= 35 && !furnaceRequired) {
      // Warm enough and furnace not forced: run heat pump
      digitalWrite(Zone1_HeatPump_Out, HIGH);
      digitalWrite(Zone1_Space_Out, LOW);
      digitalWrite(Furnace_Out, LOW);
    } else {
      // Furnace required either by cold underbelly or cold outdoor
      digitalWrite(Furnace_Out, HIGH);
      digitalWrite(Zone1_HeatPump_Out, LOW);
      digitalWrite(Zone1_Space_Out, HIGH); // assist with space heater
    }
  } else {
    // Zone1 idle, but furnace may still be required due to underbelly
    if (furnaceRequired) {
      digitalWrite(Furnace_Out, HIGH);
      digitalWrite(Zone1_Space_Out, HIGH); // keep space heater on with furnace
      digitalWrite(Zone1_HeatPump_Out, LOW);
    } else {
      digitalWrite(Furnace_Out, LOW);
      digitalWrite(Zone1_Space_Out, LOW);
      digitalWrite(Zone1_HeatPump_Out, LOW);
    }
  }

  // -------------------------
  // Zone 2 (Living Room)
  // -------------------------
  handleZone(Zone2_In, Zone2_HeatPump_Out, Zone2_Space_Out, outdoorTemp);

  // -------------------------
  // Zone 3 (Garage)
  // -------------------------
  handleZone(Zone3_In, Zone3_HeatPump_Out, Zone3_Space_Out, outdoorTemp);

  delay(250);
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

  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

    // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  Serial.print("Device 0 Address: ");
  printAddress(Thermometer::Outside);
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(Thermometer::Underbelly);
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  Serial.println("------------------------------------------------------------------------------------------");
  #if LCD == true
    lcd.begin(16, 2);
  #endif
  writeLCD(FIRST_LINE, "Initializing");

  // --- Inputs ---
  pinMode(Zone1_In, INPUT);    // Bedroom thermostat (yellow wire)
  pinMode(Zone2_In, INPUT);    // Living Room thermostat (yellow wire)
  pinMode(Zone3_In, INPUT);    // Garage thermostat (yellow wire)
  pinMode(Furnace_In, INPUT);  // Furnace trigger (blue wire from Zone 1)

  // --- Outputs: Heat pumps & furnace ---
  pinMode(Zone1_HeatPump_Out, OUTPUT);
  pinMode(Zone2_HeatPump_Out, OUTPUT);
  pinMode(Zone3_HeatPump_Out, OUTPUT);
  pinMode(Furnace_Out, OUTPUT);

  // --- Outputs: Space heaters ---
  pinMode(Zone1_Space_Out, OUTPUT);
  pinMode(Zone2_Space_Out, OUTPUT);
  pinMode(Zone3_Space_Out, OUTPUT);


  Serial.println("Pins Configured");

  // Initialize all outputs to OFF (LOW)
  digitalWrite(Zone1_HeatPump_Out, LOW);
  digitalWrite(Zone2_HeatPump_Out, LOW);
  digitalWrite(Zone3_HeatPump_Out, LOW);
  digitalWrite(Furnace_Out, LOW);

  digitalWrite(Zone1_Space_Out, LOW);
  digitalWrite(Zone2_Space_Out, LOW);
  digitalWrite(Zone3_Space_Out, LOW);


  Serial.println("Pins Off");
  

  startSensors();

  
  writeLCD(SECOND_LINE, "Pins Set");

}