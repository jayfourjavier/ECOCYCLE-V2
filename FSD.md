# Food Waste–Loam Soil Mixing Machine

## Functional System Documentation (FSD)

**Document:** `fsd.md`  
**System controller:** ESP32  
**Status:** Initial functional specification for state-machine implementation  
**Batch target:** Approximately 1,000 g total mixture  
**Nominal recipe:** 100 g food waste + 900 g loam soil

---

## 1. Purpose and scope

This document describes the machine's functional architecture, sensors, actuators, operating sequence, safety interlocks, data model, and proposed ESP32 state machine. It is intended to guide implementation and testing; it is not a certified machinery-safety assessment.

The machine performs these functions:

1. Detects whether the food-waste and loam-soil hopper covers are closed.
2. Measures the approximate material level in each hopper using VL53L0X time-of-flight (ToF) sensors.
3. Grinds food waste using an AC motor switched through a relay/contactor arrangement.
4. Adds loam soil using a geared DC motor controlled by a relay.
5. Mixes the batch using a stepper-driven mixer.
6. Moves a temperature/moisture probe into the mixture only after the mixer reaches a designated safe position.
7. Reads soil temperature using a PT100 and MAX31865, and soil moisture using an analog capacitive sensor.
8. Opens the mixer bottom using a stepper mechanism after sampling.
9. Measures the mixer assembly's weight using a load cell and HX711.
10. Displays operating status on a 20×4 I²C LCD.
11. Uploads one CSV-formatted batch record to Firebase.

The machine is a prototype. Software interlocks complement, but do not replace, physical guards, correctly rated electrical protection, emergency-stop hardware, and safe mechanical design.

## 2. System overview

### 2.1 Controller and subsystems

The ESP32 is the central controller. Its firmware should use a **non-blocking finite-state machine (FSM)**: each state performs a bounded action, checks conditions, updates outputs, and returns control quickly. Long delays should not block sensor monitoring, emergency handling, or LCD updates.

| Subsystem | Hardware | Main function |
| --- | --- | --- |
| Main controller | ESP32 | FSM, interlocks, sensor processing, LCD, Firebase |
| Food-waste grinder | AC motor, relay and contactor | Grinds food waste |
| Soil feeder | Geared DC motor and relay | Feeds loam soil |
| Mixer drive | Stepper motor and driver; inductive position sensor | Mixes batch and identifies designated safe position |
| Probe lift | Stepper motor and driver; home limit switch | Moves probe up/down; home means fully up |
| Bottom gate | Stepper motor and driver; home limit switch | Opens/closes mixer bottom; home means fully open |
| Probe temperature | PT100 + MAX31865 | Measures mixture temperature |
| Probe moisture | Analog capacitive soil-moisture sensor | Measures moisture signal |
| Hopper covers | Active-high switches, COM/NO, external pull-down | HIGH means cover closed |
| Hopper level | Two VL53L0X ToF sensors | Estimates food-waste and soil levels |
| Batch weight | Load cell → HX711 | Measures mixer assembly weight/change; HX711 wiring: bare = GND, red = 3.3V, white = DAT, blue = CLK |
| User display | 20×4 I²C LCD | Shows state, readings, prompts and faults |
| Batch logging | ESP32 Wi-Fi + Firebase | Stores one CSV record per completed batch |

### 2.2 Conceptual sequence

```text
Power on
   ↓
Initialize and validate sensors/actuators
   ↓
Wait for covers closed and machine ready
   ↓
Confirm mixer/gate/probe positions
   ↓
Grind food waste → add/verify food-waste quantity
   ↓
Feed loam soil → verify target batch weight
   ↓
Close/secure mixer bottom and establish safe position
   ↓
Mix batch
   ↓
Move mixer to probe-safe position
   ↓
Lower probe → allow readings to stabilize → sample
   ↓
Raise probe to home
   ↓
Open bottom gate → discharge batch
   ↓
Verify discharge / reset batch counters
   ↓
Log batch to Firebase → return to READY
```

