#include <Arduino.h>
#include <FastAccelStepper.h>
#include "defines.h"
#include "inputs_controller.h"

FastAccelStepperEngine engine;
FastAccelStepper *sensorStepper = nullptr;
FastAccelStepper *mixerStepper = nullptr;
FastAccelStepper *gateStepper = nullptr;

static void configureStepper(FastAccelStepper *stepper,
                             uint8_t dirPin,
                             uint8_t pulsePin,
                             uint32_t speedHz,
                             uint32_t accel,
                             uint32_t microstep,
                             const char *name)
{
    if (!stepper)
    {
        Serial.printf("[%s] Failed to connect to pulse pin %d\n", name, pulsePin);
        return;
    }

    stepper->setDirectionPin(dirPin);
    stepper->setAutoEnable(true);
    stepper->setSpeedInHz(speedHz);
    stepper->setAcceleration(accel);

    Serial.printf("[%s] Stepper initialized on pulse pin %d, dir pin %d, speed %lu Hz, accel %lu, microstep %lu\n",
                  name,
                  pulsePin,
                  dirPin,
                  speedHz,
                  accel,
                  microstep);
}

void setupStepper()
{
    engine.init();

    sensorStepper = engine.stepperConnectToPin(SENSOR_STEPPER_PULSE_PIN);
    mixerStepper = engine.stepperConnectToPin(MIXER_STEPPER_PULSE_PIN);
    gateStepper = engine.stepperConnectToPin(GATE_STEPPER_PULSE_PIN);

    configureStepper(sensorStepper, SENSOR_STEPPER_DIR_PIN, SENSOR_STEPPER_PULSE_PIN, SENSOR_STEPPER_SPEED_HZ, SENSOR_STEPPER_ACCELERATION, SENSOR_STEPPER_MICROSTEP, "SENSOR");
    configureStepper(mixerStepper, MIXER_STEPPER_DIR_PIN, MIXER_STEPPER_PULSE_PIN, MIXER_STEPPER_SPEED_HZ, MIXER_STEPPER_ACCELERATION, MIXER_STEPPER_MICROSTEP, "MIXER");
    configureStepper(gateStepper, GATE_STEPPER_DIR_PIN, GATE_STEPPER_PULSE_PIN, GATE_STEPPER_SPEED_HZ, GATE_STEPPER_ACCELERATION, GATE_STEPPER_MICROSTEP, "GATE");
}

void homeMotor()
{
    Serial.println("[STEPPER] Home routine placeholder");
}

static void homeStepperToSensor(FastAccelStepper *stepper,
                                const char *name,
                                uint32_t searchSpeedHz,
                                int32_t searchStepCount,
                                bool (*isHomeDetected)())
{
    static bool homingStarted = false;
    static bool homingComplete = false;

    if (!stepper)
    {
        return;
    }

    if (homingComplete)
    {
        return;
    }

    if (!homingStarted)
    {
        stepper->setSpeedInHz(searchSpeedHz);
        stepper->move(searchStepCount, false);
        homingStarted = true;
        Serial.printf("%s [HOMING] %s stepper searching for home switch\n",
                      getTimestampString().c_str(),
                      name);
        return;
    }

    if (isHomeDetected())
    {
        stepper->stopMove();
        stepper->setCurrentPosition(0);
        homingComplete = true;
        Serial.printf("%s [HOMING] %s home switch reached; position reset to 0\n",
                      getTimestampString().c_str(),
                      name);
        return;
    }

    if (!stepper->isRunning())
    {
        stepper->move(searchStepCount, false);
    }
}

void homeSensorStepper()
{
    homeStepperToSensor(sensorStepper, "SENSOR", 120, -2000, isSensorHome);
}

void homeMixerStepper()
{
    Serial.println("[MIXER] homeMixerStepper() placeholder - sensor not implemented yet");
}

void homeGateStepper()
{
    Serial.println("[GATE] homeGateStepper() placeholder - sensor not implemented yet");
}

static void testStepper(FastAccelStepper *stepper,
                        const char *name,
                        uint32_t speedHz,
                        int32_t stepsPerMove,
                        uint32_t delayMs)
{
    static bool moveForward = true;
    static uint32_t lastMoveMs = 0;

    if (!stepper)
    {
        return;
    }

    if (millis() - lastMoveMs < delayMs)
    {
        return;
    }

    if (stepper->isRunning())
    {
        return;
    }

    const int32_t steps = moveForward ? stepsPerMove : -stepsPerMove;
    stepper->setSpeedInHz(speedHz);
    stepper->move(steps, false);
    moveForward = !moveForward;
    lastMoveMs = millis();

    Serial.printf("[%s] Stepper move %ld steps at %lu Hz\n", name, steps, speedHz);
}

void testHomingWithLimitSwitch()
{
    homeSensorStepper();
}

void testStepperBackAndForth()
{
    static bool homingComplete = false;

    if (!homingComplete)
    {
        homeSensorStepper();
        homingComplete = isSensorHome();
        return;
    }

    testStepper(sensorStepper, "SENSOR", 220, 220, 500);
}
