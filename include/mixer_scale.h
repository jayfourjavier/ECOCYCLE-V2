#pragma once

#include <Arduino.h>
#include <HX711.h>
#include "defines.h"

#ifndef TO_CALIBRATE
#define TO_CALIBRATE 0
#endif

inline void setupScale(HX711 &scale)
{
    scale.begin(HX711_DAT_PIN, HX711_CLK_PIN);
    scale.set_offset(SCALE_CALIBRATION_OFFSET);
    scale.set_scale(SCALE_CALIBRATION_FACTOR);
}

inline void calibrateScale(HX711 &scale)
{
    if (!TO_CALIBRATE)
    {
        return;
    }

    Serial.println("\n\nCALIBRATION");
    Serial.println("===========");
    Serial.println("remove all weight from the loadcell");
    while (Serial.available())
    {
        Serial.read();
    }
    Serial.println("and press enter\n");

    while (Serial.available() == 0)
    {
    }

    Serial.println("Determine zero weight offset");
    scale.tare(20);
    int32_t offset = scale.get_offset();

    Serial.print("OFFSET: ");
    Serial.println(offset);
    Serial.println();

    Serial.println("place a weight on the loadcell");
    while (Serial.available())
    {
        Serial.read();
    }
    Serial.println("enter the weight in (whole) grams and press enter");

    uint32_t weight = 0;
    while (Serial.peek() != '\n')
    {
        if (Serial.available())
        {
            char ch = Serial.read();
            if (isdigit(ch))
            {
                weight *= 10;
                weight += static_cast<uint32_t>(ch - '0');
            }
        }
    }

    Serial.print("WEIGHT: ");
    Serial.println(weight);
    scale.calibrate_scale(weight, 20);
    float scaleFactor = scale.get_scale();

    Serial.print("SCALE:  ");
    Serial.println(scaleFactor, 6);
    Serial.print("\nuse scale.set_offset(");
    Serial.print(offset);
    Serial.print("); and scale.set_scale(");
    Serial.print(scaleFactor, 6);
    Serial.print(");\n");
    Serial.println("in the setup of your project");
    Serial.println("\n\n");

    Serial.printf("#define SCALE_CALIBRATION_OFFSET %ld\n", static_cast<long>(offset));
    Serial.printf("#define SCALE_CALIBRATION_FACTOR %.6f\n", scaleFactor);
    Serial.println();
}

// float getWeightGrams(sample size)
