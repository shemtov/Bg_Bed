// BED BOX FIRMWARE - TEST 012
// QuinLED-ESP32 (WROOM-32E) - one bed, 6 motors, 3x L298N
// TEST 012 - reports its zone levels back up the wire
//
// WHAT CHANGED vs TEST 011
//   The link is two-way now. Whenever a zone's level changes the box
//   sends one frame up to the panel:  {bedId, 20, zone, level}.
//   That is what lets the panel's sliders follow a running pattern
//   instead of sitting still - the panel has never known what the
//   pattern was doing, because nothing ever told it.
//   Sent at most every REPORT_MS, and only for zones that actually
//   moved, so the line stays quiet when nothing is happening.
//
// WHAT CHANGED in TEST 011 vs TEST 010
//   1. BOOT_SELFTEST. On power-up the board sweeps every motor in turn,
//      one at a time, and REPEATS the pass every few seconds until you
//      press a key. Nothing has to be typed for the motors to move, so
//      this separates a wiring fault from a console fault. Set
//      BOOT_SELFTEST to 0 once the motors are proven.
//   2. The banner is reprinted at the top of every self-test pass, so
//      you can open the monitor at any moment and still see it.
//   3. The console now accepts a line ending in CR, LF or both, and
//      ignores empty lines. TEST 010 only acted on LF, so a monitor
//      that sends CR alone left the text hanging and the next line
//      arrived joined to it - which is why help kept appearing.
//   4. An unrecognised command is echoed back in quotes with its byte
//      count, so we can see exactly what reached the board.
//
// WHAT CHANGED in TEST 010 vs TEST 009
//   Renamed local constants that collided with Arduino macros. LOW is
//   #defined as 0 by Arduino.h, so  const int LOW = ...  expanded to
//   const int 0 = ...  and failed. IN, OUT, HOLD and REST were renamed
//   at the same time for the same reason, before they bite too.
//
// WHAT CHANGED in TEST 009 vs TEST 008
//   Core 3.x removed ledcSetup() and ledcAttachPin(). The replacement is
//   ledcAttach(pin, freq, resolution), and from then on ledcWrite() takes
//   the PIN NUMBER, not a channel number. Core 2.x has the old pair and
//   ledcWrite() takes a channel. Rather than pin the platform and hope,
//   this file detects the core at compile time and uses whichever exists.
//   Nothing else changed - the pattern engine and the UART link are the same.
//
// WHAT CHANGED vs TEST 007
//   1. Serial2 receiver on RX=16 TX=17. Four-byte frames from the panel now
//      reach handleMessage(). Every frame is printed on the USB console.
//   2. The auto demo NO LONGER starts by itself. Type  demo  to run it.
//   3. loadState() is finally called in setup(). TEST 007 defined it and
//      never called it, so the bed box never actually resumed on power-on.
//   4. Six motors, not eight. New layout:
//        m0 pin 13  head left    small   \  driver 1
//        m1 pin 14  head right   small   /
//        m2 pin 18  upper back   BIG     \  driver 2
//        m3 pin 19  lower back   BIG     /
//        m4 pin 21  leg left     small   \  driver 3
//        m5 pin 22  leg right    small   /
//   5. A non-blocking pattern engine with cross-fading, and eleven patterns.
//   6. MY_BED_ID and MOTORS_PRESENT are single constants at the top.
//
// WIRING
//   Each L298N: 12V to VS, common ground with the QuinLED, IN1/IN3 to GND,
//   IN2/IN4 to +5V (single direction). Only ENA/ENB go to the ESP32.
//   Panel link: 3 wires, crossed. Panel TX -> our RX 16, panel RX <- our TX 17,
//   ground shared. 5V is NEVER connected between the two boards.
//
// CONSOLE (USB, 115200)
//   motor M N | zone Z N | all N | preset P [N] | timer MIN | off
//   demo | calib M | status | link

#include <Arduino.h>
#include <Preferences.h>

#define TEST_NUMBER 12

// Set to 0 once the motors are proven. While it is 1 the board sweeps all
// motors on power-up and keeps repeating until a key is pressed.
#define BOOT_SELFTEST 0

