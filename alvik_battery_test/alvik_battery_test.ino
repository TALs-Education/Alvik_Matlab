// Alvik battery test
// Board: Arduino Nano ESP32 (arduino:esp32:nano_nora) mounted on Arduino Alvik
// Library: Arduino_Alvik
//
// Prints the battery state of charge (%) and charging status once per second.
// Note: the Alvik power switch must be ON; begin() waits in idle mode until it is.

#include "Arduino_Alvik.h"

Arduino_Alvik alvik;

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) { }   // wait briefly for USB serial

  Serial.println("Alvik battery test: starting (make sure the Alvik switch is ON)...");

  // begin() blocks silently while the Alvik is switched off, so report the state first
  pinMode(CHECK_STM32, INPUT_PULLDOWN);
  while (!alvik.is_on()) {
    Serial.println("Alvik is OFF - waiting for the power switch...");
    delay(1000);
  }
  Serial.println("Alvik is ON - connecting to STM32...");

  int rc = alvik.begin();

  uint8_t up, mid, low, rup, rmid, rlow;
  alvik.get_version(up, mid, low, "fw");
  alvik.get_required_fw_version(rup, rmid, rlow);
  Serial.print("STM32 firmware version: ");
  Serial.print(up); Serial.print('.'); Serial.print(mid); Serial.print('.'); Serial.print(low);
  Serial.print("  (library requires ");
  Serial.print(rup); Serial.print('.'); Serial.print(rmid); Serial.print('.'); Serial.print(rlow);
  Serial.println(")");

  if (rc == -2) {
    // Version mismatch: the library's background reader is still running, and the
    // battery message is simple, so keep going but flag that readings are unofficial.
    Serial.println("WARNING: firmware mismatch - continuing anyway; update the STM32 firmware for full support.");
  } else if (rc != 0) {
    Serial.print("alvik.begin() failed, code ");
    Serial.println(rc);
    while (true) { delay(1000); }
  }
}

void loop() {
  int charge = alvik.get_battery_charge();        // state of charge, %
  bool charging = alvik.is_battery_charging();

  Serial.print("Battery: ");
  Serial.print(charge);
  Serial.print(" %  | charging: ");
  Serial.println(charging ? "yes" : "no");

  delay(1000);
}
