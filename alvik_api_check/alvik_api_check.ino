// Alvik full API check
// Board: Arduino Nano ESP32 (arduino:esp32:nano_nora) mounted on Arduino Alvik
// Library: Arduino_Alvik
//
// Serial Monitor @ 115200. Runs through:
//   1. Connection + versions (STM32 firmware / library / required firmware)
//   2. Battery (charge %, charging)
//   3. Sensors snapshot: line, color, ToF distance, IMU, tilt/lift/shake, touch
//   4. LEDs (left/right RGB, builtin LED, illuminator)
//   5. Motors - ONLY after you press the OK (check) touch button:
//        wheel speed, wheel position, rotate, move, drive, pose
//      Put the robot on the floor, or on a stand with the wheels off the table.
// Then loops a live dashboard every second.
// Touch buttons in the loop: OK = rerun motor test, CANCEL = stop motors.

#include "Arduino_Alvik.h"

Arduino_Alvik alvik;

// ---------- helpers ----------
void header(const char *title) {
  Serial.println();
  Serial.print("=== ");
  Serial.print(title);
  Serial.println(" ===");
}

void printVer(uint8_t a, uint8_t b, uint8_t c) {
  Serial.print(a); Serial.print('.'); Serial.print(b); Serial.print('.'); Serial.print(c);
}

// wait for a non-blocking move/rotate to finish, with a timeout so a firmware
// mismatch can never hang the test
bool waitTarget(unsigned long timeout_ms) {
  delay(200);
  unsigned long t0 = millis();
  while (!alvik.is_target_reached()) {
    if (millis() - t0 > timeout_ms) {
      Serial.println("    (timeout waiting for target)");
      return false;
    }
    delay(20);
  }
  return true;
}

void printPose(const char *label) {
  float x, y, th;
  alvik.get_pose(x, y, th);
  Serial.print("    pose ");
  Serial.print(label);
  Serial.print(": x="); Serial.print(x, 1);
  Serial.print(" cm  y="); Serial.print(y, 1);
  Serial.print(" cm  theta="); Serial.print(th, 1);
  Serial.println(" deg");
}

// ---------- test sections ----------
void testBattery() {
  header("BATTERY");
  Serial.print("charge: ");
  Serial.print(alvik.get_battery_charge());
  Serial.print(" %   charging: ");
  Serial.println(alvik.is_battery_charging() ? "yes" : "no");
}

void testSensors() {
  header("SENSORS");

  int l, c, r;
  alvik.get_line_sensors(l, c, r);
  Serial.print("line      L/C/R: ");
  Serial.print(l); Serial.print(" / "); Serial.print(c); Serial.print(" / "); Serial.println(r);

  int cr, cg, cb;
  alvik.get_color_raw(cr, cg, cb);
  float h, s, v;
  alvik.get_color(h, s, v, HSV);
  Serial.print("color     raw R/G/B: ");
  Serial.print(cr); Serial.print(" / "); Serial.print(cg); Serial.print(" / "); Serial.print(cb);
  Serial.print("   HSV: ");
  Serial.print(h, 1); Serial.print(", "); Serial.print(s, 2); Serial.print(", "); Serial.print(v, 2);
  Serial.print("   label: ");
  Serial.println(alvik.get_color_label());

  float dl, dcl, dc, dcr, dr;
  alvik.get_distance(dl, dcl, dc, dcr, dr);
  Serial.print("distance  L/CL/C/CR/R [cm]: ");
  Serial.print(dl, 1); Serial.print(" / "); Serial.print(dcl, 1); Serial.print(" / ");
  Serial.print(dc, 1); Serial.print(" / "); Serial.print(dcr, 1); Serial.print(" / ");
  Serial.println(dr, 1);
  Serial.print("distance  top / bottom [cm]: ");
  Serial.print(alvik.get_distance_top(), 1); Serial.print(" / ");
  Serial.println(alvik.get_distance_bottom(), 1);

  float roll, pitch, yaw;
  alvik.get_orientation(roll, pitch, yaw);
  Serial.print("orient    roll/pitch/yaw [deg]: ");
  Serial.print(roll, 1); Serial.print(" / "); Serial.print(pitch, 1); Serial.print(" / "); Serial.println(yaw, 1);

  float ax, ay, az, gx, gy, gz;
  alvik.get_imu(ax, ay, az, gx, gy, gz);
  Serial.print("accel     x/y/z [g]: ");
  Serial.print(ax, 2); Serial.print(" / "); Serial.print(ay, 2); Serial.print(" / "); Serial.println(az, 2);
  Serial.print("gyro      x/y/z [deg/s]: ");
  Serial.print(gx, 1); Serial.print(" / "); Serial.print(gy, 1); Serial.print(" / "); Serial.println(gz, 1);

  Serial.print("tilt: ");
  Serial.print(alvik.get_tilt());
  Serial.print("   lifted: ");
  Serial.print(alvik.get_lifted() ? "yes" : "no");
  Serial.print("   shake: ");
  Serial.println(alvik.get_shake() ? "yes" : "no");

  Serial.print("touch     any:"); Serial.print(alvik.get_touch_any());
  Serial.print(" ok:");     Serial.print(alvik.get_touch_ok());
  Serial.print(" cancel:"); Serial.print(alvik.get_touch_cancel());
  Serial.print(" center:"); Serial.print(alvik.get_touch_center());
  Serial.print(" up:");     Serial.print(alvik.get_touch_up());
  Serial.print(" down:");   Serial.print(alvik.get_touch_down());
  Serial.print(" left:");   Serial.print(alvik.get_touch_left());
  Serial.print(" right:");  Serial.println(alvik.get_touch_right());
}