// Arduino core 2.x drives LEDC by channel number, core 3.x by pin number.
// Detect once here so the rest of the file never has to care.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  #define LEDC_BY_PIN 1
#else
  #define LEDC_BY_PIN 0
#endif

// ---------------- What is physically connected ----------------
const uint8_t MY_BED_ID      = 1;   // this box answers to bedId 1 (and 0 = all)
const int     NUM_MOTORS     = 6;
const int     MOTORS_PRESENT = 6;   // lower this if fewer are wired

const int  MOTOR_PIN[NUM_MOTORS]  = {13, 14, 18, 19, 21, 22};
const int  MOTOR_ZONE[NUM_MOTORS] = { 0,  0,  1,  2,  3,  3};
const int  MOTOR_BIG[NUM_MOTORS]  = { 0,  0,  1,  1,  0,  0};  // 1 = big 0.3A

const int   NUM_ZONES = 4;
const char* ZONE_NAME[NUM_ZONES] = {"Head", "UpperBack", "LowerBack", "Legs"};

// Named motors, for readability inside the patterns
const int HEAD_L = 0, HEAD_R = 1, BACK_U = 2, BACK_L = 3, LEG_L = 4, LEG_R = 5;

// ---------------- PWM ----------------
const int PWM_FREQ = 20000;   // 20 kHz, silent
const int PWM_RES  = 8;       // duty 0..255

// ---------------- Calibration ----------------
int       MIN_DUTY_SMALL = 60;    // measured cold-start ~55
int       MIN_DUTY_BIG   = 165;   // measured cold-start ~165-180
const int KICK_MS        = 60;    // kick-start, direct commands only

// ---------------- Pattern engine tuning ----------------
const int TICK_MS     = 20;   // engine rate
const int RAMP_STEP   = 4;    // level units per tick -> 0..100 in ~500 ms
const int FLOOR_LEVEL = 0;    // raise to 4-6 if starts feel too abrupt

// ---------------- UART link ----------------
const int UART_RX = 16;
const int UART_TX = 17;
const unsigned long FRAME_GAP_MS = 50;   // resync gap between frames

// ---------------- Protocol ----------------
enum Cmd { CMD_OFF = 0, CMD_ALL = 1, CMD_ZONE = 2,
           CMD_MOTOR = 3, CMD_PRESET = 4, CMD_TIMER = 5 };

// Uplink only - the panel listens for this, the bed boxes never send
// anything else. Keeping it well above the downlink numbers means a
// stray echo can never be mistaken for a command.
const uint8_t RPT_LEVEL = 20;
const unsigned long REPORT_MS = 150;   // fastest we will speak
const int REPORT_STEP = 2;             // ignore changes smaller than this

// ---------------- Patterns ----------------
enum Pattern {
  PAT_WATERFALL = 0,   // head down to legs
  PAT_RISE      = 1,   // legs up to head
  PAT_ROCK      = 2,   // down and up, no pause
  PAT_DIAGONAL  = 3,   // head left + leg right, then the other diagonal
  PAT_SIDE      = 4,   // whole left side, then whole right side
  PAT_CIRCLE    = 5,   // around the body
  PAT_KNEAD     = 6,   // the two spine motors alternating
  PAT_PULSE     = 7,   // two beats then quiet
  PAT_BREATHE   = 8,   // one slow rise and fall, everything together
  PAT_RAIN      = 9,   // short random taps
  PAT_SHUFFLE   = 10,  // random zone, level and duration
  PAT_COUNT     = 11
};
const char* PATTERN_NAME[PAT_COUNT] = {
  "Waterfall", "Rise", "Rock", "Diagonal", "Side", "Circle",
  "Knead", "Pulse", "Breathe", "Rain", "Shuffle"
};

// ---------------- State ----------------
int  motorLevel[NUM_MOTORS];        // what is actually driven now
int  tgtLevel[NUM_MOTORS];          // where the pattern wants it
unsigned long timerEndMs  = 0;
unsigned long nextTickMs  = 0;
unsigned long stepAtMs    = 0;
unsigned long patStartMs  = 0;
int  activePreset = -1;
int  presetLevel  = 60;
int  phase        = 0;
bool demoActive   = false;          // TEST 008: no longer auto-starts
Preferences prefs;

