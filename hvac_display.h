#pragma once

#include <menu.h>
#include <menuIO/liquidCrystalOut.h>
#include <menuIO/serialIO.h>
#include <menuIO/stringIn.h>
#include "logging.h"

extern bool debugState;

#pragma region LCD
#define btnNONE 0
#define btnRIGHT 1
#define btnUP 2
#define btnDOWN 3
#define btnLEFT 4
#define btnSELECT 5
#define FIRST_LINE 0  // text position for first line
#define SECOND_LINE 1 // text position for second line
#define LCD_LINE_LENGTH 16
#define SOFT_DEBOUNCE_MS 100
#pragma endregion LCD

#pragma region Menu
#define MAX_DEPTH 3
#pragma endregion Menu

#pragma region LCD Pins
#define D04_LCD_DataPin0 4
#define D05_LCD_DataPin1 5
#define D06_LCD_DataPin2 6
#define D07_LCD_DataPin3 7
#define D08_LCD_RS_PIN 8
#define D09_LCD_ENABLE_PIN 9
#define D10_LCD_BACKLIGHT_PIN 10
#pragma endregion LCD

#pragma region Menu Setup
LiquidCrystal lcd(D08_LCD_RS_PIN, D09_LCD_ENABLE_PIN, D04_LCD_DataPin0, D05_LCD_DataPin1, D06_LCD_DataPin2, D07_LCD_DataPin3);

// debounce a button
int key_press_counter = 0;          // how many times we have seen new value
long key_press_previous_time = 0;   // the last time the output pin was sampled
byte key_press_debounce_count = 10; // number of millis/samples to consider before declaring a debounced input
byte current_state = 0;             // the debounced input value
bool navigateMenu = false;

TOGGLE(debugState, debugMenu, "Debug: ", doNothing, noEvent, wrapStyle,
       VALUE("No", true, doNothing, noEvent),
       VALUE("Yes", false, doNothing, noEvent));

MENU(mainMenu, "Main menu", doNothing, noEvent, wrapStyle,
     SUBMENU(debugMenu),
     EXIT("< Back"));

MENU_OUTPUTS(out, MAX_DEPTH, LIQUIDCRYSTAL_OUT(lcd, {0, 0, 16, 2}), NONE);

stringIn<0> strIn; // buffer size: 2^5 = 32 bytes, eventually use 0 for a single byte
serialIn serial(Serial);

NAVROOT(nav, mainMenu, MAX_DEPTH, serial, out);

byte read_LCD_buttons()
{                                   // read the buttons
    int adc_key_in = analogRead(0); // read the value from the sensor
    // value read: 0(0V), 130(0.64V), 306(1.49V), 479(2.33V), 722(3.5V), 1023(4.97V)
    int rightMax = 64;
    int upMax = 128;
    int downMax = 256;
    int leftMax = 512;
    int selectMax = 768;
    if (adc_key_in < 1020)
        log_debug("ADC Value = %d", adc_key_in);
    if (adc_key_in <= rightMax)
        return btnRIGHT; // 0
    if (adc_key_in <= upMax)
        return btnUP; // 99
    if (adc_key_in <= downMax)
        return btnDOWN; // 256
    if (adc_key_in <= leftMax)
        return btnLEFT; // 410
    if (adc_key_in <= selectMax)
        return btnSELECT; // 641
    if (adc_key_in > selectMax)
        return btnNONE;
}

byte key_press()
{
    // If we have gone on to the next millisecond
    if (millis() != key_press_previous_time)
    {
        byte this_button = read_LCD_buttons();

        if (this_button == current_state && key_press_counter > 0)
            key_press_counter--;

        if (this_button != current_state)
            key_press_counter++;

        // If the Input has shown the same value for long enough let's switch it
        if (key_press_counter >= key_press_debounce_count)
        {
            key_press_counter = 0;
            current_state = this_button;
            return this_button;
        }
        key_press_previous_time = millis();
    }
    return 0;
}

void handleButtonPress()
{
    // log_debug("Handling Button Presses");
    byte lcd_key = key_press(); // read the buttons

    switch (lcd_key)
    {
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
        // log_debug("no button press found");
        if (!navigateMenu)
        {
        }
        break;
    }
}

void writeLCD(int line, const char *fmt, ...)
{
    char buffer[LCD_LINE_LENGTH] = {};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    snprintf(buffer, sizeof(buffer), "%-16s", buffer);
    lcd.setCursor(0, line);
    lcd.print(buffer);
    log_debug("LCD Line %d: %s", line, buffer);
}

void writeLCD(int line, int position, const char *input)
{
    lcd.setCursor(position, line);
    lcd.print(input);

    Serial.print("LCD Line ");
    Serial.print(line);
    Serial.print(" Pos ");
    Serial.print(position);
    Serial.print(": ");
    Serial.println(input);
}

void writeError(int line, uint8_t value)
{
    char stringBuffer[LCD_LINE_LENGTH] = {};
    snprintf(stringBuffer, LCD_LINE_LENGTH, "Error: 0x%02X", value);
    writeLCD(line, stringBuffer);
}
#pragma endregion