This is a conceptual sequence. The exact order of gate closure, weighing, grinding and feeding must be confirmed against the final mechanical arrangement. The controller must never assume that a motor reached its destination merely because a movement command was issued.

## 3. Operating assumptions and unresolved design decisions

The following assumptions should be made explicit in firmware and verified during commissioning:

- Nominal batch mass is 1,000 g: 100 g food waste and 900 g loam soil.
- The mixer load cell may measure the mixer, gate, and vessel tare as well as material. A calibrated tare and a defined weighing window are therefore required.
- The food-waste grinder and soil feeder are controlled by relay outputs. Relay feedback is not currently specified.
- The mixer stepper has an inductive sensor for its designated safe position. The sensor's active level, trigger geometry, and acceptable position window must be configured.
- The probe-lift and bottom-gate home switches are active HIGH using COM/NO and an external pull-down. HIGH means the relevant home position is detected.
- Hopper-cover switches are active HIGH when the cover is closed. LOW means open, disconnected, or otherwise not confirmed closed.
- VL53L0X distance is a **level estimate**, not a direct mass measurement. Irregular, dark, reflective, or uneven material can affect readings. Calibrate the distance-to-level mapping for each hopper.
- The analog capacitive sensor's output is a moisture-related signal. Convert it to a moisture percentage only after calibration using representative loam samples and a defined reference method.
- Temperature and moisture are sampled only after the probe is lowered to a known depth and the readings have stabilized.
- Firebase upload failure must not cause the machine to repeat a physical batch or become stuck indefinitely. Keep a local pending record and retry separately if storage is available.
- The exact food-waste dose mechanism is not specified. The load cell measures total mass unless separate metering or a reliable baseline-difference procedure is implemented. Total weight alone cannot prove that the batch contains exactly 100 g food waste and 900 g soil.
- If the machine has no independent confirmation of a closed mixer bottom, add a suitable position sensor or a mechanical verification procedure. Do not treat the bottom-gate home switch (home = open) as proof that the gate is closed.

## 4. Input signals and interpretation

### 4.1 Digital inputs

| Input | Logical interpretation | Required response |
| --- | --- | --- |
| Food hopper cover | HIGH = closed; LOW = not confirmed closed | Do not start grinding or feeding with cover open |
| Soil hopper cover | HIGH = closed; LOW = not confirmed closed | Do not run soil feeder with cover open |
| Probe-lift home | HIGH = probe fully up | Required before mixer motion and discharge |
| Bottom-gate home | HIGH = gate fully open | Required only when open position is intended |
| Mixer safe-position sensor | Configure polarity; detected = safe position | Probe may descend only when detected and stable |
| Emergency stop (recommended hardware input) | Define fail-safe polarity and wiring | Immediately de-energize hazardous outputs and latch fault |
| Driver fault inputs (if available) | Driver-specific | Stop affected movement and latch fault |

**Important:** A LOW cover signal does not distinguish an open cover from a broken wire. For safety, both conditions must be treated as “not safely closed.” Debounce switches in software, but do not use software debounce as a substitute for suitable safety hardware.

### 4.2 Analog and digital measurements

| Measurement | Interface | Firmware treatment |
| --- | --- | --- |
| Soil temperature | PT100 → MAX31865 | Check sensor/fault status; reject invalid or out-of-range data |
| Soil moisture | Analog capacitive sensor → ESP32 ADC | Use a calibrated ADC configuration, filtering and valid-range checks |
| Mixer weight | Load cell → HX711 | Tare at a known empty condition; filter samples; detect unstable readings |
| Food hopper distance | VL53L0X I²C | Check sensor status and range; convert distance to calibrated level |
| Soil hopper distance | VL53L0X I²C | Check sensor status and range; convert distance to calibrated level |

If both VL53L0X modules share one I²C bus, their default I²C address may conflict. Use XSHUT pins and assign distinct addresses during initialization, or use a suitable I²C multiplexer. Verify the exact breakout board voltage and logic-level requirements.

### 4.3 Weight measurement and recipe control

Define the measured net batch mass as:

`net_batch_g = stable_loaded_weight_g - tare_weight_g`