int      lastSentZone[NUM_ZONES];
unsigned long lastReportMs = 0;

uint8_t  frame[4];
int      frameLen     = 0;
unsigned long lastByteMs = 0;
uint8_t  lastFrame[4]  = {0, 0, 0, 0};
unsigned long lastFrameMs = 0;
long     framesSeen   = 0;

// ---------------- Forward declarations ----------------
int  levelToDuty(int m, int level);
void pwmWrite(int m, int duty);
void setMotor(int m, int level);
void setZone(int z, int level);
void setAll(int level);
void allOff();
void handleMessage(uint8_t bedId, uint8_t cmd, uint8_t target, uint8_t value);
void saveState();
void loadState();
void startPattern(int p, int level);
void tickEngine();
void runCalib(int m);
void printStatus();
void printLink();
void handleLine(String line);
void pollUart();
void reportLevels();
void demoSet(int m, int level);
bool demoWait(unsigned long ms);
bool demoRamp(int m, int fromL, int toL, unsigned long ms);
void runDemoPass();
void stopDemoToManual();

// ============================================================
//  Low level
// ============================================================
void pwmWrite(int m, int duty) {
#if LEDC_BY_PIN
  ledcWrite(MOTOR_PIN[m], duty);      // core 3.x addresses the pin
#else
  ledcWrite(m, duty);                 // core 2.x addresses the channel
#endif
}

int levelToDuty(int m, int level) {
  if (level <= 0) return 0;
  if (level > 100) level = 100;
  int minDuty = MOTOR_BIG[m] ? MIN_DUTY_BIG : MIN_DUTY_SMALL;
  return minDuty + (255 - minDuty) * (level - 1) / 99;
}

// Direct command: uses the kick-start, because it must start instantly.
void setMotor(int m, int level) {
  if (m < 0 || m >= MOTORS_PRESENT) return;
  bool wasOff = (motorLevel[m] == 0);
  motorLevel[m] = constrain(level, 0, 100);
  tgtLevel[m]   = motorLevel[m];
  int duty = levelToDuty(m, motorLevel[m]);
  if (duty > 0 && wasOff && duty < 255) { pwmWrite(m, 255); delay(KICK_MS); }
  pwmWrite(m, duty);
}

void setZone(int z, int level) {
  for (int m = 0; m < MOTORS_PRESENT; m++)
    if (MOTOR_ZONE[m] == z) setMotor(m, level);
}

void setAll(int level) {
  for (int m = 0; m < MOTORS_PRESENT; m++) setMotor(m, level);
}

void allOff() {
  activePreset = -1;
  timerEndMs   = 0;
  for (int m = 0; m < NUM_MOTORS; m++) { tgtLevel[m] = 0; }
  setAll(0);
}

// Pattern target helpers, no kick-start - the ramp does the work.
void tgtMotor(int m, int level) {
  if (m >= 0 && m < MOTORS_PRESENT) tgtLevel[m] = constrain(level, 0, 100);
}
void tgtZone(int z, int level) {
  for (int m = 0; m < MOTORS_PRESENT; m++)
    if (MOTOR_ZONE[m] == z) tgtLevel[m] = constrain(level, 0, 100);
}
void tgtAll(int level) {
  for (int m = 0; m < MOTORS_PRESENT; m++) tgtLevel[m] = constrain(level, 0, 100);
}

// ============================================================
//  Protocol
// ============================================================
void handleMessage(uint8_t bedId, uint8_t cmd, uint8_t target, uint8_t value) {
  if (bedId != MY_BED_ID && bedId != 0) return;
  switch (cmd) {
    case CMD_OFF:   allOff();                                     break;
    case CMD_ALL:   activePreset = -1; setAll(value);             break;
    case CMD_ZONE:  activePreset = -1; setZone(target, value);    break;
    case CMD_MOTOR: activePreset = -1; setMotor(target, value);   break;
    case CMD_PRESET: startPattern(target, value ? value : 60);    break;
    case CMD_TIMER: timerEndMs = millis() + (unsigned long)value * 60000UL; break;
    default: return;
  }
  saveState();
}

