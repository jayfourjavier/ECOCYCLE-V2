#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include "defines.h"
#include "machine_state.h"

extern String getTimestampString();

static LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);

inline void lcdToUpper(char *destination, size_t size, const char *source)
{
    if (destination == nullptr || source == nullptr || size == 0)
    {
        return;
    }

    size_t index = 0;
    while (source[index] != '\0' && index < (size - 1))
    {
        char ch = source[index];
        destination[index] = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - 32) : ch;
        index++;
    }

    destination[index] = '\0';
}

inline void setupLcd()
{
    Serial.printf("%s [LCD] Setting up LCD\n", getTimestampString().c_str());
    Wire.begin();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ECOCYCLE");
    lcd.setCursor(0, 1);
    lcd.print("SYSTEM READY");
    Serial.printf("%s [LCD] LCD setup complete\n", getTimestampString().c_str());
}

inline void lcdPrint(uint8_t row, const char *text)
{
    if (row >= LCD_ROWS || text == nullptr)
    {
        return;
    }

    char buffer[LCD_COLUMNS + 1];
    lcdToUpper(buffer, sizeof(buffer), text);

    Serial.printf("%s [LCD] lcdPrint row=%u text=%s\n",
                  getTimestampString().c_str(),
                  row,
                  buffer);

    lcd.setCursor(0, row);
    lcd.print("                    ");
    lcd.setCursor(0, row);
    lcd.print(buffer);
}

inline void lcdPrintCentered(const char *text, uint8_t row)
{
    if (row >= LCD_ROWS || text == nullptr)
    {
        return;
    }

    char buffer[LCD_COLUMNS + 1];
    lcdToUpper(buffer, sizeof(buffer), text);

    size_t length = strlen(buffer);
    int padding = (length >= LCD_COLUMNS) ? 0 : (LCD_COLUMNS - static_cast<int>(length)) / 2;

    Serial.printf("%s [LCD] lcdPrintCentered row=%u text=%s\n",
                  getTimestampString().c_str(),
                  row,
                  buffer);

    lcd.setCursor(0, row);
    lcd.print("                    ");
    lcd.setCursor(padding, row);
    lcd.print(buffer);
}

inline void lcdPrintStatus(const char *title, const char *message)
{
    if (title == nullptr || message == nullptr)
    {
        return;
    }

    char titleBuffer[LCD_COLUMNS + 1];
    char messageBuffer[LCD_COLUMNS + 1];
    lcdToUpper(titleBuffer, sizeof(titleBuffer), title);
    lcdToUpper(messageBuffer, sizeof(messageBuffer), message);

    Serial.printf("%s [LCD] lcdPrintStatus title=%s message=%s\n",
                  getTimestampString().c_str(),
                  titleBuffer,
                  messageBuffer);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(titleBuffer);
    lcd.setCursor(0, 1);
    lcd.print(messageBuffer);
}

inline void lcdPrintState(MachineState state)
{
    Serial.printf("%s [LCD] lcdPrintState state=", getTimestampString().c_str());

    switch (state)
    {
    case MachineState::BOOT:
        Serial.println("BOOT");
        lcdPrintCentered("BOOT", 0);
        break;
    case MachineState::SELF_TEST:
        Serial.println("SELF_TEST");
        lcdPrintCentered("SELF_TEST", 0);
        break;
    case MachineState::RECOVERY_CHECK:
        Serial.println("RECOVERY_CHECK");
        lcdPrintCentered("RECOVERY_CHECK", 0);
        break;
    case MachineState::IDLE_READY:
        Serial.println("IDLE_READY");
        lcdPrintCentered("IDLE_READY", 0);
        break;
    case MachineState::PRECHECK:
        Serial.println("PRECHECK");
        lcdPrintCentered("PRECHECK", 0);
        break;
    case MachineState::GRIND_FOOD_WASTE:
        Serial.println("GRIND_FOOD_WASTE");
        lcdPrintCentered("GRIND_FOOD_WASTE", 0);
        break;
    case MachineState::VERIFY_FOOD_WASTE:
        Serial.println("VERIFY_FOOD_WASTE");
        lcdPrintCentered("VERIFY_FOOD_WASTE", 0);
        break;
    case MachineState::FEED_LOAM:
        Serial.println("FEED_LOAM");
        lcdPrintCentered("FEED_LOAM", 0);
        break;
    case MachineState::VERIFY_BATCH_WEIGHT:
        Serial.println("VERIFY_BATCH_WEIGHT");
        lcdPrintCentered("VERIFY_BATCH_WEIGHT", 0);
        break;
    case MachineState::PREPARE_MIXING:
        Serial.println("PREPARE_MIXING");
        lcdPrintCentered("PREPARE_MIXING", 0);
        break;
    case MachineState::MIXING:
        Serial.println("MIXING");
        lcdPrintCentered("MIXING", 0);
        break;
    case MachineState::MOVE_TO_PROBE_SAFE:
        Serial.println("MOVE_TO_PROBE_SAFE");
        lcdPrintCentered("MOVE_TO_PROBE_SAFE", 0);
        break;
    case MachineState::LOWER_PROBE:
        Serial.println("LOWER_PROBE");
        lcdPrintCentered("LOWER_PROBE", 0);
        break;
    case MachineState::STABILIZE_AND_SAMPLE:
        Serial.println("STABILIZE_AND_SAMPLE");
        lcdPrintCentered("STABILIZE_AND_SAMPLE", 0);
        break;
    case MachineState::RAISE_PROBE:
        Serial.println("RAISE_PROBE");
        lcdPrintCentered("RAISE_PROBE", 0);
        break;
    case MachineState::PREPARE_DISCHARGE:
        Serial.println("PREPARE_DISCHARGE");
        lcdPrintCentered("PREPARE_DISCHARGE", 0);
        break;
    case MachineState::OPEN_BOTTOM_GATE:
        Serial.println("OPEN_BOTTOM_GATE");
        lcdPrintCentered("OPEN_BOTTOM_GATE", 0);
        break;
    case MachineState::VERIFY_DISCHARGE:
        Serial.println("VERIFY_DISCHARGE");
        lcdPrintCentered("VERIFY_DISCHARGE", 0);
        break;
    case MachineState::FINALIZE_BATCH:
        Serial.println("FINALIZE_BATCH");
        lcdPrintCentered("FINALIZE_BATCH", 0);
        break;
    case MachineState::UPLOAD_RECORD:
        Serial.println("UPLOAD_RECORD");
        lcdPrintCentered("UPLOAD_RECORD", 0);
        break;
    case MachineState::COMPLETE:
        Serial.println("COMPLETE");
        lcdPrintCentered("COMPLETE", 0);
        break;
    case MachineState::PAUSED_SAFE:
        Serial.println("PAUSED_SAFE");
        lcdPrintCentered("PAUSED_SAFE", 0);
        break;
    case MachineState::FAULT:
        Serial.println("FAULT");
        lcdPrintCentered("FAULT", 0);
        break;
    default:
        Serial.println("UNKNOWN");
        lcdPrintCentered("UNKNOWN", 0);
        break;
    }
}
