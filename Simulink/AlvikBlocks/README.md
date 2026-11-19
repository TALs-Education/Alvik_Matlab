# Arduino Alvik Simulink block

A Simulink block for the Arduino Alvik robot, running on its Arduino Nano ESP32. It's a
MATLAB System object (`AlvikRobot.m`) with a small C++ driver (`src/MW_Alvik.cpp`)
that wraps the Arduino_Alvik library. It uses the same approach as the HexaMotor
Uno R4 blocks: only documented interfaces, no patch of the support package.

## Setup

1. Install the **Simulink Support Package for Arduino Hardware** (R2026b), then run its
   hardware setup for the **Arduino Nano ESP32**. This installs the ESP32 core 3.2.1
   into the support package. Without it, the build fails with a missing
   `...\aCLI\data\packages\esp32\...\partitions\...csv` file.
2. Install the **Arduino_Alvik** library (which also installs **ucPack**) with the
   Arduino IDE Library Manager. The block finds it in `Documents\Arduino\libraries`.
   You can also copy both libraries into `third_party\` here, or point to another
   folder with `setpref('AlvikBlocks','LibraryRoot','<folder>')`. See `alvikLibraryRoot.m`.
3. Add this folder to the MATLAB path and run `createAlvikBlocksLibrary` once. The
   block then appears in the Library Browser under *Arduino Alvik*.
4. In the model: *Hardware board* = **Arduino Nano ESP32**.

## Block: Arduino Alvik

**Outputs**

| Port | Type | Meaning |
|---|---|---|
| `battery %` | single | Battery state of charge, 0–100 %. Read every *Battery sample time* and held in between. |
| `charging` | boolean | Optional. True while charging. |
| `status` | int8 | Optional. Startup result: 0 = OK, −2 = the STM32 firmware differs from the version the library requires (the robot still runs). |

**Inputs** (depend on *Motor command mode*)

| Mode | Inputs |
|---|---|
| Disabled | none |
| Wheel speeds (left, right) | left and right wheel speeds, in RPM, deg/s, rad/s or rev/s |
| Drive (linear, angular) | forward speed (cm/s, mm/s, m/s or in/s) and turn rate (deg/s, rad/s, RPM or rev/s) |

**Parameters**

| Parameter | Description |
|---|---|
| Sample time | Block rate (s). Motor commands are sent at this rate. |
| Battery sample time | How often the battery is read (s). Must be an integer multiple of the sample time. |
| Output charging status / connection status | Adds the optional outputs. |
| Motor command mode | Disabled, wheel speeds or drive. |
| Units | Units of the command inputs. |
| Limits | Each command is clamped to ±limit, in the selected units. Inf = no limit. |

## Notes and limitations

- **Hardware only.** In normal simulation the outputs are 0. The block works in
  generated code (deploy or External mode).
- **Startup blocks** while the Alvik power switch is off. It also blocks if the STM32
  doesn't answer, for example when the battery is flat.
- **Use one Alvik block per model.** The driver has one robot object.
- **Library output is off.** The library runs with `verbose = false`, so it doesn't
  print to the USB serial port that External mode uses.
- **Firmware mismatch.** With `status` = −2 (for example, an Alvik whose STM32 still
  has its factory firmware), the battery and motor commands still work through the
  library's background reader. Update the STM32 firmware to the version the library
  requires for full support.

## Status (2026-11-18)

| Check | Result |
|---|---|
| Library build, port and mask behaviour in all three modes, sample-time validation | Pass |
| Normal simulation | Pass |
| Code generation for *Arduino Nano ESP32* | Pass. The generated code calls the driver with the right unit codes. |
| Compile and hardware test | **Not done yet.** The ESP32 core isn't installed in the support package (see Setup 1). |

| File | Purpose |
|---|---|
| `AlvikRobot.m` | System object: parameters, ports, battery decimation, build info. |
| `src/MW_Alvik.cpp`, `include/MW_Alvik.h` | C wrapper around Arduino_Alvik. |
| `alvikLibraryRoot.m` | Finds the Arduino_Alvik and ucPack sources. |
| `createAlvikBlocksLibrary.m`, `slblocks.m` | Builds `AlvikBlocks.slx` and registers it in the Library Browser. |
