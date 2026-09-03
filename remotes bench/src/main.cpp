// ============================================================
// BEDBOX IRA - TEST 002  (L298N static test)
// ============================================================
//
// Board: ESP32-S3-DevKitC-1 N16R8
//
// WHY THIS EXISTS AND WHAT IT CANNOT DO
//   An L298N gives the microcontroller no feedback at all. The ESP32
//   pushes PWM out and never learns what happened. So software alone
//   CANNOT tell you whether a controller is damaged.
//
//   The multimeter tells you. This sketch's only job is to hold one
//   channel at one duty, indefinitely, so there is something steady
//   to measure. TEST 001 ramps and moves on, which is far too fast.
//
// WHAT IS ACTUALLY AT RISK
//   VS tolerates up to 46 V, so 10 V there harms nothing.
//   The logic side - IN1..IN4, ENA, ENB and the module's 5 V pin -
//   is limited to about 7 V. That is what an over-volt event kills.
//
// MEASURE WITH THE MOTORS DISCONNECTED FIRST
//   No load makes the reading cleaner and nothing can run away.
//   Reconnect them only once the voltages look right.
//
// WHAT TO MEASURE AND WHAT IS NORMAL
//   1. The module's own 5 V pin, against GND.
//        ~5 V  -> the onboard regulator survived
//        0 V or VS -> that regulator is dead, replace the module
//
//   2. Between OUT1 and OUT2 of the channel under test, DC volts.
//      A cheap meter averages the PWM, which is exactly what we want.
//        duty 100 -> roughly VS minus 1.5 to 2.5 V.
//                    On 12 V that is about 9.5 to 10.5 V.
//        duty  50 -> roughly half of that
//        duty   0 -> 0 V
//
//   3. Faults and what they mean
//        always full regardless of duty -> ENA/ENB jumper still on,
//              or that enable input is damaged
//        always 0 V -> that half bridge is dead
//        reads VS with no drop at all -> output transistor shorted
//        one channel fine, the other dead -> classic L298N failure,
//              the two halves fail independently
//
// GPIO 38 DRIVES TWO MOTORS
//   Channel 7 is ENA and ENB of controller 4 wired together, so both
//   leg motors move as one. Measure both outputs of that module.
//
// NOT YET RUN ON HARDWARE.
//
// ============================================================

#include <Arduino.h>

#define TEST_NUMBER 2

// ------------------------------------------------------------
// Configuration - bedbox_ira. For bedbox_shemi use
// {13,14,18,19,21,22} and the classic-ESP32 env.
// ------------------------------------------------------------

const int PWM_FREQ = 20000;   // 20 kHz, above hearing
const int PWM_RES  = 8;       // duty 0..255

struct Chan { int pin; const char *label; };

const Chan MOTOR[] = {
  {  4, "ctrl 1 ENA  motor 1" },
  {  5, "ctrl 1 ENB  motor 2" },
  {  6, "ctrl 2 ENA  motor 3" },
  {  7, "ctrl 2 ENB  motor 4" },
  { 15, "ctrl 3 ENA  motor 5" },
  { 21, "ctrl 3 ENB  motor 6" },
  { 38, "ctrl 4 ENA+ENB  motors 7 and 8" },
};

const int CHANS = sizeof(MOTOR) / sizeof(MOTOR[0]);

int  selected = 1;    // 1..CHANS
int  duty     = 0;    // percent

// ------------------------------------------------------------

void allOff() {
  for (int i = 0; i < CHANS; i++) ledcWrite(MOTOR[i].pin, 0);
}

void applyOne(int idx, int pct) {
  allOff();
  if (pct <= 0) return;
  if (pct > 100) pct = 100;
  ledcWrite(MOTOR[idx - 1].pin, (pct * 255) / 100);
}

void showState() {
  Serial.println();
  Serial.printf("  HOLDING  channel %d  (GPIO %d)  %s\n",
                selected, MOTOR[selected - 1].pin, MOTOR[selected - 1].label);
  Serial.printf("  duty %d %%   (raw %d of 255)\n", duty, (duty * 255) / 100);
  Serial.println(F("  Measure OUT1 to OUT2 on that controller, DC volts."));
  Serial.println(F("  It stays here until the next command."));
  Serial.println();
}

