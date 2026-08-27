// ============================================================
// BEDBOX IRA - TEST 001  (motor bring-up)
// ============================================================
//
// Board: ESP32-S3-DevKitC-1 N16R8
// Purpose: prove the solder work on the L298N controllers.
//
// Ramps ONE motor at a time from 0 to 100 percent over about
// 2.5 seconds, holds briefly, ramps back down, then moves to the
// next. Never two motors at once - a short in one controller shows
// up as that controller and nothing else.
//
// WIRING THIS ASSUMES
//   Controller 1  ENA = 4    ENB = 5
//   Controller 2  ENA = 6    ENB = 7
//   Controller 3  ENA = 15   ENB = 21
//   Controller 4  ENA = 38   ENB = 38    (both leg motors, one signal)
//   IN1/IN3 -> GND, IN2/IN4 -> the module's own 5V pin
//   ENA/ENB jumpers REMOVED, regulator jumper LEFT ON
//   GND common between every module and the ESP32
//
// If controller 4 is not built yet, step 7 simply does nothing.
//
// NOT YET RUN ON HARDWARE.
//
// ============================================================

#include <Arduino.h>

#define TEST_NUMBER 1

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------

const int PWM_FREQ = 20000;   // 20 kHz - above hearing, no whine
const int PWM_RES  = 8;       // 8 bit -> duty 0..255

const int RAMP_MS  = 2500;    // 0 to 100 percent takes this long
const int HOLD_MS  = 800;     // time held at full before ramping down
const int GAP_MS   = 700;     // silence between motors

struct Motor {
  int         pin;
  const char *label;
};

const Motor MOTOR[] = {
  {  4, "controller 1  motor 1" },
  {  5, "controller 1  motor 2" },
  {  6, "controller 2  motor 3" },
  {  7, "controller 2  motor 4" },
  { 15, "controller 3  motor 5" },
  { 21, "controller 3  motor 6" },
  { 38, "controller 4  motors 7+8" },
};

const int MOTOR_COUNT = sizeof(MOTOR) / sizeof(MOTOR[0]);

bool gRunning = true;         // sequence runs on boot

// ------------------------------------------------------------

void allOff() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    ledcWrite(MOTOR[i].pin, 0);
  }
}

bool interrupted() {
  return Serial.available() > 0;
}

// ------------------------------------------------------------
// Ramp one motor 0 -> 100 -> 0
// Returns false if the user interrupted.
// ------------------------------------------------------------

bool runMotor(int index) {
  const Motor &m = MOTOR[index];

  allOff();
  delay(GAP_MS);

  Serial.println();
  Serial.printf("--- %s   (GPIO %d) ---\n", m.label, m.pin);

  const int stepWait = RAMP_MS / 100;

  for (int pct = 0; pct <= 100; pct++) {
    if (interrupted()) { allOff(); return false; }
    int duty = (pct * 255) / 100;
    ledcWrite(m.pin, duty);
    if (pct % 10 == 0) {
      Serial.printf("    up   %3d %%   duty %3d\n", pct, duty);
    }
    delay(stepWait);
  }

  Serial.printf("    hold 100 %%  for %d ms\n", HOLD_MS);
  for (int t = 0; t < HOLD_MS; t += 50) {
    if (interrupted()) { allOff(); return false; }
    delay(50);
  }

  for (int pct = 100; pct >= 0; pct--) {
    if (interrupted()) { allOff(); return false; }
    int duty = (pct * 255) / 100;
    ledcWrite(m.pin, duty);
    if (pct % 20 == 0) {
      Serial.printf("    down %3d %%   duty %3d\n", pct, duty);
    }
    delay(stepWait / 2);
  }

  ledcWrite(m.pin, 0);
  Serial.printf("    %s done\n", m.label);
  return true;
}

// ------------------------------------------------------------

void printMenu() {
  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" BEDBOX IRA - TEST 001 - motor bring-up"));
  Serial.println(F("=================================================="));
  Serial.println(F(" Motors:"));
  for (int i = 0; i < MOTOR_COUNT; i++) {
    Serial.printf("   %d  GPIO %-3d  %s\n", i + 1, MOTOR[i].pin, MOTOR[i].label);
  }
  Serial.println();
  Serial.println(F(" Commands:"));
  Serial.println(F("   1..7   ramp that motor once"));
  Serial.println(F("   a      run the whole sequence, looping"));
  Serial.println(F("   s      stop everything"));
  Serial.println(F("   ?      this menu"));
  Serial.println();
  Serial.println(F(" Any key during a ramp aborts and stops the motor."));
  Serial.println(F("=================================================="));
}

// ------------------------------------------------------------

void handleLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  char c = line.charAt(0);

  if (c >= '1' && c <= '9') {
    int idx = line.toInt();
    if (idx < 1 || idx > MOTOR_COUNT) {
      Serial.printf("  no motor %d\n", idx);
      return;
    }
    gRunning = false;
    runMotor(idx - 1);
    allOff();
    Serial.println(F("  stopped. type a to resume the sequence."));
    return;
  }

  switch (c) {
    case 'a':
      gRunning = true;
      Serial.println(F("  sequence running."));
      break;

    case 's':
      gRunning = false;
      allOff();
      Serial.println(F("  all motors off."));
      break;

    case '?':
      printMenu();
      break;

    default:
      Serial.println(F("  unknown command. type ? for the menu."));
      break;
  }
}

// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(400);

  Serial.println();
  Serial.println(F("=== BEDBOX IRA - TEST 001 - motor bring-up ==="));

  // GPIO 38 appears once in the table but carries ENA and ENB of
  // controller 4 on the same wire, so one channel covers both motors.
  for (int i = 0; i < MOTOR_COUNT; i++) {
    ledcAttach(MOTOR[i].pin, PWM_FREQ, PWM_RES);
    ledcWrite(MOTOR[i].pin, 0);
  }

  Serial.printf("%d PWM channels attached at %d Hz, %d bit\n",
                MOTOR_COUNT, PWM_FREQ, PWM_RES);

  printMenu();
}

void loop() {
  static String line = "";

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length() > 0) {
        handleLine(line);
        line = "";
      }
    } else if (line.length() < 32) {
      line += c;
    }
  }

  if (gRunning) {
    for (int i = 0; i < MOTOR_COUNT; i++) {
      if (!gRunning) break;
      if (!runMotor(i)) {
        gRunning = false;
        allOff();
        Serial.println(F("  aborted. type a to resume, ? for the menu."));
        break;
      }
    }
    if (gRunning) {
      Serial.println();
      Serial.println(F("=== sequence complete, looping ==="));
      delay(1500);
    }
  }
}

// ============================================================
// END OF FILE - BEDBOX IRA - TEST 001 (motor bring-up)
// ============================================================
