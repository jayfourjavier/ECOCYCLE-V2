#pragma once

#include <Arduino.h>
#include <MCP23017.h>
#include "defines.h"

extern String getTimestampString();

static MCP23017 mcpRelay(MCP23017_RELAY_ADDRESS);
static uint8_t g_relayState[16] = {0};
static bool g_soilDispenserPositiveOn = false;
static bool g_soilDispenserNegativeOn = false;
static bool g_soilDispenserOn = false;
static bool g_buzzerOn = false;
static bool g_meatGrinderOn = false;

constexpr uint8_t RELAY_SOIL_DISPENSER_POSITIVE = 6;
constexpr uint8_t RELAY_SOIL_DISPENSER_NEGATIVE = 5;
constexpr uint8_t RELAY_BUZZER = 10;
constexpr uint8_t RELAY_MEAT_GRINDER = 8;

inline uint8_t relayNumberToPin(uint8_t relayNumber)
{
    if (relayNumber < 1 || relayNumber > 16)
    {
        return 255;
    }

    if (relayNumber % 2 == 1)
    {
        return 7 - ((relayNumber - 1) / 2);
    }

    return 15 - ((relayNumber - 2) / 2);
}

inline bool relayOn(uint8_t relayNumber)
{
    if (relayNumber < 1 || relayNumber > 16)
    {
        return false;
    }

    const uint8_t pin = relayNumberToPin(relayNumber);
    if (pin == 255)
    {
        return false;
    }

    const uint8_t index = relayNumber - 1;
    if (g_relayState[index])
    {
        return true;
    }

    g_relayState[index] = 1;
    mcpRelay.write1(pin, LOW);
    Serial.printf("%s [RELAY] relay %u ON (pin %u)\n",
                  getTimestampString().c_str(), relayNumber, pin);
    return true;
}

inline bool relayOff(uint8_t relayNumber)
{
    if (relayNumber < 1 || relayNumber > 16)
    {
        return false;
    }

    const uint8_t pin = relayNumberToPin(relayNumber);
    if (pin == 255)
    {
        return false;
    }

    const uint8_t index = relayNumber - 1;
    if (!g_relayState[index])
    {
        return true;
    }

    g_relayState[index] = 0;
    mcpRelay.write1(pin, HIGH);
    Serial.printf("%s [RELAY] relay %u OFF (pin %u)\n",
                  getTimestampString().c_str(), relayNumber, pin);
    return true;
}

inline void setRelay(uint8_t relayNumber, bool enabled)
{
    if (enabled)
    {
        relayOn(relayNumber);
    }
    else
    {
        relayOff(relayNumber);
    }
}

inline void setSoilDispenserPositive(bool enabled)
{
    if (g_soilDispenserPositiveOn == enabled)
    {
        return;
    }

    g_soilDispenserPositiveOn = enabled;
    g_soilDispenserOn = g_soilDispenserPositiveOn || g_soilDispenserNegativeOn;
    setRelay(RELAY_SOIL_DISPENSER_POSITIVE, enabled);
}

inline void setSoilDispenserNegative(bool enabled)
{
    if (g_soilDispenserNegativeOn == enabled)
    {
        return;
    }

    g_soilDispenserNegativeOn = enabled;
    g_soilDispenserOn = g_soilDispenserPositiveOn || g_soilDispenserNegativeOn;
    setRelay(RELAY_SOIL_DISPENSER_NEGATIVE, enabled);
}

inline void turnOnSoilDispenser()
{
    if (g_soilDispenserOn)
    {
        return;
    }

    setSoilDispenserPositive(true);
    setSoilDispenserNegative(true);
}

inline void turnOffSoilDispenser()
{
    if (!g_soilDispenserOn)
    {
        return;
    }

    setSoilDispenserPositive(false);
    setSoilDispenserNegative(false);
}

inline void setBuzzer(bool enabled)
{
    if (g_buzzerOn == enabled)
    {
        return;
    }

    g_buzzerOn = enabled;
    setRelay(RELAY_BUZZER, enabled);
}

inline void turnOnBuzzer()
{
    if (g_buzzerOn)
    {
        return;
    }

    setBuzzer(true);
}

inline void turnOffBuzzer()
{
    if (!g_buzzerOn)
    {
        return;
    }

    setBuzzer(false);
}

inline void setMeatGrinder(bool enabled)
{
    if (g_meatGrinderOn == enabled)
    {
        return;
    }

    g_meatGrinderOn = enabled;
    setRelay(RELAY_MEAT_GRINDER, enabled);
}

inline void turnOnMeatGrinder()
{
    if (g_meatGrinderOn)
    {
        return;
    }

    setMeatGrinder(true);
}

inline void turnOffMeatGrinder()
{
    if (!g_meatGrinderOn)
    {
        return;
    }

    setMeatGrinder(false);
}

inline void setAllRelaysOff()
{
    for (uint8_t relayNumber = 1; relayNumber <= 16; ++relayNumber)
    {
        relayOff(relayNumber);
    }
}

inline void setupRelays()
{
    mcpRelay.begin(false);

    for (uint8_t pin = 0; pin < 16; ++pin)
    {
        mcpRelay.pinMode1(pin, OUTPUT);
        mcpRelay.write1(pin, HIGH);
    }

    for (uint8_t i = 0; i < 16; ++i)
    {
        g_relayState[i] = 0;
    }

    Serial.printf("%s [RELAY] MCP23017 relay expander at 0x%02X setup complete\n",
                  getTimestampString().c_str(), MCP23017_RELAY_ADDRESS);
}

inline void testRelays1sCycle()
{
    static uint8_t relayIndex = 1;
    static uint32_t lastToggleMs = 0;
    static bool relayEnabled = false;

    const uint32_t intervalMs = 1000;
    const uint32_t now = millis();

    if (now - lastToggleMs < intervalMs)
    {
        return;
    }

    lastToggleMs = now;

    if (!relayEnabled)
    {
        setAllRelaysOff();
        relayOn(relayIndex);
        relayEnabled = true;
        Serial.printf("%s [RELAY] test cycle relay %u ON\n",
                      getTimestampString().c_str(), relayIndex);
    }
    else
    {
        relayOff(relayIndex);
        relayEnabled = false;
        relayIndex++;
        if (relayIndex > 16)
        {
            relayIndex = 1;
        }
        Serial.printf("%s [RELAY] test cycle moved to relay %u\n",
                      getTimestampString().c_str(), relayIndex);
    }
}
