/* ============================================================
 *                        TEST  007
 * ============================================================
 *  BED BOX FIRMWARE  (QuinLED-ESP32, WROOM-32E)
 *  Adjustable Bed Massage Retrofit — Revision 4, 8 motors
 *  Stage 1: TWO-MOTOR AUTO DEMO + manual console
 * ------------------------------------------------------------
 *  CHANGE vs TEST 006: motor 0 / motor 1 sizes FLIPPED to match
 *  the actual bench wiring measured with 'calib':
 *      Channel 0 (OUT1/OUT2) = BIG   motor (cold-start ~165)
 *      Channel 1 (OUT3/OUT4) = small motor (cold-start ~55)
 *  So MOTOR_BIG[0]=1 (big), MOTOR_BIG[1]=0 (small).
 *  Running minimums (kick-start covers break-loose):
 *      MIN_DUTY_BIG   = 165
 *      MIN_DUTY_SMALL = 60
 * ------------------------------------------------------------
 *  Demo auto-starts and loops. Press ENTER (or type) to STOP;
 *  type  demo  to start it again.
 *
 *  BENCH WIRING (this build):
 *    BIG   motor -> OUT1/OUT2   (Motor 0, ENA=GPIO13, IN1=GND, IN2=+5V)
 *    small motor -> OUT3/OUT4   (Motor 1, ENB=GPIO14, IN3=GND, IN4=+5V)
 *    common GND between QuinLED and L298N. 12V to the L298N power in.
 * ------------------------------------------------------------
 *  PWM: Arduino-ESP32 core 2.x LEDC channel API.
 *  TEST_NUMBER 7 — printed at boot; later shown on the panel.
 *  Board: ESP32 Dev Module | Serial 115200 | monitor_speed=115200
 * ============================================================ */

#include <Arduino.h>
#include <Preferences.h>

#define TEST_NUMBER 7     // <==== THIS BUILD

// ---------- Pin map (Revision 4, 4 zones x 2 motors) ----------
const int NUM_MOTORS = 8;
const int MOTOR_PIN[NUM_MOTORS] = {13, 14, 18, 19, 21, 22, 23, 25};
const int MOTOR_ZONE[NUM_MOTORS] = {0, 0, 1, 1, 2, 2, 3, 3};
const char* ZONE_NAME[4] = {"Head", "Shoulders", "Back", "Legs"};

// Motor size per index: 0 = small (0.1A), 1 = big (0.3A)
// BENCH (this build): channel 0 = BIG, channel 1 = small (matches wiring).
const int MOTOR_BIG[NUM_MOTORS] = {1, 0, 1, 1, 1, 1, 0, 0};

// How many motors the auto-demo drives (the two on the one L298N).
const int DEMO_MOTORS = 2;
const char* BENCH_NAME[2] = {"Motor0 BIG (OUT1/2)", "Motor1 small (OUT3/4)"};

// ---------- PWM config (core 2.x channel API) ----------
const int PWM_FREQ = 20000;      // 20 kHz
const int PWM_RES  = 8;          // 0..255 duty

// ---------- Intensity mapping (CALIBRATED) ----------
// Running minimum duty; kick-start handles the initial break-loose.
int MIN_DUTY_SMALL = 60;         // measured cold-start ~55
int MIN_DUTY_BIG   = 165;        // measured cold-start ~165-180
const int KICK_MS  = 60;         // kick-start pulse length

// ---------- Protocol constants ----------
const uint8_t MY_BED_ID = 1;
enum Cmd { CMD_OFF = 0, CMD_ALL = 1, CMD_ZONE = 2,
           CMD_MOTOR = 3, CMD_PRESET = 4, CMD_TIMER = 5 };

// ---------- State ----------
int motorLevel[NUM_MOTORS];
unsigned long timerEndMs = 0;
Preferences prefs;

int activePreset = -1;
unsigned long presetStepMs = 0;
int presetPhase = 0;
int presetLevel = 60;

bool demoActive = true;          // auto-start the demo at boot

// ---------- Forward declarations ----------
int  levelToDuty(int motor, int level);
void pwmWrite(int m, int duty);
void setMotor(int m, int level);
void setZone(int z, int level);
void setAll(int level);
void allOff();
void handleMessage(uint8_t bedId, uint8_t cmd, uint8_t target, uint8_t value);
void saveState();
void loadState();
void runCalib(int m);
void printStatus();
void handleLine(String line);
void tickPreset();
void demoSet(int m, int level);
bool demoWait(unsigned long ms);
bool demoRamp(int m, int fromL, int toL, unsigned long ms);
void runDemoPass();
void stopDemoToManual();

