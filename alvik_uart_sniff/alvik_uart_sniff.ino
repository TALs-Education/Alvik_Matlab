// Alvik UART sniff test
// Board: Arduino Nano ESP32 (arduino:esp32:nano_nora) mounted on Arduino Alvik
//
// Diagnoses the ESP32 <-> STM32 carrier UART link (Serial0):
//  Test A: normal boot (BOOT0 LOW) - reset the STM32 and dump any bytes it sends,
//          at several baud rates (the Arduino_Alvik library uses 460800).
//  Test B: ROM bootloader (BOOT0 HIGH) - send 0x7F at 115200 8E1; a healthy STM32
//          always answers 0x79 (ACK). This proves the UART wiring/link works
//          independently of the application firmware.
// Pins are the same as the Arduino_Alvik library (definitions.h). Runs once.

#define CHECK_STM32 A6
#define BOOT_STM32  D2
#define RESET_STM32 D3
#define NANO_CHK    D4

void resetStm32() {
  digitalWrite(RESET_STM32, LOW);
  delay(100);
  digitalWrite(RESET_STM32, HIGH);
}

size_t dump(unsigned long ms) {
  unsigned long start = millis();
  size_t count = 0;
  while (millis() - start < ms) {
    while (Serial0.available()) {
      uint8_t b = Serial0.read();
      if (count < 64) {               // print the first 64 bytes only
        if (b < 0x10) Serial.print('0');
        Serial.print(b, HEX);
        Serial.print(' ');
      }
      count++;
    }
    delay(1);
  }
  return count;
}

void testNormalBoot(unsigned long baud) {
  Serial0.end();
  Serial0.begin(baud);
  delay(20);
  while (Serial0.available()) Serial0.read();
  digitalWrite(BOOT_STM32, LOW);
  Serial.print("[A] normal boot @ ");
  Serial.print(baud);
  Serial.print(" : ");
  resetStm32();
  size_t n = dump(2500);
  Serial.print(" -> ");
  Serial.print(n);
  Serial.println(" bytes");
}

void testBootloader() {
  Serial0.end();
  Serial0.begin(115200, SERIAL_8E1);
  delay(20);
  digitalWrite(BOOT_STM32, HIGH);
  delay(50);
  resetStm32();
  delay(200);
  while (Serial0.available()) Serial0.read();
  Serial.print("[B] ROM bootloader: sent 0x7F, reply: ");
  Serial0.write(0x7F);
  size_t n = dump(500);
  Serial.print(" -> ");
  Serial.print(n);
  Serial.println(" bytes (expect 79 = ACK)");
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 5000) { }
  delay(500);

  pinMode(CHECK_STM32, INPUT_PULLDOWN);
  pinMode(BOOT_STM32, OUTPUT);
  pinMode(RESET_STM32, OUTPUT);
  pinMode(NANO_CHK, OUTPUT);
  digitalWrite(NANO_CHK, LOW);
  digitalWrite(BOOT_STM32, LOW);
  digitalWrite(RESET_STM32, HIGH);

  Serial.println("Alvik UART sniff test");
  Serial.print("Alvik power: ");
  Serial.println(digitalRead(CHECK_STM32) ? "ON" : "OFF");

  const unsigned long bauds[] = {460800, 115200, 230400, 921600};
  for (unsigned long b : bauds) testNormalBoot(b);

  testBootloader();

  // leave the STM32 running its normal firmware
  digitalWrite(BOOT_STM32, LOW);
  resetStm32();
  Serial.println("Done.");
}

void loop() { }
