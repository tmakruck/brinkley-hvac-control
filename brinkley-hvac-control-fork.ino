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
unsigned long temperatureReadTimer = 0;


#ifndef CONSTANTS
  const int UNDERBELLY_TEMP_THRESHOLD_F = 50;
  const int TEMPERATURE_READ_INTERVAL_MS = 5000;
  const int INVALID_TEMP_THRESHOLD = -100;
  
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
// DeviceAddress thirdAddr      = { 0x28, 0x44, 0x71, 0x87, 0x00, 0x6B, 0x20, 0x12 };
Thermometer OutsideThermometer;
Thermometer UnderbellyThermometer;
TemperatureController* tempController = nullptr;

#pragma region HVAC setup

HVACZoneConfig zone1Config;
HVACZoneConfig zone2Config;
HVACZoneConfig zone3Config;

HVACZone Zone1;
HVACZone Zone2;
HVACZone Zone3;
#pragma endregion

void loop() {
  unsigned long currentTime = millis();
  unsigned long timeDiff = currentTime - temperatureReadTimer;
  // Check if 10 seconds (10000 ms) have passed since last read
  if (timeDiff >= TEMPERATURE_READ_INTERVAL_MS) {
    //log_debug("Requesting temperatures...");
    
    // Request temperatures FIRST, then read after conversion
    tempController->requestTemperatures();
    delay(LOOP_DELAY_MS);  // Wait for conversion to complete (~750ms)
    
    int outdoorTemp     = OutsideThermometer.retrieveTemperature();
    int underbellyTemp  = UnderbellyThermometer.retrieveTemperature();

    if (outdoorTemp < INVALID_TEMP_THRESHOLD or underbellyTemp < INVALID_TEMP_THRESHOLD) {
      log_info("Invalid temperature readings detected, skipping adjustments");
      log_info("Outdoor Temp: %d, Underbelly: %d", outdoorTemp, underbellyTemp);
      Zone1.fallbackToDefaultBehavior();
      Zone2.fallbackToDefaultBehavior();
      Zone3.fallbackToDefaultBehavior();
    }

    writeLCD(FIRST_LINE, "O:%d U:%d", outdoorTemp, underbellyTemp);
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
  temperatureReadTimer = millis()-20000;  // Initialize timer to 20 seconds ago to force immediate read

  writeLCD(SECOND_LINE, " ");

  OutsideThermometer    = Thermometer("Outside",    outsideAddr);
  UnderbellyThermometer = Thermometer("Underbelly", underbellyAddr);

  Thermometer* thermometers[] = {
      &OutsideThermometer,
      &UnderbellyThermometer,
  };

  log_debug("Setting Temp Controller");
  tempController = new TemperatureController(Temperature_Data, thermometers);
  log_debug("Setting Zone Configs");
  zone1Config = HVACZoneConfig("Bedroom", ZONE1_START, SSR1_HEAT, 35, FURN_SENSE, UNDERBELLY_TEMP_THRESHOLD_F);
  zone2Config = HVACZoneConfig("Living Room", ZONE2_START, SSR2_HEAT, 35);
  zone3Config = HVACZoneConfig("Garage", ZONE3_START, SSR3_HEAT, 39);

  Zone1 = HVACZone(zone1Config);  
  Zone2 = HVACZone(zone2Config);  
  Zone3 = HVACZone(zone3Config);
}