// ============================================================
//  Low-level PWM
// ============================================================
void pwmWrite(int m, int duty) { ledcWrite(m, duty); }   // channel m

int levelToDuty(int motor, int level) {
  if (level <= 0) return 0;
  if (level > 100) level = 100;
  int minDuty = MOTOR_BIG[motor] ? MIN_DUTY_BIG : MIN_DUTY_SMALL;
  return minDuty + (255 - minDuty) * (level - 1) / 99;
}

void setMotor(int m, int level) {
  if (m < 0 || m >= NUM_MOTORS) return;
  bool wasOff = (motorLevel[m] == 0);
  motorLevel[m] = constrain(level, 0, 100);
  int duty = levelToDuty(m, motorLevel[m]);
  if (duty > 0 && wasOff && duty < 255) { pwmWrite(m, 255); delay(KICK_MS); }
  pwmWrite(m, duty);
}

void setZone(int z, int level) {
  for (int m = 0; m < NUM_MOTORS; m++) if (MOTOR_ZONE[m] == z) setMotor(m, level);
}
void setAll(int level) { for (int m = 0; m < NUM_MOTORS; m++) setMotor(m, level); }
void allOff() { activePreset = -1; timerEndMs = 0; setAll(0); }

// ============================================================
//  Protocol handler
// ============================================================
void handleMessage(uint8_t bedId, uint8_t cmd, uint8_t target, uint8_t value) {
  if (bedId != MY_BED_ID && bedId != 0) return;
  switch (cmd) {
    case CMD_OFF:    allOff();                                    break;
    case CMD_ALL:    activePreset = -1; setAll(value);            break;
    case CMD_ZONE:   activePreset = -1; setZone(target, value);   break;
    case CMD_MOTOR:  activePreset = -1; setMotor(target, value);  break;
    case CMD_PRESET:
      if (target == 0) { activePreset = 0; presetPhase = 0;
                         presetLevel = value ? value : 60; presetStepMs = 0; }
      break;
    case CMD_TIMER:  timerEndMs = millis() + (unsigned long)value * 60000UL; break;
  }
  saveState();
}

// ============================================================
//  Persistence
// ============================================================
void saveState() {
  prefs.begin("bedbox", false);
  prefs.putBytes("levels", motorLevel, sizeof(motorLevel));
  prefs.putInt("preset", activePreset);
  prefs.end();
}
void loadState() {
  prefs.begin("bedbox", true);
  if (prefs.isKey("levels")) prefs.getBytes("levels", motorLevel, sizeof(motorLevel));
  activePreset = prefs.getInt("preset", -1);
  prefs.end();
  for (int m = 0; m < NUM_MOTORS; m++) {
    int lvl = motorLevel[m]; motorLevel[m] = 0;
    if (lvl > 0) setMotor(m, lvl);
  }
}

// ============================================================
//  AUTO DEMO
// ============================================================
void demoSet(int m, int level) {          // smooth (no kick-start)
  motorLevel[m] = constrain(level, 0, 100);
  pwmWrite(m, levelToDuty(m, motorLevel[m]));
}

void stopDemoToManual() {
  demoActive = false;
  for (int m = 0; m < NUM_MOTORS; m++) { motorLevel[m] = 0; pwmWrite(m, 0); }
  Serial.println();
  Serial.println(">>> Demo stopped. MANUAL mode.");
  Serial.println("    Try: motor 0 50 | motor 1 50 | status | calib 0 | demo");
}

bool demoWait(unsigned long ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    if (Serial.available()) { while (Serial.available()) Serial.read(); stopDemoToManual(); return true; }
    delay(5);
  }
  return false;
}

bool demoRamp(int m, int fromL, int toL, unsigned long ms) {
  const int steps = 40;
  for (int i = 0; i <= steps; i++) {
    if (!demoActive) return true;
    int lvl = fromL + (toL - fromL) * i / steps;
    demoSet(m, lvl);
    if (demoWait(ms / steps)) return true;
  }
  return false;
}

