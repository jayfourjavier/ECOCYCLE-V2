#pragma once

#include <Arduino.h>
#include <MCP23017.h>
#include "defines.h"

extern String getTimestampString();

// MCP23017 pin map for the limit switches.
// These inputs use external pull-down resistors, so they are configured as normal INPUTs.
static MCP23017 mcpInputs(MCP23017_INPUTS_ADDRESS);
extern volatile bool g_mcpInterruptFlag;

inline bool readInput(uint8_t pin);
inline bool isFoodHopperCoverClosed();
inline bool isSoilHopperCoverClosed();
inline bool isSensorHome();
inline bool isGateHome();
inline bool isMixerSafePositionDetected();

// Backward compatibility alias for legacy naming.
inline bool isProbeHome()
{
    return isSensorHome();
}

inline void setupInputs()
{
    mcpInputs.begin(false);

    mcpInputs.pinMode1(MCP23017_FOOD_HOPPER_COVER_PIN, INPUT);
    mcpInputs.pinMode1(MCP23017_SOIL_HOPPER_COVER_PIN, INPUT);
    mcpInputs.pinMode1(MCP23017_SENSOR_HOME_PIN, INPUT);
    mcpInputs.pinMode1(MCP23017_GATE_HOME_PIN, INPUT);
    mcpInputs.pinMode1(MCP23017_MIXER_SAFE_PIN, INPUT);
    mcpInputs.setPullup(MCP23017_MIXER_SAFE_PIN, true);

    // Configure MCP23017 to alert on any input change on the monitored pins.
    mcpInputs.mirrorInterrupts(true);
    mcpInputs.setInterruptPolarity(0);
    mcpInputs.enableInterrupt(MCP23017_SENSOR_HOME_PIN, CHANGE);
    mcpInputs.enableInterrupt(MCP23017_FOOD_HOPPER_COVER_PIN, CHANGE);
    mcpInputs.enableInterrupt(MCP23017_SOIL_HOPPER_COVER_PIN, CHANGE);
    mcpInputs.enableInterrupt(MCP23017_GATE_HOME_PIN, CHANGE);
    mcpInputs.enableInterrupt(MCP23017_MIXER_SAFE_PIN, CHANGE);

    Serial.printf("%s [INPUTS] MCP23017 limit switch inputs setup complete\n",
                  getTimestampString().c_str());
}

inline void handleMcpInterrupts()
{
    if (!g_mcpInterruptFlag)
    {
        return;
    }

    g_mcpInterruptFlag = false;

    uint16_t flags = mcpInputs.getInterruptFlagRegister();
    Serial.printf("%s [INTA] flags=0x%04X\n", getTimestampString().c_str(), flags);

    if (flags & (1u << MCP23017_SENSOR_HOME_PIN))
    {
        Serial.printf("%s [INTA] SENSOR_HOME changed -> %d\n",
                      getTimestampString().c_str(),
                      isSensorHome() ? 1 : 0);
    }

    if (flags & (1u << MCP23017_FOOD_HOPPER_COVER_PIN))
    {
        Serial.printf("%s [INTA] FOOD_HOPPER_COVER changed -> %d\n",
                      getTimestampString().c_str(),
                      isFoodHopperCoverClosed() ? 1 : 0);
    }

    if (flags & (1u << MCP23017_SOIL_HOPPER_COVER_PIN))
    {
        Serial.printf("%s [INTA] SOIL_HOPPER_COVER changed -> %d\n",
                      getTimestampString().c_str(),
                      isSoilHopperCoverClosed() ? 1 : 0);
    }

    if (flags & (1u << MCP23017_GATE_HOME_PIN))
    {
        Serial.printf("%s [INTA] GATE_HOME changed -> %d\n",
                      getTimestampString().c_str(),
                      isGateHome() ? 1 : 0);
    }

    if (flags & (1u << MCP23017_MIXER_SAFE_PIN))
    {
        Serial.printf("%s [INTA] MIXER_SAFE changed -> %d\n",
                      getTimestampString().c_str(),
                      isMixerSafePositionDetected() ? 1 : 0);
    }
}

inline bool readInput(uint8_t pin)
{
    if (pin > 15)
    {
        return false;
    }

    return mcpInputs.read1(pin) == HIGH;
}

inline bool isFoodHopperCoverClosed()
{
    return readInput(MCP23017_FOOD_HOPPER_COVER_PIN);
}

inline bool isSoilHopperCoverClosed()
{
    return readInput(MCP23017_SOIL_HOPPER_COVER_PIN);
}

inline bool isSensorHome()
{
    return readInput(MCP23017_SENSOR_HOME_PIN);
}

inline bool isGateHome()
{
    return readInput(MCP23017_GATE_HOME_PIN);
}

inline bool isMixerSafePositionDetected()
{
    // Mixer-safe is wired with an internal pull-up on the MCP23017,
    // so the valid active state is LOW when the switch is closed to ground.
    return readInput(MCP23017_MIXER_SAFE_PIN) == LOW;
}
