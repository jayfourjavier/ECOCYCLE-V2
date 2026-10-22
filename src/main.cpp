#include <Arduino.h>
#include "utilities.h"
#include "wifi_controller.h"
#include "lcd_controller.h"
#include "machine_state.h"
#include "inputs_controller.h"

volatile bool g_mcpInterruptFlag = false;

void IRAM_ATTR onMcpInterrupt()
{
  g_mcpInterruptFlag = true;
}

void setup()
{
  Serial.begin(115200);
  setupWifi();
  setupTime();

  pinMode(MCP23017_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MCP23017_INT_PIN), onMcpInterrupt, FALLING);

  setupInputs();
  scanI2c();
  // setupLcd();
  // lcdPrintState(MachineState::IDLE_READY);
  delay(2000);
}

void loop()
{
  handleMcpInterrupts();
  testInputsInLoop();
}