void runDemoPass() {
  Serial.printf("\n[DEMO] 1/7  Solo ramp 0->100->0 : %s\n", BENCH_NAME[0]);
  if (demoRamp(0, 0, 100, 3000)) return;
  if (demoWait(700)) return;
  if (demoRamp(0, 100, 0, 2000)) return;
  demoSet(0, 0);
  if (demoWait(500)) return;

  Serial.printf("[DEMO] 2/7  Solo ramp 0->100->0 : %s\n", BENCH_NAME[1]);
  if (demoRamp(1, 0, 100, 3000)) return;
  if (demoWait(700)) return;
  if (demoRamp(1, 100, 0, 2000)) return;
  demoSet(1, 0);
  if (demoWait(500)) return;

  Serial.println("[DEMO] 3/7  Alternate: Motor0 1s ON, then Motor1 2s ON (x3)");
  for (int k = 0; k < 3; k++) {
    demoSet(0, 90); if (demoWait(1000)) return; demoSet(0, 0);
    if (demoWait(300)) return;
    demoSet(1, 90); if (demoWait(2000)) return; demoSet(1, 0);
    if (demoWait(300)) return;
  }

  Serial.println("[DEMO] 4/7  Both together: ramp up, hold, ramp down");
  for (int i = 0; i <= 40; i++) {
    if (!demoActive) return;
    int lvl = 100 * i / 40; demoSet(0, lvl); demoSet(1, lvl);
    if (demoWait(80)) return;
  }
  if (demoWait(1000)) return;
  for (int i = 40; i >= 0; i--) {
    if (!demoActive) return;
    int lvl = 100 * i / 40; demoSet(0, lvl); demoSet(1, lvl);
    if (demoWait(60)) return;
  }
  demoSet(0, 0); demoSet(1, 0);
  if (demoWait(500)) return;

  Serial.println("[DEMO] 5/7  Cross-fade: Motor0 down while Motor1 up, then reverse");
  for (int i = 0; i <= 50; i++) {
    if (!demoActive) return;
    demoSet(0, 100 - 2 * i); demoSet(1, 2 * i);
    if (demoWait(80)) return;
  }
  for (int i = 0; i <= 50; i++) {
    if (!demoActive) return;
    demoSet(0, 2 * i); demoSet(1, 100 - 2 * i);
    if (demoWait(80)) return;
  }
  demoSet(0, 0); demoSet(1, 0);
  if (demoWait(500)) return;

  Serial.println("[DEMO] 6/7  Random: motor, level and duration");
  for (int r = 0; r < 6; r++) {
    int m   = random(0, DEMO_MOTORS);
    int lvl = random(35, 101);
    int dur = random(500, 2500);
    Serial.printf("        %s -> %d%% for %d ms\n", BENCH_NAME[m], lvl, dur);
    demoSet(m, lvl);
    if (demoWait(dur)) return;
    demoSet(m, 0);
    if (demoWait(250)) return;
  }

  Serial.println("[DEMO] 7/7  Pulse both together (x5)");
  for (int k = 0; k < 5; k++) {
    demoSet(0, 100); demoSet(1, 100); if (demoWait(250)) return;
    demoSet(0, 0);   demoSet(1, 0);   if (demoWait(400)) return;
  }

  Serial.println("[DEMO] --- pass complete, looping ---");
  if (demoWait(1200)) return;
}

// ============================================================
//  Manual text console
// ============================================================
String inBuf;

void runCalib(int m) {
  if (m < 0 || m >= NUM_MOTORS) { Serial.println("bad motor #"); return; }
  Serial.printf("Calibrating motor %d (%s, %s). Ramping duty slowly.\n",
                m, ZONE_NAME[MOTOR_ZONE[m]], MOTOR_BIG[m] ? "BIG" : "small");
  Serial.println("Watch/touch the motor. Press ENTER the moment it starts!");
  while (Serial.available()) Serial.read();
  for (int duty = 30; duty <= 255; duty += 5) {
    pwmWrite(m, duty);
    Serial.printf("  duty = %d  (%.0f%%)\n", duty, duty * 100.0 / 255);
    unsigned long t0 = millis();
    while (millis() - t0 < 700) {
      if (Serial.available()) {
        while (Serial.available()) Serial.read();
        pwmWrite(m, 0);
        Serial.printf("\n>>> Start threshold: duty %d\n", duty);
        Serial.printf(">>> Set %s to about %d (threshold + margin).\n",
                      MOTOR_BIG[m] ? "MIN_DUTY_BIG" : "MIN_DUTY_SMALL", duty + 8);
        return;
      }
      delay(10);
    }
  }
  pwmWrite(m, 0);
  Serial.println("Ramp finished without a keypress — motor never started?");
}