Use a calibrated tolerance around the 1,000 g target, for example a configurable `BATCH_WEIGHT_TOLERANCE_G`. Do not hard-code a tolerance until the load cell, frame, vibration and feeding repeatability have been tested.

Recommended checks:

- Establish tare only when the vessel is empty and mechanically settled.
- Sample the HX711 several times and require the readings to remain within a stability band.
- Stop the soil feeder before checking final batch weight.
- If the weight is below target, permit controlled top-up only if the feeding mechanism supports it.
- If the weight exceeds the permitted maximum, stop the sequence and report an over-weight fault.
- If the weight is invalid, drifting or unstable, do not use it to authorize discharge or declare the batch complete.
- If separate 100 g food waste and 900 g soil quantities are requirements, provide a method to measure each addition independently (for example, weigh the food-waste addition before soil feeding, or use separate metering). Total batch mass cannot verify the ratio by itself.

## 5. Outputs and actuator rules

| Output | Actuator | General control rule |
| --- | --- | --- |
| Grinder command | AC motor relay/contactor | Run only with relevant cover closed, no fault, and valid sequence state |
| Soil feeder command | DC geared motor relay | Run only with soil cover closed and a valid feed state |
| Mixer stepper command | Stepper driver | Move only when probe is confirmed home/up and gate position is safe for mixing |
| Probe-lift stepper command | Stepper driver | Descend only at mixer-safe position; raise on completion, timeout, or recoverable interruption |
| Bottom-gate stepper command | Stepper driver | Open only after probe is home and mixing/sampling is complete; close before mixing if the mechanism supports it |
| LCD | I²C | Refresh without blocking the FSM |
| Firebase writer | Wi-Fi/network client | Upload completed record asynchronously or in a bounded state |

The software should use centralized output functions such as `stopAllProcessActuators()` and `enterFault(code)`. The emergency stop should remove power from hazardous actuators through appropriate hardware; an ESP32 command alone is not a sufficient emergency-stop circuit.

For the AC grinder, use a correctly rated contactor and motor overload protection. The relay should operate the contactor coil when appropriate, not carry motor current unless explicitly rated for the motor's load and inrush. Provide suitable isolation, enclosure, grounding and electrical protection.

## 6. State machine specification

### 6.1 State list

| State | Purpose | Exit condition |
| --- | --- | --- |
| `BOOT` | Initialize pins, serial, I²C, sensor libraries and safe outputs | Initialization sequence started |
| `SELF_TEST` | Validate critical sensors, communications and known positions | Required checks pass or fault |
| `RECOVERY_CHECK` | Determine whether actuators are in a known safe condition after restart | Safe recovery possible or operator intervention required |
| `IDLE_READY` | Await start request and required conditions | Start requested and interlocks pass |
| `PRECHECK` | Confirm covers, probe up, gate condition, mixer safe state and valid measurements | All required conditions pass |
| `GRIND_FOOD_WASTE` | Run grinder for the configured process interval or until dose condition | Dose/operation complete, or timeout/fault |
| `VERIFY_FOOD_WASTE` | Verify food-waste addition if a measurement method exists | Food dose accepted or fault |
| `FEED_LOAM` | Run soil feeder toward target batch mass | Target reached or timeout/fault |
| `VERIFY_BATCH_WEIGHT` | Stop feeder and confirm stable mass | Weight in range or controlled correction/fault |
| `PREPARE_MIXING` | Ensure probe is up and gate is in mixing-safe condition | Preconditions satisfied |
| `MIXING` | Run mixer stepper through the configured mixing routine | Mixing routine complete or fault |
| `MOVE_TO_PROBE_SAFE` | Move mixer to the probe-safe location | Inductive sensor detected and stable |
| `LOWER_PROBE` | Move probe downward to sampling position | Position reached or timeout/fault |
| `STABILIZE_AND_SAMPLE` | Wait for temperature/moisture readings to settle and validate them | Valid sample or sample fault/timeout |
| `RAISE_PROBE` | Return probe to home/up position | Home switch detected or timeout/fault |
| `PREPARE_DISCHARGE` | Confirm probe home and mixer stopped | Discharge conditions satisfied |
| `OPEN_BOTTOM_GATE` | Move gate to open position | Gate-open home detected or timeout/fault |
| `VERIFY_DISCHARGE` | Confirm expected weight reduction/empty condition if measurable | Discharge accepted or operator intervention |
| `FINALIZE_BATCH` | Freeze batch values, increment batch number, create record | Record prepared |
| `UPLOAD_RECORD` | Send CSV record to Firebase with bounded timeout | Upload succeeds or record queued for retry |
| `COMPLETE` | Display completion and return to ready | Completion message duration elapsed |
| `PAUSED_SAFE` | Stop process outputs after a recoverable interruption | Operator resume/cancel after safety checks |
| `FAULT` | Stop hazardous outputs, record fault, show code | Manual reset and recovery checks pass |