// ============================================================
//  UART receiver - four bytes, resynced by a quiet gap
// ============================================================
void pollUart() {
  while (Serial2.available()) {
    unsigned long now = millis();
    if (frameLen > 0 && now - lastByteMs > FRAME_GAP_MS) frameLen = 0;
    lastByteMs = now;

    frame[frameLen++] = (uint8_t)Serial2.read();

    if (frameLen == 4) {
      frameLen = 0;
      framesSeen++;
      memcpy(lastFrame, frame, 4);
      lastFrameMs = now;
      Serial.printf("[LINK] bed=%u cmd=%u target=%u value=%u\n",
                    frame[0], frame[1], frame[2], frame[3]);
      handleMessage(frame[0], frame[1], frame[2], frame[3]);
    }
  }
}

// ============================================================
//  Uplink - tell the panel what the zones are doing
// ============================================================
void reportLevels() {
  unsigned long now = millis();
  if (now - lastReportMs < REPORT_MS) return;
  lastReportMs = now;

  for (int z = 0; z < NUM_ZONES; z++) {
    int lvl = 0;
    for (int m = 0; m < MOTORS_PRESENT; m++)
      if (MOTOR_ZONE[m] == z && motorLevel[m] > lvl) lvl = motorLevel[m];

    if (abs(lvl - lastSentZone[z]) < REPORT_STEP) continue;
    lastSentZone[z] = lvl;

    uint8_t out[4] = { MY_BED_ID, RPT_LEVEL, (uint8_t)z, (uint8_t)lvl };
    Serial2.write(out, 4);
  }
}

// ============================================================
//  Persistence
// ============================================================
void saveState() {
  prefs.begin("bedbox", false);
  prefs.putBytes("levels", motorLevel, sizeof(motorLevel));
  prefs.putInt("preset", activePreset);
  prefs.putInt("plevel", presetLevel);
  prefs.end();
}

void loadState() {
  int saved[NUM_MOTORS];
  for (int m = 0; m < NUM_MOTORS; m++) saved[m] = 0;
  int savedPreset = -1;

  prefs.begin("bedbox", true);
  if (prefs.isKey("levels")) prefs.getBytes("levels", saved, sizeof(saved));
  savedPreset = prefs.getInt("preset", -1);
  presetLevel = prefs.getInt("plevel", 60);
  prefs.end();

  if (savedPreset >= 0 && savedPreset < PAT_COUNT) {
    Serial.printf("Resuming pattern %s at %d%%\n",
                  PATTERN_NAME[savedPreset], presetLevel);
    startPattern(savedPreset, presetLevel);
    return;
  }

  bool any = false;
  for (int m = 0; m < MOTORS_PRESENT; m++) {
    if (saved[m] > 0) { setMotor(m, saved[m]); any = true; }
  }
  if (any) Serial.println("Resuming the levels that were running at power-off.");
  else     Serial.println("Nothing to resume - starting idle.");
}

// ============================================================
//  Pattern engine
// ============================================================
void startPattern(int p, int level) {
  if (p < 0 || p >= PAT_COUNT) return;
  activePreset = p;
  presetLevel  = constrain(level, 10, 100);
  phase        = 0;
  stepAtMs     = 0;
  patStartMs   = millis();
  Serial.printf("Pattern %s at %d%%\n", PATTERN_NAME[p], presetLevel);
}

