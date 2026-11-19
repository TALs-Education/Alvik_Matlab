/* Arduino Alvik driver for Simulink (Arduino Nano ESP32 mounted on the Alvik).
 *
 * Wraps one global Arduino_Alvik object. begin() starts the library's own
 * FreeRTOS task, which reads the STM32 carrier's UART packets (battery,
 * sensors, ...) in the background, so the get functions here only return the
 * latest cached values and are cheap to call from the model step.
 *
 * begin() is called with verbose = false: the library would otherwise print
 * to Serial (USB), which Simulink uses for External mode.
 */
#include "MW_Alvik.h"

#if !( defined(MATLAB_MEX_FILE) || defined(RSIM_PARAMETER_LOADING) || defined(RSIM_WITH_SL_SOLVER) )
#include "Arduino.h"
#include "Arduino_Alvik.h"

#if !defined(ARDUINO_ARCH_ESP32)
#error "MW_Alvik supports only the Arduino Nano ESP32 mounted on the Arduino Alvik."
#endif

static Arduino_Alvik mwAlvik;
static bool   mwAlvikStarted = false;
static int8_T mwAlvikStatus  = 0;

extern "C" int8_T MW_Alvik_Setup(void)
{
    if (!mwAlvikStarted) {
        mwAlvikStarted = true;
        mwAlvikStatus = (int8_T)mwAlvik.begin(false);
        if (mwAlvikStatus == -2) {
            /* begin() returns before these when the firmware check fails */
            mwAlvik.set_illuminator(true);
            mwAlvik.set_behaviour(BEHAVIOUR_BATTERY_ALERT);
        }
    }
    return mwAlvikStatus;
}

extern "C" int8_T MW_Alvik_Status(void)
{
    return mwAlvikStatus;
}

extern "C" real32_T MW_Alvik_BatteryCharge(void)
{
    return mwAlvikStarted ? (real32_T)mwAlvik.get_battery_charge() : 0.0F;
}

extern "C" boolean_T MW_Alvik_BatteryCharging(void)
{
    return mwAlvikStarted ? (boolean_T)mwAlvik.is_battery_charging() : false;
}

extern "C" void MW_Alvik_SetWheelSpeeds(real32_T left, real32_T right, uint8_T unit)
{
    if (mwAlvikStarted) {
        mwAlvik.set_wheels_speed(left, right, unit);
    }
}

extern "C" void MW_Alvik_Drive(real32_T linear, real32_T angular, uint8_T linearUnit, uint8_T angularUnit)
{
    if (mwAlvikStarted) {
        mwAlvik.drive(linear, angular, linearUnit, angularUnit);
    }
}

extern "C" void MW_Alvik_Brake(void)
{
    if (mwAlvikStarted) {
        mwAlvik.brake();
    }
}

#endif