### 6.2 State transition overview

```text
BOOT → SELF_TEST → RECOVERY_CHECK → IDLE_READY → PRECHECK
                                              ↓
GRIND_FOOD_WASTE → VERIFY_FOOD_WASTE → FEED_LOAM → VERIFY_BATCH_WEIGHT
                                              ↓
PREPARE_MIXING → MIXING → MOVE_TO_PROBE_SAFE → LOWER_PROBE
                                              ↓
STABILIZE_AND_SAMPLE → RAISE_PROBE → PREPARE_DISCHARGE
                                              ↓
OPEN_BOTTOM_GATE → VERIFY_DISCHARGE → FINALIZE_BATCH
                                              ↓
UPLOAD_RECORD → COMPLETE → IDLE_READY
```

Any critical interlock violation, actuator timeout, invalid critical sensor, or emergency stop transitions to `FAULT` or a specifically designed safe-stop state. The fault handler must take priority over normal state progression.

### 6.3 State entry/exit pattern

Each state should follow a consistent pattern:

1. On entry, initialize its timer, counters and target values once.
2. Set only the outputs required for that state.
3. On each loop iteration, poll emergency stop and critical interlocks first.
4. Update sensors and filter values.
5. Check success conditions and timeouts.
6. Transition to the next state without long blocking delays.
7. On exit, stop state-specific outputs unless the next state explicitly owns them.

Use `millis()`-based elapsed-time checks rather than long `delay()` calls for process sequencing. Sensor libraries may have their own blocking behavior; bound or isolate those operations where possible.

## 7. Safety interlocks and fault handling

### 7.1 Mandatory movement interlocks

- **Probe down:** permitted only when the mixer is stopped, the mixer safe-position sensor is detected and stable, the bottom gate is confirmed in the required closed/secure state, and no fault is active.
- **Mixer movement:** permitted only when the probe is confirmed up/home and the gate is in the mechanically safe position.
- **Bottom gate open:** permitted only after mixing and sampling are complete, the probe is confirmed up, and the mixer is stopped.
- **Grinding:** permitted only when the food-waste hopper cover is confirmed closed and the system is in the grinding state.
- **Soil feeding:** permitted only when the soil hopper cover is confirmed closed and the system is in the soil-feed state.
- **Fault/reset:** reset must not automatically restart a motor. Require deliberate operator reset and re-check all safety conditions.

The exact gate interlock depends on the mechanical design. If the gate's only limit switch detects the **open** position, add another means of confirming closed/secure position before allowing mixing or probe descent.

### 7.2 Recommended fault codes