// Advance whichever pattern is running. Only sets targets - never writes PWM.
static void advancePattern() {
  const int L    = presetLevel;
  const int BASE = max(FLOOR_LEVEL, L / 5);
  const unsigned long now = millis();

  switch (activePreset) {

    case PAT_WATERFALL:
      if (now < stepAtMs) return;
      stepAtMs = now + 1600;
      for (int z = 0; z < NUM_ZONES; z++) tgtZone(z, z == phase ? L : BASE);
      phase = (phase + 1) % NUM_ZONES;
      break;

    case PAT_RISE:
      if (now < stepAtMs) return;
      stepAtMs = now + 1600;
      for (int z = 0; z < NUM_ZONES; z++)
        tgtZone(z, z == (NUM_ZONES - 1 - phase) ? L : BASE);
      phase = (phase + 1) % NUM_ZONES;
      break;

    case PAT_ROCK: {
      if (now < stepAtMs) return;
      stepAtMs = now + 1400;
      const int seq[6] = {0, 1, 2, 3, 2, 1};
      for (int z = 0; z < NUM_ZONES; z++) tgtZone(z, z == seq[phase] ? L : BASE);
      phase = (phase + 1) % 6;
      break;
    }

    case PAT_DIAGONAL:
      if (now < stepAtMs) return;
      stepAtMs = now + 2500;
      tgtZone(1, BASE); tgtZone(2, BASE);
      if (phase == 0) { tgtMotor(HEAD_L, L); tgtMotor(HEAD_R, FLOOR_LEVEL);
                        tgtMotor(LEG_R,  L); tgtMotor(LEG_L,  FLOOR_LEVEL); }
      else            { tgtMotor(HEAD_R, L); tgtMotor(HEAD_L, FLOOR_LEVEL);
                        tgtMotor(LEG_L,  L); tgtMotor(LEG_R,  FLOOR_LEVEL); }
      phase ^= 1;
      break;

    case PAT_SIDE:
      if (now < stepAtMs) return;
      stepAtMs = now + 2500;
      tgtZone(1, BASE); tgtZone(2, BASE);
      if (phase == 0) { tgtMotor(HEAD_L, L); tgtMotor(LEG_L,  L);
                        tgtMotor(HEAD_R, FLOOR_LEVEL); tgtMotor(LEG_R, FLOOR_LEVEL); }
      else            { tgtMotor(HEAD_R, L); tgtMotor(LEG_R,  L);
                        tgtMotor(HEAD_L, FLOOR_LEVEL); tgtMotor(LEG_L, FLOOR_LEVEL); }
      phase ^= 1;
      break;

    case PAT_CIRCLE: {
      if (now < stepAtMs) return;
      stepAtMs = now + 1200;
      const int ring[4] = {HEAD_L, HEAD_R, LEG_R, LEG_L};
      tgtZone(1, L / 4); tgtZone(2, L / 4);
      for (int i = 0; i < 4; i++) tgtMotor(ring[i], i == phase ? L : FLOOR_LEVEL);
      phase = (phase + 1) % 4;
      break;
    }

    case PAT_KNEAD:
      if (now < stepAtMs) return;
      stepAtMs = now + 1400;
      tgtZone(0, L / 6); tgtZone(3, L / 6);
      tgtMotor(BACK_U, phase == 0 ? L : FLOOR_LEVEL);
      tgtMotor(BACK_L, phase == 0 ? FLOOR_LEVEL : L);
      phase ^= 1;
      break;

    case PAT_PULSE: {
      if (now < stepAtMs) return;
      const unsigned long dur[4] = {260, 260, 260, 1800};
      stepAtMs = now + dur[phase];
      tgtZone(0, FLOOR_LEVEL); tgtZone(3, FLOOR_LEVEL);
      bool on = (phase == 0 || phase == 2);
      tgtMotor(BACK_U, on ? L : FLOOR_LEVEL);
      tgtMotor(BACK_L, on ? L : FLOOR_LEVEL);
      phase = (phase + 1) % 4;
      break;
    }

    case PAT_BREATHE: {
      // 4 s in, 0.8 s hold, 6 s out, 1.2 s rest
      const unsigned long T_IN = 4000, T_HOLD = 800, T_OUT = 6000, T_REST = 1200;
      const unsigned long cycle = T_IN + T_HOLD + T_OUT + T_REST;
      unsigned long t = (now - patStartMs) % cycle;
      int lvl;
      if      (t < T_IN)                 lvl = (int)((long)L * t / T_IN);
      else if (t < T_IN + T_HOLD)        lvl = L;
      else if (t < T_IN + T_HOLD + T_OUT)
              lvl = (int)((long)L * (T_IN + T_HOLD + T_OUT - t) / T_OUT);
      else                               lvl = 0;
      tgtAll(max(lvl, FLOOR_LEVEL));
      break;
    }

    case PAT_RAIN:
      if (now < stepAtMs) return;
      if (phase == 0) {                       // a drop
        int m = random(0, MOTORS_PRESENT);
        tgtAll(FLOOR_LEVEL);
        tgtMotor(m, max(30, L * 2 / 3));
        stepAtMs = now + random(300, 700);
        phase = 1;
      } else {                                // the gap
        tgtAll(FLOOR_LEVEL);
        stepAtMs = now + random(400, 1600);
        phase = 0;
      }
      break;

    case PAT_SHUFFLE: {
      if (now < stepAtMs) return;
      stepAtMs = now + random(1500, 4000);
      int z   = random(0, NUM_ZONES);
      int lvl = random(max(30, L / 2), L + 1);
      for (int i = 0; i < NUM_ZONES; i++) tgtZone(i, i == z ? lvl : FLOOR_LEVEL);
      break;
    }

    default: break;
  }
}