// Walks 0, 25, 50, 75, 100 with four seconds on each, so the meter
// has time to settle and a linear response is visible.
void stepThrough(int idx) {
  const int steps[5] = {0, 25, 50, 75, 100};
  Serial.println();
  Serial.printf("--- stepping channel %d, 4 s per step ---\n", idx);
  Serial.println(F("    watch the meter. It should climb in even steps."));
  for (int i = 0; i < 5; i++) {
    applyOne(idx, steps[i]);
    Serial.printf("    duty %3d %%\n", steps[i]);
    unsigned long t = millis();
    while (millis() - t < 4000) {
      if (Serial.available()) {
        while (Serial.available()) Serial.read();
        allOff();
        duty = 0;
        Serial.println(F("    aborted, all off."));
        return;
      }
    }
  }
  allOff();
  duty = 0;
  Serial.println(F("--- done, all off ---"));
  Serial.println(F("    Not linear? The enable input is damaged."));
  Serial.println(F("    Flat at 0? That half bridge is dead."));
  Serial.println();
}

// ------------------------------------------------------------

void printMenu() {
  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" BEDBOX IRA - TEST 002 - L298N static test"));
  Serial.println(F("=================================================="));
  for (int i = 0; i < CHANS; i++)
    Serial.printf("   %d   GPIO %-3d  %s\n", i + 1, MOTOR[i].pin, MOTOR[i].label);
  Serial.println();
  Serial.println(F("   1..7    select a channel and hold it at the"));
  Serial.println(F("           current duty"));
  Serial.println(F("   d50     set duty percent, 0 to 100"));
  Serial.println(F("   s       step 0,25,50,75,100 on the selected"));
  Serial.println(F("           channel, 4 s each"));
  Serial.println(F("   0       everything off"));
  Serial.println(F("   ?       this menu"));
  Serial.println();
  Serial.println(F(" Motors DISCONNECTED for the first pass."));
  Serial.println(F(" On 12 V expect about 9.5 to 10.5 V at duty 100,"));
  Serial.println(F(" and 0 V at duty 0."));
  Serial.println(F("=================================================="));
}

void handleLine(String line) {
  line.trim();
  line.toLowerCase();
  if (!line.length()) return;

  if (line == "?") { printMenu(); return; }

  if (line == "0") {
    allOff();
    duty = 0;
    Serial.println(F("  all channels off."));
    return;
  }

  if (line == "s") { stepThrough(selected); return; }

  if (line.length() == 1 && line.charAt(0) >= '1' && line.charAt(0) <= '9') {
    int n = line.toInt();
    if (n < 1 || n > CHANS) { Serial.printf("  1 .. %d\n", CHANS); return; }
    selected = n;
    if (duty == 0) duty = 100;      // selecting a channel implies testing it
    applyOne(selected, duty);
    showState();
    return;
  }

  if (line.charAt(0) == 'd') {
    int v = line.substring(1).toInt();
    if (v < 0 || v > 100) { Serial.println(F("  duty 0..100")); return; }
    duty = v;
    applyOne(selected, duty);
    showState();
    return;
  }

  Serial.println(F("  1..7, d50, s, 0 or ?"));
}

// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(400);

  Serial.println();
  Serial.println(F("=== BEDBOX IRA - TEST 002 - L298N static test ==="));

  for (int i = 0; i < CHANS; i++) {
    ledcAttach(MOTOR[i].pin, PWM_FREQ, PWM_RES);
    ledcWrite(MOTOR[i].pin, 0);
  }
  Serial.printf("%d channels attached at %d Hz, %d bit. All off.\n",
                CHANS, PWM_FREQ, PWM_RES);

  printMenu();
}

void loop() {
  static String line = "";
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length()) { handleLine(line); line = ""; }
    } else if (line.length() < 32) {
      line += c;
    }
  }
}

// ============================================================
// END OF FILE - BEDBOX IRA - TEST 002 (L298N static test)
// ============================================================
