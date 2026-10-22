#include "inputs_controller.h"
#include "defines.h"

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

String getTimestampString()
{
    time_t now = time(nullptr);
    struct tm timeinfo;

    if (now == 0 || !getLocalTime(&timeinfo))
    {
        return String("0000-00-00 00:00:00");
    }

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

void setupTime()
{
    // Set the system clock to GMT+8 (UTC+8) for local timestamps.
    setenv("TZ", "GMT-8", 1);
    tzset();
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("[TIME] Current time: %s\n", buffer);
    }
    else
    {
        Serial.println("[TIME] Failed to sync with NTP");
    }
}

void scanI2c()
{
    Serial.printf("%s [I2C] Scanning I2C bus...\n", getTimestampString().c_str());
    Wire.begin();

    byte error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.printf("%s [I2C] Device found at address 0x%02X\n",
                          getTimestampString().c_str(),
                          address);
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.printf("%s [I2C] Unknown error at address 0x%02X\n",
                          getTimestampString().c_str(),
                          address);
        }
    }

    if (nDevices == 0)
    {
        Serial.printf("%s [I2C] No I2C devices found\n", getTimestampString().c_str());
    }
    else
    {
        Serial.printf("%s [I2C] Scan complete\n", getTimestampString().c_str());
    }
}

void testInputsInLoop()
{
    static uint16_t lastState = 0xFFFF;

    uint16_t currentState = 0;
    currentState |= isFoodHopperCoverClosed() ? (1u << 0) : 0;
    currentState |= isSoilHopperCoverClosed() ? (1u << 1) : 0;
    currentState |= isSensorHome() ? (1u << 2) : 0;
    currentState |= isGateHome() ? (1u << 3) : 0;
    currentState |= isMixerSafePositionDetected() ? (1u << 4) : 0;

    if (currentState == lastState)
    {
        return;
    }

    lastState = currentState;

    Serial.printf("%s [INPUTS] FOOD=%d SOIL=%d SENSOR=%d GATE=%d MIXER=%d\n",
                  getTimestampString().c_str(),
                  (currentState & (1u << 0)) ? 1 : 0,
                  (currentState & (1u << 1)) ? 1 : 0,
                  (currentState & (1u << 2)) ? 1 : 0,
                  (currentState & (1u << 3)) ? 1 : 0,
                  (currentState & (1u << 4)) ? 1 : 0);
}
