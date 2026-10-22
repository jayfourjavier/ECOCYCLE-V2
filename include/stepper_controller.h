#pragma once

#include <Arduino.h>
#include <FastAccelStepper.h>
#include "defines.h"

class StepperController
{
public:
    StepperController(uint8_t pulsePin, uint8_t directionPin, uint8_t enablePin)
        : pulsePin_(pulsePin),
          directionPin_(directionPin),
          enablePin_(enablePin),
          stepper_(nullptr),
          initialized_(false)
    {
    }

    void begin(uint32_t speedHz = STEPPER_DEFAULT_SPEED_HZ,
               uint32_t acceleration = STEPPER_DEFAULT_ACCELERATION)
    {
        if (initialized_)
        {
            return;
        }

        engine_.init();
        stepper_ = engine_.stepperConnectToPin(pulsePin_);

        if (!stepper_)
        {
            Serial.printf("[STEPPER] Failed to connect pulse pin %d\n", pulsePin_);
            return;
        }

        stepper_->setDirectionPin(directionPin_);
        stepper_->setEnablePin(enablePin_);
        stepper_->setAutoEnable(true);
        stepper_->setSpeedInHz(speedHz);
        stepper_->setAcceleration(acceleration);

        initialized_ = true;
    }

    bool isValid() const
    {
        return stepper_ != nullptr;
    }

    void setSpeedInHz(uint32_t speedHz)
    {
        if (stepper_)
        {
            stepper_->setSpeedInHz(speedHz);
        }
    }

    void setAcceleration(uint32_t acceleration)
    {
        if (stepper_)
        {
            stepper_->setAcceleration(acceleration);
        }
    }

    void enable()
    {
        if (stepper_)
        {
            stepper_->enableOutputs();
        }
    }

    void disable()
    {
        if (stepper_)
        {
            stepper_->disableOutputs();
        }
    }

    void moveRelative(long steps)
    {
        if (stepper_)
        {
            stepper_->move(steps, false);
        }
    }

    void moveAbsolute(long position)
    {
        if (stepper_)
        {
            stepper_->moveTo(position, false);
        }
    }

    void stop()
    {
        if (stepper_)
        {
            stepper_->stopMove();
        }
    }

    bool isRunning() const
    {
        return stepper_ ? stepper_->isRunning() : false;
    }

private:
    uint8_t pulsePin_;
    uint8_t directionPin_;
    uint8_t enablePin_;
    FastAccelStepperEngine engine_;
    FastAccelStepper *stepper_;
    bool initialized_;
};