| Code | Condition | Default response |
| --- | --- | --- |
| `E001` | Initialization or critical sensor failure | Keep actuators off; block start |
| `E002` | Food hopper cover not confirmed closed | Stop grinder; fault or pause |
| `E003` | Soil hopper cover not confirmed closed | Stop feeder; fault or pause |
| `E004` | Mixer did not reach safe position in time | Stop mixer; prohibit probe descent |
| `E005` | Probe failed to reach home/up | Stop motion; prohibit mixer and gate movement |
| `E006` | Probe failed to reach sampling position | Stop probe movement; attempt only a designed safe recovery |
| `E007` | Bottom gate failed to reach open position | Stop gate; require operator inspection |
| `E008` | HX711/load-cell invalid or unstable | Stop feeding; do not accept batch |
| `E009` | Batch underweight/overweight | Stop feeding; correction or operator action |
| `E010` | PT100/MAX31865 fault or invalid temperature | Mark sample invalid; do not report valid batch sample |
| `E011` | Moisture ADC reading invalid/out of calibrated range | Mark moisture invalid |
| `E012` | VL53L0X reading invalid/out of range | Mark hopper level unknown; inhibit decisions relying on level |
| `E013` | Actuator timeout | Stop affected actuator; latch fault |
| `E014` | Firebase upload failed | Keep batch locally pending; do not repeat physical batch |
| `E015` | Emergency stop activated | Hardware de-energization; latch fault |
| `E016` | Unexpected restart / unknown actuator position | Require safe recovery and possibly operator intervention |

Fault codes are proposed identifiers and can be changed during implementation.

### 7.3 Power loss and restart

On boot, all motor/contactor outputs must initialize to OFF before network setup or sensor initialization. Do not resume a partially completed batch automatically. The machine may have stopped with the probe down, gate moving, or material partially discharged. Determine safe positions and ask for operator intervention if the position cannot be established safely.

### 7.4 Mechanical and electrical safety notes

- Provide a physical emergency stop that removes actuator power through a suitable safety-rated or appropriately engineered circuit.
- Use motor overload protection and correctly rated contactors for the AC grinder.
- Guard pinch points at the mixer, gate, stepper-driven mechanisms and feeders.
- Ensure stepper drivers, motors, supplies, wiring and enclosures are rated for stall current, duty cycle, dust, moisture and mechanical load.
- Use suitable grounding, fusing/overcurrent protection, strain relief, separation of mains and low-voltage wiring, and protective enclosures.
- Avoid exposing the capacitive probe, load-cell wiring or ESP32 electronics to wet soil or washdown unless suitably protected.
- Treat sensor disagreement, disconnected wiring and impossible measurements as faults, not as permission to proceed.

## 8. Sensor processing and calibration

### 8.1 PT100 + MAX31865

- Configure the MAX31865 for the actual PT100 wiring (2-, 3- or 4-wire) and the board's reference resistor.
- Check the library's fault flags before accepting temperature.
- Reject NaN, open/short-circuit faults and values outside a configurable plausible range.
- During sampling, collect several readings and require a stability condition over a defined window.
- Record temperature in degrees Celsius.
- Calibration and sensor insertion depth must be documented. The probe's reading may not represent the entire batch if the mixture is uneven.

### 8.2 Analog capacitive moisture sensor

- Read the ADC repeatedly and use a median or moving average to reduce noise.
- Record raw ADC counts during commissioning.
- Calibrate dry/wet or reference moisture points using the actual soil type and a documented reference method.
- If reporting percentage, document the conversion and its limits; do not imply laboratory accuracy from a generic capacitive probe.
- Require the probe to be inserted to a repeatable depth and held for a repeatable sampling interval.
- Keep the raw value in diagnostic logs even if the final CSV stores a converted value.

### 8.3 HX711 load cell

- Calibrate using known weights across the expected operating range.
- Record tare and calibration factor in configuration.
- Use a stable-reading test before making feed/discharge decisions.
- Ensure the load path is mechanically isolated from hoses, cables, frame contact and actuator forces that can bias the reading.
- Account for vibration during mixing; do not use a vibrating measurement as a precise batch-weight confirmation.

### 8.4 VL53L0X hopper level

- Mount each sensor with a clear view of the material surface and protect it from dust where practical.
- Convert measured distance to level using the measured empty-hopper reference distance and usable hopper height.
- Treat invalid status, saturation and out-of-range values as “unknown.”
- Use level as a replenishment/operator prompt unless a validated geometry and repeatability test supports stronger decisions.
- Display estimated level as percentage or descriptive bands only after calibration.

## 9. LCD user interface

The 20×4 LCD should present a concise live status. Example layouts:

**Ready**

```text
FoodWaste Soil Mixer
State: READY
Food: 68% Soil: 42%
Batch: 0012  1000g
```

**Mixing**