void printStatus() {
  Serial.println("---- STATUS ----");
  for (int m = 0; m < NUM_MOTORS; m++)
    Serial.printf(" motor %d  %-9s %-5s  level %3d%%  duty %3d\n",
                  m, ZONE_NAME[MOTOR_ZONE[m]], MOTOR_BIG[m] ? "BIG" : "small",
                  motorLevel[m], levelToDuty(m, motorLevel[m]));
  Serial.printf(" thresholds: small MIN_DUTY=%d, big MIN_DUTY=%d\n", MIN_DUTY_SMALL, MIN_DUTY_BIG);
  Serial.printf(" demo: %s\n", demoActive ? "RUNNING" : "stopped");
  Serial.println("----------------");
}

void handleLine(String line) {
  line.trim(); line.toLowerCase();
  if (line.length() == 0) return;
  int sp1 = line.indexOf(' ');
  String cmd = sp1 < 0 ? line : line.substring(0, sp1);
  String rest = sp1 < 0 ? "" : line.substring(sp1 + 1);
  int a = rest.toInt();
  int sp2 = rest.indexOf(' ');
  int b = sp2 < 0 ? -1 : rest.substring(sp2 + 1).toInt();

  if      (cmd == "demo")   { demoActive = true; Serial.println("Demo starting... (press ENTER to stop)"); }
  else if (cmd == "off")    { handleMessage(MY_BED_ID, CMD_OFF, 0, 0);    Serial.println("All off."); }
  else if (cmd == "all")    { handleMessage(MY_BED_ID, CMD_ALL, 0, a);    Serial.printf("All -> %d%%\n", a); }
  else if (cmd == "zone")   { handleMessage(MY_BED_ID, CMD_ZONE, a, b);   Serial.printf("%s -> %d%%\n", ZONE_NAME[constrain(a,0,3)], b); }
  else if (cmd == "motor")  { handleMessage(MY_BED_ID, CMD_MOTOR, a, b);  Serial.printf("Motor %d -> %d%%\n", a, b); }
  else if (cmd == "wave")   { handleMessage(MY_BED_ID, CMD_PRESET, 0, a); Serial.println("Wave preset on."); }
  else if (cmd == "timer")  { handleMessage(MY_BED_ID, CMD_TIMER, 0, a);  Serial.printf("Timer %d min.\n", a); }
  else if (cmd == "calib")  { runCalib(a); }
  else if (cmd == "status") { printStatus(); }
  else Serial.println("Commands: demo | motor M N | zone Z N | all N | wave [N] | timer MIN | calib M | off | status");
}

// ============================================================
//  Wave preset (manual mode)
// ============================================================
void tickPreset() {
  if (activePreset != 0) return;
  if (millis() < presetStepMs) return;
  presetStepMs = millis() + 1500;
  for (int z = 0; z < 4; z++) setZone(z, z == presetPhase ? presetLevel : presetLevel / 4);
  presetPhase = (presetPhase + 1) % 4;
}

// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  for (int m = 0; m < NUM_MOTORS; m++) {
    motorLevel[m] = 0;
    ledcSetup(m, PWM_FREQ, PWM_RES);
    ledcAttachPin(MOTOR_PIN[m], m);
    ledcWrite(m, 0);
  }
  randomSeed(esp_random());
  Serial.println();
  Serial.printf("=== BED BOX — TEST %03d — bed #%d, TWO-MOTOR DEMO ===\n", TEST_NUMBER, MY_BED_ID);
  Serial.printf("Calibrated: MIN_DUTY_SMALL=%d, MIN_DUTY_BIG=%d\n", MIN_DUTY_SMALL, MIN_DUTY_BIG);
  Serial.println("Wiring: BIG motor OUT1/2 (Motor0), small motor OUT3/4 (Motor1).");
  Serial.println("Demo auto-starts and loops. Press ENTER (or type) to STOP.");
  Serial.println("Then: motor 0 50 | motor 1 50 | calib 0 | calib 1 | demo | status");
}

void loop() {
  if (demoActive) {
    runDemoPass();
  } else {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') { handleLine(inBuf); inBuf = ""; }
      else if (c != '\r') inBuf += c;
    }
    if (timerEndMs && millis() > timerEndMs) {
      timerEndMs = 0; activePreset = -1;
      for (int m = 0; m < NUM_MOTORS; m++) setMotor(m, 0);
      Serial.println("Session timer finished — motors stopped (settings kept).");
    }
    tickPreset();
  }
}

/* ============================================================
 *                        TEST  007   (end of file)
 *  Bed box — Stage 1 — two-motor demo + manual console
 *  Channel 0 = BIG (MIN_DUTY_BIG=165), Channel 1 = small (MIN_DUTY_SMALL=60)
 *  PWM: core 2.x channel API. platformio.ini monitor_speed=115200
 * ============================================================ */