void testLeds() {
  header("LEDS");
  const char *names[] = {"red", "green", "blue", "white", "off"};
  const bool cols[][3] = {{1,0,0}, {0,1,0}, {0,0,1}, {1,1,1}, {0,0,0}};
  for (int i = 0; i < 5; i++) {
    Serial.print("left/right RGB LEDs: ");
    Serial.println(names[i]);
    alvik.left_led.set_color(cols[i][0], cols[i][1], cols[i][2]);
    alvik.right_led.set_color(cols[i][0], cols[i][1], cols[i][2]);
    delay(500);
  }
  Serial.println("builtin LED blink");
  for (int i = 0; i < 3; i++) {
    alvik.set_builtin_led(true);  delay(200);
    alvik.set_builtin_led(false); delay(200);
  }
  Serial.println("illuminator off/on");
  alvik.set_illuminator(false); delay(500);
  alvik.set_illuminator(true);
}

void testMotors() {
  header("MOTORS");

  // 1. wheel speed
  Serial.println("1) wheels forward 30 RPM for 1.5 s");
  alvik.set_wheels_speed(30, 30);
  delay(800);
  float wl, wr;
  alvik.get_wheels_speed(wl, wr);
  Serial.print("    measured L/R [RPM]: "); Serial.print(wl, 1); Serial.print(" / "); Serial.println(wr, 1);
  delay(700);
  alvik.brake();
  delay(500);

  Serial.println("2) wheels backward 30 RPM for 1.5 s");
  alvik.set_wheels_speed(-30, -30);
  delay(1500);
  alvik.brake();
  delay(500);

  // 2. single wheels
  Serial.println("3) left wheel only, then right wheel only (1 s each)");
  alvik.left_wheel.set_speed(30);  delay(1000); alvik.left_wheel.set_speed(0);
  alvik.right_wheel.set_speed(30); delay(1000); alvik.right_wheel.set_speed(0);
  delay(500);

  // 3. wheel position
  float pl, pr;
  alvik.get_wheels_position(pl, pr);
  Serial.print("4) wheel position before L/R [deg]: "); Serial.print(pl, 1); Serial.print(" / "); Serial.println(pr, 1);
  Serial.println("   set both wheels +180 deg");
  alvik.set_wheels_position(pl + 180, pr + 180, DEG, false);
  delay(2000);
  alvik.get_wheels_position(pl, pr);
  Serial.print("   wheel position after  L/R [deg]: "); Serial.print(pl, 1); Serial.print(" / "); Serial.println(pr, 1);

  // 4. pose-based motion
  alvik.reset_pose();
  printPose("reset");

  Serial.println("5) rotate +90 deg");
  alvik.rotate(90, DEG, false);  waitTarget(5000);  printPose("after +90");
  Serial.println("   rotate -90 deg");
  alvik.rotate(-90, DEG, false); waitTarget(5000);  printPose("after -90");

  Serial.println("6) move +10 cm");
  alvik.move(10, CM, false);     waitTarget(5000);  printPose("after +10cm");
  Serial.println("   move -10 cm");
  alvik.move(-10, CM, false);    waitTarget(5000);  printPose("after -10cm");

  // 5. drive (linear + angular)
  Serial.println("7) drive 5 cm/s + 30 deg/s for 2 s");
  alvik.drive(5, 30);
  delay(1000);
  float lin, ang;
  alvik.get_drive_speed(lin, ang);
  Serial.print("    measured linear/angular: "); Serial.print(lin, 1); Serial.print(" cm/s / ");
  Serial.print(ang, 1); Serial.println(" deg/s");
  delay(1000);
  alvik.brake();
  printPose("final");

  Serial.println("motor test done");
}