```text
Batch: 0013
State: MIXING
Weight: 1002 g
Please wait...
```

**Sampling**

```text
Batch: 0013
State: SOIL SAMPLE
Temp: 28.4 C
Moisture: 41.2 %
```

**Fault**

```text
!!! MACHINE FAULT !!!
Code: E004
Mixer safe position
not detected
Reset after inspection
```

LCD refresh should be rate-limited (for example, several times per second or slower) and must not block the FSM. Avoid clearing the display on every loop iteration because that causes flicker.

## 10. Batch data and Firebase

### 10.1 Required record

The requested logical record is:

`timestamp (Unix time), batch#, temperature, humidity`

The described moisture sensor measures soil moisture, not relative humidity in air. To avoid ambiguity, the recommended column name is `moisture`, unless “humidity” is intentionally the existing application field name.

Recommended CSV header:

```csv
timestamp,batch,temperature_c,moisture_pct
```

Example only:

```csv
1792600000,13,28.4,41.2
```

The example values are illustrative, not measured data. If the application requires the literal field `humidity`, use:

```csv
timestamp,batch,temperature_c,humidity
```

but document that the value represents **soil moisture** rather than air relative humidity.

### 10.2 Timestamp and batch number

- Use Unix epoch seconds in UTC.
- Synchronize the ESP32 clock using NTP when Wi-Fi is available.
- Do not silently write an invalid timestamp if time synchronization fails. Either block final logging until time is valid or record a clear invalid-time marker plus a local sequence identifier.
- Persist the batch counter if batch numbers must remain unique after power loss. A RAM-only counter resets on reboot.
- Assign a unique record ID or idempotency key so a network retry cannot create duplicate batch records.

### 10.3 Upload behavior

- Create the record only after the sample and batch finalization rules have completed.
- Upload with a finite timeout.
- If upload succeeds, mark the record sent.
- If upload fails, preserve the record in a local queue if nonvolatile storage is available, show a warning, and retry in a separate non-blocking process.
- Do not rerun grinding, feeding, mixing, or discharge simply because Firebase is unavailable.
- Keep credentials out of source control and use Firebase rules that restrict device access to the required paths.

### 10.4 Data validity

Temperature and moisture should be marked invalid if their sensors fault or fail the stability/valid-range checks. If the CSV schema must remain four columns, define a consistent invalid representation and document it; otherwise add validity columns such as `temperature_valid` and `moisture_valid`.

## 11. Firmware architecture proposal

A maintainable PlatformIO project can be split into modules:

```text
src/
  main.cpp                 // setup(), loop(), top-level orchestration
  machine_fsm.cpp/.h       // states, transitions, state entry/exit
  safety.cpp/.h            // interlocks, emergency stop, fault latch
  actuators.cpp/.h         // grinder, feeder, mixer, probe and gate commands
  sensors.cpp/.h           // PT100, moisture ADC, HX711, ToF and switches
  batch_manager.cpp/.h     // recipe, batch ID, tare, weight checks
  lcd_ui.cpp/.h             // 20x4 display rendering
  firebase_logger.cpp/.h    // timestamp, CSV serialization, queue and upload
  config.h                  // pins, thresholds, timeouts and calibration
```

Only `main.cpp` should define `setup()` and `loop()`. Shared variables should be exposed through clear interfaces, not duplicated as separate global definitions in multiple `.cpp` files. Prefer a `MachineContext` or small subsystem classes with explicit state over many unrelated global variables.

### 11.1 Main loop responsibilities

Each loop iteration should, in order:

1. Read emergency-stop and critical interlocks.
2. Update input/sensor snapshots when due.
3. Evaluate safety conditions and latched faults.
4. Execute one bounded FSM step.
5. Update LCD if due.
6. Process queued Firebase work if due.
7. Yield to ESP32 background tasks as needed.

### 11.2 Suggested configuration constants

Keep all tunable values in one configuration module:

- `TARGET_BATCH_G = 1000`
- `TARGET_FOOD_WASTE_G = 100`
- `TARGET_LOAM_G = 900`
- `BATCH_WEIGHT_TOLERANCE_G` (set after calibration)
- `MIX_DURATION_MS` or a motion-based mixing recipe
- `PROBE_DOWN_TIMEOUT_MS`
- `PROBE_UP_TIMEOUT_MS`
- `MIXER_SAFE_POSITION_TIMEOUT_MS`
- `GATE_OPEN_TIMEOUT_MS`
- `GRINDER_MAX_RUN_MS`
- `SOIL_FEED_MAX_RUN_MS`
- `SENSOR_STABILITY_WINDOW_MS`
- `FIREBASE_UPLOAD_TIMEOUT_MS`
- `COVER_DEBOUNCE_MS`
- calibrated moisture conversion and hopper-level mappings

Do not invent final timeout or tolerance values without measuring the actual mechanism. Set conservative limits during commissioning.

## 12. Pseudocode

```text
setup():
    set all hazardous outputs OFF immediately
    initialize serial, GPIO, I2C and required buses
    initialize LCD and display BOOT
    initialize sensor drivers
    load calibration and persistent batch counter
    initialize Wi-Fi/Firebase service without blocking the machine
    state = SELF_TEST

loop():
    updateEmergencyStop()
    updateCoverInputs()
    updatePositionInputs()
    updateSensorSnapshotsIfDue()

    if emergencyStopActive:
        stopAllProcessActuators()
        latchFault(E015)

    if criticalInterlockBrokenDuringMotion:
        stopAffectedActuatorImmediately()
        latchAppropriateFault()

    if faultLatched:
        stopHazardousOutputs()
        updateFaultLCD()
        handleManualResetOnlyIfSafe()
        return

    switch state:
        case SELF_TEST:
            validateCriticalSensorsAndDrivers()
            if allRequiredChecksPass: transition(RECOVERY_CHECK)
            else: transition(FAULT)

        case RECOVERY_CHECK:
            verifyProbeUpAndRequiredGate/MixerPositions()
            if safeKnownCondition: transition(IDLE_READY)
            else: requestOperatorRecovery()

        case IDLE_READY:
            keepProcessActuatorsOff()
            updateHopperLevels()
            if startRequested: transition(PRECHECK)

        case PRECHECK:
            if coversClosed and probeUp and positionsSafe and sensorsValid:
                startNewBatchContext()
                transition(GRIND_FOOD_WASTE)

        case GRIND_FOOD_WASTE:
            runGrinderOnlyWhileFoodCoverClosed()
            stopGrinderWhenDoseOrConfiguredRunCompletes()
            transition(VERIFY_FOOD_WASTE)

        case VERIFY_FOOD_WASTE:
            if foodDoseVerified: transition(FEED_LOAM)
            else: transition(FAULT or operator correction)

        case FEED_LOAM:
            runSoilFeederOnlyWhileSoilCoverClosed()
            stopFeederAtTargetMassOrTimeout()
            transition(VERIFY_BATCH_WEIGHT)

        case VERIFY_BATCH_WEIGHT:
            if stableWeightWithinTolerance: transition(PREPARE_MIXING)
            else: applyAllowedCorrectionOrFault()

        case PREPARE_MIXING:
            if probeUp and gateSafeForMixing: transition(MIXING)

        case MIXING:
            runConfiguredMixingMotion()
            if mixingComplete: stopMixer(); transition(MOVE_TO_PROBE_SAFE)

        case MOVE_TO_PROBE_SAFE:
            moveMixerTowardSafePosition()
            if safeSensorStable: stopMixer(); transition(LOWER_PROBE)
            if timeout: latchFault(E004)

        case LOWER_PROBE:
            if mixerStopped and safePositionStable and gateSecure:
                lowerProbe()
            if samplePositionReached: transition(STABILIZE_AND_SAMPLE)
            if timeout: latchFault(E006)

        case STABILIZE_AND_SAMPLE:
            if temperatureAndMoistureStableAndValid:
                saveBatchSample()
                transition(RAISE_PROBE)
            if timeout: markSampleInvalid(); transition(RAISE_PROBE)

        case RAISE_PROBE:
            raiseProbeUntilHome()
            if probeHome: transition(PREPARE_DISCHARGE)
            if timeout: latchFault(E005)

        case PREPARE_DISCHARGE:
            if probeHome and mixerStopped: transition(OPEN_BOTTOM_GATE)

        case OPEN_BOTTOM_GATE:
            openGate()
            if gateOpenHome: transition(VERIFY_DISCHARGE)
            if timeout: latchFault(E007)

        case VERIFY_DISCHARGE:
            if dischargeVerified or operatorConfirmsDischarge:
                transition(FINALIZE_BATCH)

        case FINALIZE_BATCH:
            freezeSampleAndBatchMetadata()
            incrementAndPersistBatchNumber()
            createPendingCsvRecord()
            transition(UPLOAD_RECORD)

        case UPLOAD_RECORD:
            tryBoundedUploadWithoutRepeatingPhysicalActions()
            if uploadSuccess or recordQueuedLocally: transition(COMPLETE)

        case COMPLETE:
            displayBatchComplete()
            if completionDisplayElapsed: transition(IDLE_READY)
```

