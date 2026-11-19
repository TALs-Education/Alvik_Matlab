/* Arduino Alvik driver for Simulink (Arduino Nano ESP32 mounted on the Alvik).
 * Thin C wrapper around the Arduino_Alvik library (one robot per model). */
#ifndef MW_ALVIK_H
#define MW_ALVIK_H

#include "rtwtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) || defined(RSIM_WITH_SL_SOLVER) )

/* Starts the link to the STM32 carrier (Arduino_Alvik::begin).
 * Safe to call more than once: only the first call starts the robot.
 * Blocks while the Alvik power switch is off.
 * Returns 0 = OK, -2 = STM32 firmware differs from the version the library
 * requires (the robot still runs; the library's reader keeps working). */
int8_T MW_Alvik_Setup(void);

/* Status returned by MW_Alvik_Setup (0 until setup has run). */
int8_T MW_Alvik_Status(void);

/* Battery state of charge in percent (0-100). */
real32_T MW_Alvik_BatteryCharge(void);

/* true while the battery is charging. */
boolean_T MW_Alvik_BatteryCharging(void);

/* Wheel speeds. unit: 0 = RPM, 1 = deg/s, 2 = rad/s, 3 = rev/s. */
void MW_Alvik_SetWheelSpeeds(real32_T left, real32_T right, uint8_T unit);

/* Body speeds. linearUnit: 0 = cm/s, 1 = mm/s, 2 = m/s, 3 = in/s;
 * angularUnit: 0 = RPM, 1 = deg/s, 2 = rad/s, 3 = rev/s. */
void MW_Alvik_Drive(real32_T linear, real32_T angular, uint8_T linearUnit, uint8_T angularUnit);

/* Stops both wheels. */
void MW_Alvik_Brake(void);

#else
#define MW_Alvik_Setup() ((int8_T)0)
#define MW_Alvik_Status() ((int8_T)0)
#define MW_Alvik_BatteryCharge() (0.0F)
#define MW_Alvik_BatteryCharging() (false)
#define MW_Alvik_SetWheelSpeeds(l, r, u) ((void)0)
#define MW_Alvik_Drive(v, w, lu, au) ((void)0)
#define MW_Alvik_Brake() ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* MW_ALVIK_H */
