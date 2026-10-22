#include <Arduino.h>
#include <HX711.h>
#include "wifi_controller.h"
#include "lcd_controller.h"
#include "machine_state.h"
#include "inputs_controller.h"
#include "mcp23017_relay.h"
#include "mixer_scale.h"

HX711 scale;

extern String getTimestampString();
extern void setupTime();
extern void scanI2c();
extern void setupStepper();
extern void homeSensorStepper();
extern void homeMixerStepper();
extern void homeGateStepper();
extern void testStepperBackAndForth();
extern void testInputsInLoop();

volatile bool g_mcpInterruptFlag = false;

void IRAM_ATTR onMcpInterrupt()
{
  g_mcpInterruptFlag = true;
}

void setup()
{
  Serial.begin(115200);
  scanI2c();

  setupWifi();
  setupTime();
  setupInputs();
  setupRelays();
  setupLcd();
  setupStepper();
  setupScale(scale);

  pinMode(MCP23017_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MCP23017_INT_PIN), onMcpInterrupt, FALLING);

  lcdPrintState(MachineState::IDLE_READY);
  delay(2000);

  // turnOnBuzzer();
  // delay(2000);
  // turnOffBuzzer();
  // delay(1000);

  // turnOnMeatGrinder();
  // delay(2000);
  // turnOffMeatGrinder();
  // delay(1000);

  // turnOnSoilDispenser();
  // delay(20000);
  // turnOffSoilDispenser();
  // delay(1000);

  Serial.printf("%s [SCALE] Weight: %.2f g\n", getTimestampString().c_str(), getWeightGrams(scale, 10));
}

void loop()
{
  handleMcpInterrupts();
  testInputsInLoop();
  homeSensorStepper();
  // homeMixerStepper();
  // homeGateStepper();

  // testRelays1sCycle();
  // Serial.printf("%s [SCALE] Weight: %.2f g\n", getTimestampString().c_str(), getWeightGrams(scale, 10));
}