This pseudocode describes the intended architecture. It must be adapted to the actual actuator mechanics, available position sensors, and selected motor drivers before it is used to control the machine.

## 13. Commissioning and acceptance tests

Test each function independently before running a complete batch.

1. **Power-on safe state:** confirm all hazardous outputs remain OFF during boot, Wi-Fi connection and sensor initialization.
2. **Cover interlocks:** open each cover and verify its associated motor cannot start; disconnect a switch wire and verify the result is treated as unsafe.
3. **Stepper home switches:** test probe-up and gate-open detection, including switch bounce and a failed/disconnected switch.
4. **Mixer safe position:** confirm the inductive sensor is repeatable; verify probe descent is impossible when the sensor is not detected.
5. **Gate interlock:** verify the gate is confirmed closed/secure before mixing and probe descent. If not measurable, add the necessary sensing.
6. **Load-cell calibration:** test tare, known weights, stability and over/underweight handling.
7. **Feeder behavior:** verify the feeder stops at the intended target and cannot continue indefinitely if weight does not change.
8. **Sensor validation:** test PT100 fault detection, moisture ADC range, ToF invalid readings and I²C initialization failures.
9. **Motion timeouts:** deliberately prevent each actuator from reaching its destination and confirm that it stops and latches a fault.
10. **Power interruption:** interrupt power during safe idle and during controlled test motions; verify no automatic restart of a partial batch.
11. **Network loss:** disconnect Wi-Fi during logging and verify the physical cycle does not repeat and the record remains pending if local storage is implemented.
12. **Full batch:** verify actual mass, recipe repeatability, mixing quality, sample repeatability, discharge behavior and unique batch records across multiple cycles.

Acceptance thresholds should be documented after calibration and trial runs. Record firmware version, calibration values, date, and test results.

## 14. Known limitations and next design decisions

Before final firmware implementation, specify:

- ESP32 board and pin assignments.
- Stepper drivers, motor travel limits, speed/acceleration and position feedback.
- Whether the mixer has a separate gate-closed sensor.
- How the food-waste portion is weighed or metered independently from the soil.
- Whether the load cell measures total batch mass, mixer contents only, or the entire mixer assembly including moving parts.
- Exact mixer mixing routine (time, rotations, direction and speed).
- Probe sampling depth and temperature/moisture stability thresholds.
- Moisture calibration method and whether the Firebase field is named `moisture` or `humidity`.
- Hopper geometry and distance-to-level calibration.
- Firebase data path, authentication method, offline queue persistence and batch-number persistence.
- Emergency-stop hardware design and safe response for each actuator.
- Whether a human operator must confirm discharge when automatic empty-state detection is unavailable.

## 15. Revision history

| Version | Date | Notes |
|---|---|---|
| 0.1 | 2026-10-22 | Initial FSD based on the described machine and proposed FSM |

---

**Implementation principle:** Sensor readings and motor commands are not proof of physical completion. Use position feedback, bounded timeouts, stable measurements, latched faults, and explicit interlocks before permitting the next state.