void waitForOkThenRunMotors() {
  header("MOTOR TEST READY");
  Serial.println("Place the robot safely, then press the OK (check) touch button to start.");
  Serial.println("Press CANCEL (x) to skip.");
  alvik.left_led.set_color(0, 0, 1);
  alvik.right_led.set_color(0, 0, 1);
  while (true) {
    if (alvik.get_touch_ok())     { delay(1000); testMotors(); break; }
    if (alvik.get_touch_cancel()) { Serial.println("motor test skipped"); break; }
    delay(50);
  }
  alvik.left_led.set_color(0, 0, 0);
  alvik.right_led.set_color(0, 0, 0);
}

// ---------- main ----------
void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) { }

  header("ALVIK API CHECK");
  pinMode(CHECK_STM32, INPUT_PULLDOWN);
  while (!alvik.is_on()) {
    Serial.println("Alvik is OFF - waiting for the power switch...");
    delay(1000);
  }
  Serial.println("Alvik is ON - connecting to STM32...");

  int rc = alvik.begin();

  header("VERSIONS");
  uint8_t a, b, c;
  alvik.get_fw_version(a, b, c);          Serial.print("STM32 firmware:    "); printVer(a, b, c); Serial.println();
  alvik.get_lib_version(a, b, c);         Serial.print("library:           "); printVer(a, b, c); Serial.println();
  alvik.get_required_fw_version(a, b, c); Serial.print("required firmware: "); printVer(a, b, c); Serial.println();
  Serial.print("begin() code: "); Serial.println(rc);

  if (rc == -2) {
    Serial.println("WARNING: firmware mismatch - continuing anyway; some features may not work.");
    // begin() returns before these when the firmware check fails
    alvik.set_illuminator(true);
    alvik.set_behaviour(BEHAVIOUR_BATTERY_ALERT);
  } else if (rc != 0) {
    Serial.println("begin() failed - stopping.");
    while (true) delay(1000);
  }

  delay(500);   // let sensor data arrive
  testBattery();
  testSensors();
  testLeds();
  waitForOkThenRunMotors();

  header("LIVE DASHBOARD (OK = rerun motor test, CANCEL = stop motors)");
}

void loop() {
  if (alvik.get_touch_ok())     { waitForOkThenRunMotors(); return; }
  if (alvik.get_touch_cancel()) { alvik.brake(); Serial.println("motors stopped"); }

  static unsigned long last = 0;
  if (millis() - last < 1000) { delay(20); return; }
  last = millis();

  int l, c, r;
  alvik.get_line_sensors(l, c, r);
  float dl, dcl, dc, dcr, dr;
  alvik.get_distance(dl, dcl, dc, dcr, dr);
  float roll, pitch, yaw;
  alvik.get_orientation(roll, pitch, yaw);

  Serial.print("bat "); Serial.print(alvik.get_battery_charge()); Serial.print("%");
  Serial.print(alvik.is_battery_charging() ? "(chg)" : "");
  Serial.print(" | line "); Serial.print(l); Serial.print('/'); Serial.print(c); Serial.print('/'); Serial.print(r);
  Serial.print(" | dist C "); Serial.print(dc, 1); Serial.print("cm");
  Serial.print(" | color "); Serial.print(alvik.get_color_label());
  Serial.print(" | rpy "); Serial.print(roll, 0); Serial.print('/'); Serial.print(pitch, 0); Serial.print('/'); Serial.print(yaw, 0);
  Serial.print(" | tilt "); Serial.println(alvik.get_tilt());
}