// Ramp every motor toward its target and write the PWM. No kick-start here:
// the ramp crosses the break-loose threshold on its own, which is what makes
// the patterns feel like movement instead of switching.
void tickEngine() {
  unsigned long now = millis();
  if (now < nextTickMs) return;
  nextTickMs = now + TICK_MS;

  if (activePreset >= 0) advancePattern();

  for (int m = 0; m < MOTORS_PRESENT; m++) {
    if (motorLevel[m] == tgtLevel[m]) continue;
    int diff = tgtLevel[m] - motorLevel[m];
    int step = (abs(diff) < RAMP_STEP) ? abs(diff) : RAMP_STEP;
    motorLevel[m] += (diff > 0) ? step : -step;
    pwmWrite(m, levelToDuty(m, motorLevel[m]));
  }
}

// ============================================================
//  Console
// ============================================================
String inBuf;

void runCalib(int m) {
  if (m < 0 || m >= MOTORS_PRESENT) { Serial.println("bad motor #"); return; }
  Serial.printf("Calibrating motor %d (%s, %s). Ramping duty slowly.\n",
                m, ZONE_NAME[MOTOR_ZONE[m]], MOTOR_BIG[m] ? "BIG" : "small");
  Serial.println("Watch the motor. Press ENTER the moment it starts.");
  while (Serial.available()) Serial.read();
  for (int duty = 30; duty <= 255; duty += 5) {
    pwmWrite(m, duty);
    Serial.printf("  duty = %d  (%.0f%%)\n", duty, duty * 100.0 / 255);
    unsigned long t0 = millis();
    while (millis() - t0 < 700) {
      if (Serial.available()) {
        while (Serial.available()) Serial.read();
        pwmWrite(m, 0);
        Serial.printf("\nStart threshold: duty %d\n", duty);
        Serial.printf("Set %s to about %d (threshold plus margin).\n",
                      MOTOR_BIG[m] ? "MIN_DUTY_BIG" : "MIN_DUTY_SMALL", duty + 8);
        return;
      }
      delay(10);
    }
  }
  pwmWrite(m, 0);
  Serial.println("Ramp finished without a keypress - motor never started?");
}

void printStatus() {
  Serial.println("---- STATUS ----");
  for (int m = 0; m < MOTORS_PRESENT; m++)
    Serial.printf(" m%d pin%-3d %-10s %-5s level %3d%% target %3d%% duty %3d\n",
                  m, MOTOR_PIN[m], ZONE_NAME[MOTOR_ZONE[m]],
                  MOTOR_BIG[m] ? "BIG" : "small",
                  motorLevel[m], tgtLevel[m], levelToDuty(m, motorLevel[m]));
  Serial.printf(" thresholds: small=%d big=%d\n", MIN_DUTY_SMALL, MIN_DUTY_BIG);
  Serial.printf(" pattern: %s\n",
                activePreset >= 0 ? PATTERN_NAME[activePreset] : "none");
  if (timerEndMs)
    Serial.printf(" timer: %lu s left\n", (timerEndMs - millis()) / 1000UL);
  Serial.printf(" demo: %s\n", demoActive ? "RUNNING" : "stopped");
  Serial.println("----------------");
}

