// For Menu

#include <DallasTemperature.h>

// Custom Classes 
#include "Thermometer.h"
#include "HvacZone.h"
#include "StateType.h"
#include "logging.h"
#include "TemperatureController.h"
#include "hvac_display.h"

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



#ifndef CONSTANTS
  const int UNDERBELLY_TEMP_THRESHOLD_F = 50;
  
  // Must be > 750 ms for Dallas Temp sensors to work properly
  const int LOOP_DELAY_MS = 800; // Dallas sensors: 750ms + 50ms buffer


  #pragma region Pin Definitions


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
  // Store each sensor's unique 64-bit address
DeviceAddress outsideAddr    = { 0x28, 0xEC, 0x81, 0x87, 0x00, 0xE0, 0x49, 0xBC };
DeviceAddress underbellyAddr = { 0x28, 0xA6, 0x0B, 0x87, 0x00, 0xA6, 0x0C, 0x38 };
DeviceAddress thirdAddr      = { 0x28, 0x44, 0x71, 0x87, 0x00, 0x6B, 0x20, 0x12 };
Thermometer OutsideThermometer    = Thermometer("Outside",    outsideAddr);
Thermometer UnderbellyThermometer = Thermometer("Underbelly", underbellyAddr);
Thermometer ThirdThermometer      = Thermometer("Third",      thirdAddr);

Thermometer thermometers[] = {
    OutsideThermometer,
    UnderbellyThermometer,
    ThirdThermometer
};
TemperatureController tempController(Temperature_Data, thermometers);

#pragma region HVAC setup

HVACZoneConfig zone1Config = HVACZoneConfig("Bedroom", ZONE1_START, SSR1_HEAT, 35, FURN_SENSE, UNDERBELLY_TEMP_THRESHOLD_F);
HVACZoneConfig zone2Config = HVACZoneConfig("Living Room", ZONE2_START, SSR2_HEAT, 35);
HVACZoneConfig zone3Config = HVACZoneConfig("Garage", ZONE3_START, SSR3_HEAT, 39);

HVACZone Zone1(zone1Config);  
HVACZone Zone2(zone2Config);  
HVACZone Zone3(zone3Config);
#pragma endregion

void loop() {
  unsigned long currentTime = millis();
  unsigned long timeDiff = currentTime - temperatureReadTimer;
  // Check if 10 seconds (10000 ms) have passed since last read
  if (timeDiff >= 5000) {
    log_info("Requesting temperatures...");
    outdoorTemp     = OutsideThermometer.retrieveTemperature();
    underbellyTemp  = UnderbellyThermometer.retrieveTemperature();
    thirdTemp       = ThirdThermometer.retrieveTemperature();

    

    if (outdoorTemp < -100 or underbellyTemp < -100) {
      log_info("Invalid temperature readings detected, skipping adjustments");
      Zone1.fallbackToDefaultBehavior();
      Zone2.fallbackToDefaultBehavior();
      Zone3.fallbackToDefaultBehavior();
    }
            

    
    writeLCD(FIRST_LINE, "O:%d%s U:%d", outdoorTemp, isHysteresisLowMode ? "L" : "H", underbellyTemp);
    tempController.requestTemperatures();
    temperatureReadTimer = currentTime;  // Reset the timer
  }

  #if LCD == true
    handleButtonPress();
  #endif
  
  Zone1.DoLoop();
  Zone2.DoLoop();
  Zone3.DoLoop();

}



void setup() {
  Serial.begin(9600);
  log_debug("------------------------------------------------------------------------------------------");
  lcd.begin(16, 2);
  writeLCD(FIRST_LINE, "Initializing");

  pinMode(FURN_SENSE,     INPUT_PULLUP);
  pinMode(FURN_OUT,       OUTPUT);
  digitalWrite(FURN_OUT,  HIGH);
  pinMode(FURN_12V,       OUTPUT);
  digitalWrite(FURN_12V,  HIGH);
  pinMode(HP_12V,         OUTPUT);
  digitalWrite(HP_12V,    HIGH);
  
  delay(LOOP_DELAY_MS);
  temperatureReadTimer = millis()-20000;  // Initialize timer
  lastDailyResetTime = millis()-20000;    // Initialize daily reset timer

  writeLCD(SECOND_LINE, " ");

}