void printLink() {
  Serial.println("---- LINK ----");
  Serial.printf(" bed id      : %u\n", MY_BED_ID);
  Serial.printf(" uart        : RX=%d TX=%d @115200\n", UART_RX, UART_TX);
  Serial.printf(" frames seen : %ld\n", framesSeen);
  if (framesSeen)
    Serial.printf(" last frame  : %u %u %u %u  (%lu ms ago)\n",
                  lastFrame[0], lastFrame[1], lastFrame[2], lastFrame[3],
                  millis() - lastFrameMs);
  else
    Serial.println(" last frame  : none yet - check TX/RX crossed and GND shared");
  Serial.println("--------------");
}

void handleLine(String line) {
  line.trim(); line.toLowerCase();
  if (line.length() == 0) return;
  int sp1 = line.indexOf(' ');
  String cmd  = sp1 < 0 ? line : line.substring(0, sp1);
  String rest = sp1 < 0 ? "" : line.substring(sp1 + 1);
  int a = rest.toInt();
  int sp2 = rest.indexOf(' ');
  int b = sp2 < 0 ? -1 : rest.substring(sp2 + 1).toInt();

  if      (cmd == "off")    { handleMessage(MY_BED_ID, CMD_OFF, 0, 0); Serial.println("All off."); }
  else if (cmd == "all")    { handleMessage(MY_BED_ID, CMD_ALL, 0, a); }
  else if (cmd == "zone")   { handleMessage(MY_BED_ID, CMD_ZONE, a, b); }
  else if (cmd == "motor")  { handleMessage(MY_BED_ID, CMD_MOTOR, a, b); }
  else if (cmd == "preset") { handleMessage(MY_BED_ID, CMD_PRESET, a, b < 0 ? 60 : b); }
  else if (cmd == "timer")  { handleMessage(MY_BED_ID, CMD_TIMER, 0, a); Serial.printf("Timer %d min.\n", a); }
  else if (cmd == "calib")  { runCalib(a); }
  else if (cmd == "status") { printStatus(); }
  else if (cmd == "link")   { printLink(); }
  else if (cmd == "demo")   { demoActive = true; Serial.println("Demo starting - press ENTER to stop."); }
  else if (cmd == "list") {
    for (int p = 0; p < PAT_COUNT; p++) Serial.printf("  %2d  %s\n", p, PATTERN_NAME[p]);
  }
  else {
    Serial.printf("Unknown command: \"%s\" (%d chars)\n", line.c_str(), line.length());
    Serial.println("motor M N | zone Z N | all N | preset P [N] | timer MIN | off");
    Serial.println("demo | calib M | status | link | list");
  }
}

// ============================================================
//  Demo - unchanged in spirit, but no longer automatic
// ============================================================
void demoSet(int m, int level) {
  if (m < 0 || m >= MOTORS_PRESENT) return;
  motorLevel[m] = constrain(level, 0, 100);
  tgtLevel[m]   = motorLevel[m];
  pwmWrite(m, levelToDuty(m, motorLevel[m]));
}

void stopDemoToManual() {
  demoActive = false;
  for (int m = 0; m < MOTORS_PRESENT; m++) { motorLevel[m] = 0; tgtLevel[m] = 0; pwmWrite(m, 0); }
  Serial.println();
  Serial.println("Demo stopped.");
}

bool demoWait(unsigned long ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    pollUart();
    if (Serial.available()) {
      while (Serial.available()) Serial.read();
      stopDemoToManual();
      return true;
    }
    delay(5);
  }
  return false;
}

bool demoRamp(int m, int fromL, int toL, unsigned long ms) {
  const int steps = 40;
  for (int i = 0; i <= steps; i++) {
    if (!demoActive) return true;
    demoSet(m, fromL + (toL - fromL) * i / steps);
    if (demoWait(ms / steps)) return true;
  }
  return false;
}

void runDemoPass() {
  Serial.println();
  Serial.printf("=== BED BOX - TEST %03d - bed #%d - %d motors ===\n",
                TEST_NUMBER, MY_BED_ID, MOTORS_PRESENT);
  Serial.printf("LEDC mode: %s\n", LEDC_BY_PIN ? "core 3.x, by pin" : "core 2.x, by channel");
  Serial.println("Self-test running. Press any key to stop and get the console.");

  for (int m = 0; m < MOTORS_PRESENT; m++) {
    Serial.printf("[DEMO] motor %d  %s %s  ramp up and down\n",
                  m, ZONE_NAME[MOTOR_ZONE[m]], MOTOR_BIG[m] ? "BIG" : "small");
    if (demoRamp(m, 0, 100, 2500)) return;
    if (demoWait(600)) return;
    if (demoRamp(m, 100, 0, 1800)) return;
    demoSet(m, 0);
    if (demoWait(400)) return;
  }
  Serial.println("[DEMO] all together");
  for (int i = 0; i <= 40; i++) {
    if (!demoActive) return;
    for (int m = 0; m < MOTORS_PRESENT; m++) demoSet(m, 100 * i / 40);
    if (demoWait(70)) return;
  }
  if (demoWait(900)) return;
  for (int i = 40; i >= 0; i--) {
    if (!demoActive) return;
    for (int m = 0; m < MOTORS_PRESENT; m++) demoSet(m, 100 * i / 40);
    if (demoWait(55)) return;
  }
  for (int m = 0; m < MOTORS_PRESENT; m++) demoSet(m, 0);
  Serial.println("[DEMO] pass complete, looping");
  if (demoWait(1200)) return;
}

// ============================================================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  delay(300);

  for (int z = 0; z < NUM_ZONES; z++) lastSentZone[z] = -99;
  for (int m = 0; m < NUM_MOTORS; m++) {
    motorLevel[m] = 0;
    tgtLevel[m]   = 0;
#if LEDC_BY_PIN
    ledcAttach(MOTOR_PIN[m], PWM_FREQ, PWM_RES);
#else
    ledcSetup(m, PWM_FREQ, PWM_RES);
    ledcAttachPin(MOTOR_PIN[m], m);
#endif
    pwmWrite(m, 0);
  }
  randomSeed(esp_random());

  Serial.println();
  Serial.printf("=== BED BOX - TEST %03d - bed #%d - %d motors ===\n",
                TEST_NUMBER, MY_BED_ID, MOTORS_PRESENT);
  Serial.printf("UART link on RX=%d TX=%d @115200. Listening for 4-byte frames.\n",
                UART_RX, UART_TX);
  Serial.printf("Calibration: MIN_DUTY_SMALL=%d MIN_DUTY_BIG=%d\n",
                MIN_DUTY_SMALL, MIN_DUTY_BIG);
  Serial.printf("LEDC mode: %s\n", LEDC_BY_PIN ? "core 3.x, by pin" : "core 2.x, by channel");
  Serial.println("Type  list  for the patterns, or  link  to check the panel wire.");

  loadState();

#if BOOT_SELFTEST
  demoActive = true;
  Serial.println("BOOT_SELFTEST is on - sweeping the motors now.");
#endif
}

void loop() {
  if (demoActive) {
    runDemoPass();
    return;
  }

  pollUart();

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {          // CR, LF or CRLF all end a line
      if (inBuf.length()) { handleLine(inBuf); inBuf = ""; }
    } else {
      inBuf += c;
    }
  }

  if (timerEndMs && millis() > timerEndMs) {
    timerEndMs = 0;
    activePreset = -1;
    for (int m = 0; m < MOTORS_PRESENT; m++) { tgtLevel[m] = 0; setMotor(m, 0); }
    Serial.println("Session timer finished - motors stopped, settings kept.");
  }

  tickEngine();
  reportLevels();
}

// BED BOX - TEST 012 - end of file
// 6 motors, 3 drivers, two-way UART on 16/17, 11 patterns
// TEST 012 - reports zone levels up to the panel