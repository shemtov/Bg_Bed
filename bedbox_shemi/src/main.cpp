// ============================================================
// BED BOX SHEMI - TEST 024
// QuinLED-ESP32 (WROOM-32E) - 6 motors + RS485 + 7 RGBW NeoPixels
// ============================================================
//
// TEST 024
// - Diagnostic LED behavior redesigned for closed-loop testing.
// - Startup: ALL 7 LEDs flash RED twice.
// - Bus alive: ONE GREEN LED scans back-and-forth.
// - Bus silent for 2 seconds: ONE RED LED scans back-and-forth.
// - Valid massage command Panel -> BedBox: 3-BLUE train moves LEFT-to-RIGHT.
// - BedBox feedback -> Panel: 3-YELLOW train moves RIGHT-to-LEFT.
// - If feedback is transmitted during the blue command indication, yellow is
//   queued and starts immediately after blue, so both directions are visible.
// - A5 TIME/ACK traffic keeps the communication-alive indication green but does
//   NOT trigger the blue command train.
// - Bad A5 checksum: all 7 LEDs flash RED briefly.
// - Motor control, pattern engine, RS485 parser, GPIO mapping and RGBW format
//   are unchanged from TEST023.
// - NOT YET RUN ON HARDWARE.
//
// TEST 023
// - Full BedBox Shemi system restored after the standalone LED tests.
// - Exactly 7 physical NeoPixels.
// - IMPORTANT FIX proven by TEST022: LEDs are RGBW, so use NEO_GRBW (32-bit),
//   not NEO_GRB (24-bit).
// - NeoPixel DATA = GPIO23 through the external 3.3V -> 5V level shifter.
// - Startup: ALL 7 LEDs flash RED twice.
// - Normal: a train of 3 GREEN LEDs moves continuously right-to-left.
// - Valid received A5/14-byte frame: the SAME train turns YELLOW briefly.
// - BedBox transmission: the SAME train turns BLUE briefly.
// - Bad A5 checksum: all 7 LEDs flash RED briefly.
// - RS485 parser and motor/pattern behavior are restored from TEST018.
// - USB Serial diagnostics remain active.
//
// TEST022 proved the RGBW LED format and GPIO23+level-shifter path on hardware.
// TEST023 full-system integration is NOT proven until built/uploaded/tested.
//
// // WIRING
// NeoPixels: +5V -> 5V, GND -> GND, DIN <- HV1 of external level shifter; GPIO23 -> LV1.
// RS485 module TTL side:
//   VCC -> 5V, GND -> GND, TXD -> GPIO16, RXD -> GPIO17.
// RS485 bus side: A+ -> A, B- -> B, Earth -> common GND.
//
// ============================================================

#include <Arduino.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

#define TEST_NUMBER 24

#define BOOT_SELFTEST 0

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  #define LEDC_BY_PIN 1
#else
  #define LEDC_BY_PIN 0
#endif

// ---------------- Physical configuration ----------------
const uint8_t MY_BED_ID      = 1;
const int     NUM_MOTORS     = 6;
const int     MOTORS_PRESENT = 6;

const int MOTOR_PIN[NUM_MOTORS]  = {13, 14, 18, 19, 21, 22};
const int MOTOR_ZONE[NUM_MOTORS] = { 0,  0,  1,  2,  3,  3};
const int MOTOR_BIG[NUM_MOTORS]  = { 0,  0,  1,  1,  0,  0};

const int   NUM_ZONES = 4;
const char* ZONE_NAME[NUM_ZONES] = {"Head", "UpperBack", "LowerBack", "Legs"};

const int HEAD_L = 0, HEAD_R = 1, BACK_U = 2, BACK_L = 3, LEG_L = 4, LEG_R = 5;

// ---------------- PWM ----------------
const int PWM_FREQ = 20000;
const int PWM_RES  = 8;

int       MIN_DUTY_SMALL = 60;
int       MIN_DUTY_BIG   = 165;
const int KICK_MS        = 60;

const int TICK_MS     = 20;
const int RAMP_STEP   = 4;
const int FLOOR_LEVEL = 0;

// ---------------- UART / RS485 ----------------
const int UART_RX = 16;
const int UART_TX = 17;
const unsigned long FRAME_GAP_MS = 50;

// ---------------- NeoPixel diagnostics ----------------
const int NEO_PIN   = 23;
const int NEO_COUNT = 7;
const uint8_t NEO_BRIGHTNESS = 45;

Adafruit_NeoPixel pixels(NEO_COUNT, NEO_PIN, NEO_GRBW + NEO_KHZ800);

enum LedEvent {
  LED_EVT_NONE = 0,
  LED_EVT_COMMAND,    // Panel -> BedBox, BLUE, left-to-right
  LED_EVT_FEEDBACK,   // BedBox -> Panel, YELLOW, right-to-left
  LED_EVT_FAULT
};

LedEvent ledEvent = LED_EVT_NONE;
unsigned long ledEventUntil = 0;
unsigned long ledStepAt = 0;

// Idle scanner: exactly ONE pixel bounces between the ends.
int idlePixel = 0;
int idleDir   = +1;

// Event train head. It is reset when an event starts so direction is obvious.
int eventHead = 0;
bool feedbackPending = false;

// Communication is considered alive after any VALID received A5 or legacy frame.
// TIME/ACK therefore keep this green even when no massage command is being sent.
unsigned long lastValidCommMs = 0;
bool haveValidComm = false;

const unsigned long LED_STEP_MS       = 120;
const unsigned long COMMAND_HOLD_MS   = 520;
const unsigned long FEEDBACK_HOLD_MS  = 520;
const unsigned long FAULT_HOLD_MS     = 900;
const unsigned long COMM_TIMEOUT_MS   = 2000;

uint32_t C_RED, C_GREEN, C_YELLOW, C_BLUE, C_WHITE, C_OFF;

void noteValidCommunication() {
  haveValidComm = true;
  lastValidCommMs = millis();
}

void startLedEvent(LedEvent e, unsigned long holdMs) {
  ledEvent = e;
  ledEventUntil = millis() + holdMs;
  ledStepAt = millis();

  if (e == LED_EVT_COMMAND) {
    eventHead = 0;                 // start at left, move right
  } else if (e == LED_EVT_FEEDBACK) {
    eventHead = NEO_COUNT - 1;     // start at right, move left
  }
}

void showCommandTx() {
  // A new command should always be visible as BLUE first.
  feedbackPending = false;
  startLedEvent(LED_EVT_COMMAND, COMMAND_HOLD_MS);
}

void showFeedbackTx() {
  // A feedback report often follows the command almost immediately.
  // Queue yellow behind blue instead of overwriting blue before it can be seen.
  if (ledEvent == LED_EVT_COMMAND) {
    feedbackPending = true;
    return;
  }
  startLedEvent(LED_EVT_FEEDBACK, FEEDBACK_HOLD_MS);
}

void setFaultEvent() {
  feedbackPending = false;
  startLedEvent(LED_EVT_FAULT, FAULT_HOLD_MS);
}

void startupLedTest() {
  // Hardware-proven RGBW chain: exactly 7 physical NeoPixels.
  // All seven flash RED twice at startup.
  for (int pass = 0; pass < 2; pass++) {
    pixels.fill(C_RED);
    pixels.show();
    delay(500);

    pixels.clear();
    pixels.show();
    delay(300);
  }

  pixels.clear();
  pixels.show();
  delay(250);
}

void updateDiagnostics() {
  const unsigned long now = millis();

  // Finish a temporary event. Feedback queued during BLUE starts next.
  if (ledEvent != LED_EVT_NONE && (long)(now - ledEventUntil) >= 0) {
    if (ledEvent == LED_EVT_COMMAND && feedbackPending) {
      feedbackPending = false;
      startLedEvent(LED_EVT_FEEDBACK, FEEDBACK_HOLD_MS);
    } else {
      ledEvent = LED_EVT_NONE;
    }
  }

  // Advance animation only on its timing tick.
  if ((long)(now - ledStepAt) >= 0) {
    ledStepAt = now + LED_STEP_MS;

    if (ledEvent == LED_EVT_COMMAND) {
      eventHead++;
      if (eventHead >= NEO_COUNT) eventHead = 0;
    }
    else if (ledEvent == LED_EVT_FEEDBACK) {
      eventHead--;
      if (eventHead < 0) eventHead = NEO_COUNT - 1;
    }
    else if (ledEvent == LED_EVT_NONE) {
      idlePixel += idleDir;
      if (idlePixel >= NEO_COUNT - 1) {
        idlePixel = NEO_COUNT - 1;
        idleDir = -1;
      } else if (idlePixel <= 0) {
        idlePixel = 0;
        idleDir = +1;
      }
    }
  }

  pixels.clear();

  if (ledEvent == LED_EVT_FAULT) {
    bool on = ((now / 140) & 1) == 0;
    if (on) pixels.fill(C_RED);
    pixels.show();
    return;
  }

  if (ledEvent == LED_EVT_COMMAND) {
    // 3-pixel BLUE train, LEFT -> RIGHT.
    for (int i = 0; i < 3; i++) {
      int p = eventHead - i;
      while (p < 0) p += NEO_COUNT;
      pixels.setPixelColor(p, C_BLUE);
    }
    pixels.show();
    return;
  }

  if (ledEvent == LED_EVT_FEEDBACK) {
    // 3-pixel YELLOW train, RIGHT -> LEFT.
    for (int i = 0; i < 3; i++) {
      int p = eventHead + i;
      while (p >= NEO_COUNT) p -= NEO_COUNT;
      pixels.setPixelColor(p, C_YELLOW);
    }
    pixels.show();
    return;
  }

  // Idle: exactly ONE pixel. GREEN = recent valid communication,
  // RED = no valid communication for COMM_TIMEOUT_MS (or none since boot).
  const bool commAlive = haveValidComm && (now - lastValidCommMs <= COMM_TIMEOUT_MS);
  pixels.setPixelColor(idlePixel, commAlive ? C_GREEN : C_RED);
  pixels.show();
}

// ---------------- Protocol ----------------
enum Cmd {
  CMD_OFF = 0,
  CMD_ALL = 1,
  CMD_ZONE = 2,
  CMD_MOTOR = 3,
  CMD_PRESET = 4,
  CMD_TIMER = 5
};

const uint8_t RPT_LEVEL = 20;
const unsigned long REPORT_MS = 150;
const int REPORT_STEP = 2;

// ---------------- Patterns ----------------
enum Pattern {
  PAT_WATERFALL = 0,
  PAT_RISE      = 1,
  PAT_ROCK      = 2,
  PAT_DIAGONAL  = 3,
  PAT_SIDE      = 4,
  PAT_CIRCLE    = 5,
  PAT_KNEAD     = 6,
  PAT_PULSE     = 7,
  PAT_BREATHE   = 8,
  PAT_RAIN      = 9,
  PAT_SHUFFLE   = 10,
  PAT_COUNT     = 11
};

const char* PATTERN_NAME[PAT_COUNT] = {
  "Waterfall", "Rise", "Rock", "Diagonal", "Side", "Circle",
  "Knead", "Pulse", "Breathe", "Rain", "Shuffle"
};

// ---------------- State ----------------
int motorLevel[NUM_MOTORS];
int tgtLevel[NUM_MOTORS];

unsigned long timerEndMs = 0;
unsigned long nextTickMs = 0;
unsigned long stepAtMs   = 0;
unsigned long patStartMs = 0;

int activePreset = -1;
int presetLevel  = 60;
int phase        = 0;
bool demoActive  = false;

Preferences prefs;

int lastSentZone[NUM_ZONES];
unsigned long lastReportMs = 0;

const int A5_FRAME_LEN = 14;
uint8_t a5Frame[A5_FRAME_LEN];
int a5Len = 0;
bool collectingA5 = false;

uint8_t legacyBuf[4];
int legacyLen = 0;

unsigned long lastByteMs = 0;

uint8_t lastFrame[14] = {0};
int lastFrameLen = 0;
unsigned long lastFrameMs = 0;

long a5FramesSeen = 0;
long timeFramesSeen = 0;
long ackFramesSeen = 0;
long legacyFramesSeen = 0;
long checksumFaults = 0;
long rxBytesSeen = 0;
long txFrames = 0;

unsigned long nextAlivePrint = 0;

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
void processA5Frame();
void processLegacyPacket();
void pollUart();
void reportLevels();
void demoSet(int m, int level);
bool demoWait(unsigned long ms);
bool demoRamp(int m, int fromL, int toL, unsigned long ms);
void runDemoPass();
void stopDemoToManual();

// ============================================================
// Low level
// ============================================================
void pwmWrite(int m, int duty) {
#if LEDC_BY_PIN
  ledcWrite(MOTOR_PIN[m], duty);
#else
  ledcWrite(m, duty);
#endif
}

int levelToDuty(int m, int level) {
  if (level <= 0) return 0;
  if (level > 100) level = 100;
  int minDuty = MOTOR_BIG[m] ? MIN_DUTY_BIG : MIN_DUTY_SMALL;
  return minDuty + (255 - minDuty) * (level - 1) / 99;
}

void setMotor(int m, int level) {
  if (m < 0 || m >= MOTORS_PRESENT) return;
  bool wasOff = (motorLevel[m] == 0);
  motorLevel[m] = constrain(level, 0, 100);
  tgtLevel[m]   = motorLevel[m];
  int duty = levelToDuty(m, motorLevel[m]);
  if (duty > 0 && wasOff && duty < 255) {
    pwmWrite(m, 255);
    delay(KICK_MS);
  }
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
  timerEndMs = 0;
  for (int m = 0; m < NUM_MOTORS; m++) tgtLevel[m] = 0;
  setAll(0);
}

void tgtMotor(int m, int level) {
  if (m >= 0 && m < MOTORS_PRESENT)
    tgtLevel[m] = constrain(level, 0, 100);
}

void tgtZone(int z, int level) {
  for (int m = 0; m < MOTORS_PRESENT; m++)
    if (MOTOR_ZONE[m] == z) tgtLevel[m] = constrain(level, 0, 100);
}

void tgtAll(int level) {
  for (int m = 0; m < MOTORS_PRESENT; m++)
    tgtLevel[m] = constrain(level, 0, 100);
}

// ============================================================
// Protocol
// ============================================================
void handleMessage(uint8_t bedId, uint8_t cmd, uint8_t target, uint8_t value) {
  if (bedId != MY_BED_ID && bedId != 0) return;

  switch (cmd) {
    case CMD_OFF:    allOff(); break;
    case CMD_ALL:    activePreset = -1; setAll(value); break;
    case CMD_ZONE:   activePreset = -1; setZone(target, value); break;
    case CMD_MOTOR:  activePreset = -1; setMotor(target, value); break;
    case CMD_PRESET: startPattern(target, value ? value : 60); break;
    case CMD_TIMER:  timerEndMs = millis() + (unsigned long)value * 60000UL; break;
    default: return;
  }

  saveState();
}

// ============================================================
// UART / RS485 receiver
// ============================================================
uint8_t xorChecksum(const uint8_t* data, int len) {
  uint8_t x = 0;
  for (int i = 0; i < len; i++) x ^= data[i];
  return x;
}

void processA5Frame() {
  if (a5Len != A5_FRAME_LEN) return;

  uint8_t calc = xorChecksum(a5Frame, 13);
  uint8_t got  = a5Frame[13];

  if (calc != got) {
    checksumFaults++;
    setFaultEvent();
    Serial.printf("[RS485 A5 BAD CRC] calc=%02X got=%02X :", calc, got);
    for (int i = 0; i < A5_FRAME_LEN; i++) Serial.printf(" %02X", a5Frame[i]);
    Serial.println();
    return;
  }

  a5FramesSeen++;
  memcpy(lastFrame, a5Frame, A5_FRAME_LEN);
  lastFrameLen = A5_FRAME_LEN;
  lastFrameMs = millis();

  uint8_t src  = a5Frame[1];
  uint8_t dst  = a5Frame[2];
  uint8_t type = a5Frame[3];
  uint16_t seq = (uint16_t)a5Frame[4] | ((uint16_t)a5Frame[5] << 8);

  noteValidCommunication();

  if (type == 0x10) {
    timeFramesSeen++;
    uint16_t year = (uint16_t)a5Frame[6] | ((uint16_t)a5Frame[7] << 8);
    uint8_t month  = a5Frame[8];
    uint8_t day    = a5Frame[9];
    uint8_t hour   = a5Frame[10];
    uint8_t minute = a5Frame[11];
    uint8_t second = a5Frame[12];

    Serial.printf("[RS485 A5 TIME] src=%u dst=%u seq=%u %04u-%02u-%02u %02u:%02u:%02u\n",
                  src, dst, seq, year, month, day, hour, minute, second);
  }
  else if (type == 0x11) {
    ackFramesSeen++;
    Serial.printf("[RS485 A5 ACK] src=%u dst=%u seq=%u\n",
                  src, dst, seq);
  }
  else {
    Serial.printf("[RS485 A5] src=%u dst=%u type=0x%02X seq=%u payload:",
                  src, dst, type, seq);
    for (int i = 6; i <= 12; i++) Serial.printf(" %02X", a5Frame[i]);
    Serial.println();
  }
}

void processLegacyPacket() {
  if (legacyLen != 4) return;

  legacyFramesSeen++;
  memcpy(lastFrame, legacyBuf, 4);
  lastFrameLen = 4;
  lastFrameMs = millis();

  noteValidCommunication();

  Serial.printf("[RS485 LEGACY CMD] bed=%u cmd=%u target=%u value=%u\n",
                legacyBuf[0], legacyBuf[1], legacyBuf[2], legacyBuf[3]);

  // Blue is reserved for an actual massage order addressed to this BedBox.
  if ((legacyBuf[0] == MY_BED_ID || legacyBuf[0] == 0) && legacyBuf[1] <= CMD_TIMER) {
    showCommandTx();
  }

  handleMessage(legacyBuf[0], legacyBuf[1], legacyBuf[2], legacyBuf[3]);
}

void pollUart() {
  while (Serial2.available()) {
    unsigned long now = millis();
    uint8_t b = (uint8_t)Serial2.read();

    rxBytesSeen++;

    // Quiet gap closes a pending legacy packet.
    if (!collectingA5 && legacyLen > 0 && now - lastByteMs > FRAME_GAP_MS) {
      if (legacyLen == 4) processLegacyPacket();
      else {
        Serial.printf("[RS485 RAW] ignored legacy-sized fragment len=%d\n", legacyLen);
      }
      legacyLen = 0;
    }

    lastByteMs = now;

    if (collectingA5) {
      a5Frame[a5Len++] = b;

      if (a5Len == A5_FRAME_LEN) {
        processA5Frame();
        collectingA5 = false;
        a5Len = 0;
      }
      continue;
    }

    if (b == 0xA5) {
      // A5 is a hard start marker for the observed 14-byte protocol.
      // Drop any incomplete non-A5 fragment rather than mixing protocols.
      if (legacyLen > 0) {
        Serial.printf("[RS485 RAW] dropped %d byte fragment before A5\n", legacyLen);
        legacyLen = 0;
      }

      collectingA5 = true;
      a5Len = 0;
      a5Frame[a5Len++] = b;
      continue;
    }

    // Non-A5 traffic can still be the legacy 4-byte BedBox protocol.
    if (legacyLen < 4) {
      legacyBuf[legacyLen++] = b;
    } else {
      // More than 4 non-A5 bytes without a quiet gap: not a valid legacy packet.
      Serial.println("[RS485 RAW] non-A5 stream longer than 4 bytes - discarded");
      legacyLen = 0;
    }
  }

  // Close a legacy packet only after a quiet gap.
  if (!collectingA5 && legacyLen > 0 && millis() - lastByteMs > FRAME_GAP_MS) {
    if (legacyLen == 4) processLegacyPacket();
    else Serial.printf("[RS485 RAW] ignored fragment len=%d\n", legacyLen);
    legacyLen = 0;
  }

  // A5 frame timed out before 14 bytes: clear it safely, no motor action.
  if (collectingA5 && a5Len > 0 && millis() - lastByteMs > FRAME_GAP_MS) {
    Serial.printf("[RS485 A5] incomplete frame len=%d - discarded\n", a5Len);
    collectingA5 = false;
    a5Len = 0;
  }
}

// ============================================================
// Uplink
// ============================================================
void reportLevels() {
  unsigned long now = millis();
  if (now - lastReportMs < REPORT_MS) return;
  lastReportMs = now;

  for (int z = 0; z < NUM_ZONES; z++) {
    int lvl = 0;

    for (int m = 0; m < MOTORS_PRESENT; m++)
      if (MOTOR_ZONE[m] == z && motorLevel[m] > lvl)
        lvl = motorLevel[m];

    if (abs(lvl - lastSentZone[z]) < REPORT_STEP) continue;
    lastSentZone[z] = lvl;

    uint8_t out[4] = {
      MY_BED_ID,
      RPT_LEVEL,
      (uint8_t)z,
      (uint8_t)lvl
    };

    Serial2.write(out, 4);
    Serial2.flush();
    txFrames++;
    showFeedbackTx();

    Serial.printf("[RS485 TX] bed=%u rpt=%u zone=%u level=%u\n",
                  out[0], out[1], out[2], out[3]);
  }
}

// ============================================================
// Persistence
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
  if (prefs.isKey("levels"))
    prefs.getBytes("levels", saved, sizeof(saved));
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
    if (saved[m] > 0) {
      setMotor(m, saved[m]);
      any = true;
    }
  }

  if (any) Serial.println("Resuming the levels that were running at power-off.");
  else     Serial.println("Nothing to resume - starting idle.");
}

// ============================================================
// Pattern engine
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
      for (int z = 0; z < NUM_ZONES; z++)
        tgtZone(z, z == seq[phase] ? L : BASE);
      phase = (phase + 1) % 6;
      break;
    }

    case PAT_DIAGONAL:
      if (now < stepAtMs) return;
      stepAtMs = now + 2500;
      tgtZone(1, BASE);
      tgtZone(2, BASE);
      if (phase == 0) {
        tgtMotor(HEAD_L, L); tgtMotor(HEAD_R, FLOOR_LEVEL);
        tgtMotor(LEG_R, L);  tgtMotor(LEG_L, FLOOR_LEVEL);
      } else {
        tgtMotor(HEAD_R, L); tgtMotor(HEAD_L, FLOOR_LEVEL);
        tgtMotor(LEG_L, L);  tgtMotor(LEG_R, FLOOR_LEVEL);
      }
      phase ^= 1;
      break;

    case PAT_SIDE:
      if (now < stepAtMs) return;
      stepAtMs = now + 2500;
      tgtZone(1, BASE);
      tgtZone(2, BASE);
      if (phase == 0) {
        tgtMotor(HEAD_L, L); tgtMotor(LEG_L, L);
        tgtMotor(HEAD_R, FLOOR_LEVEL); tgtMotor(LEG_R, FLOOR_LEVEL);
      } else {
        tgtMotor(HEAD_R, L); tgtMotor(LEG_R, L);
        tgtMotor(HEAD_L, FLOOR_LEVEL); tgtMotor(LEG_L, FLOOR_LEVEL);
      }
      phase ^= 1;
      break;

    case PAT_CIRCLE: {
      if (now < stepAtMs) return;
      stepAtMs = now + 1200;
      const int ring[4] = {HEAD_L, HEAD_R, LEG_R, LEG_L};
      tgtZone(1, L / 4);
      tgtZone(2, L / 4);
      for (int i = 0; i < 4; i++)
        tgtMotor(ring[i], i == phase ? L : FLOOR_LEVEL);
      phase = (phase + 1) % 4;
      break;
    }

    case PAT_KNEAD:
      if (now < stepAtMs) return;
      stepAtMs = now + 1400;
      tgtZone(0, L / 6);
      tgtZone(3, L / 6);
      tgtMotor(BACK_U, phase == 0 ? L : FLOOR_LEVEL);
      tgtMotor(BACK_L, phase == 0 ? FLOOR_LEVEL : L);
      phase ^= 1;
      break;

    case PAT_PULSE: {
      if (now < stepAtMs) return;
      const unsigned long dur[4] = {260, 260, 260, 1800};
      stepAtMs = now + dur[phase];
      tgtZone(0, FLOOR_LEVEL);
      tgtZone(3, FLOOR_LEVEL);
      bool on = (phase == 0 || phase == 2);
      tgtMotor(BACK_U, on ? L : FLOOR_LEVEL);
      tgtMotor(BACK_L, on ? L : FLOOR_LEVEL);
      phase = (phase + 1) % 4;
      break;
    }

    case PAT_BREATHE: {
      const unsigned long T_IN = 4000;
      const unsigned long T_HOLD = 800;
      const unsigned long T_OUT = 6000;
      const unsigned long T_REST = 1200;
      const unsigned long cycle = T_IN + T_HOLD + T_OUT + T_REST;
      unsigned long t = (now - patStartMs) % cycle;
      int lvl;

      if      (t < T_IN) lvl = (int)((long)L * t / T_IN);
      else if (t < T_IN + T_HOLD) lvl = L;
      else if (t < T_IN + T_HOLD + T_OUT)
        lvl = (int)((long)L * (T_IN + T_HOLD + T_OUT - t) / T_OUT);
      else lvl = 0;

      tgtAll(max(lvl, FLOOR_LEVEL));
      break;
    }

    case PAT_RAIN:
      if (now < stepAtMs) return;
      if (phase == 0) {
        int m = random(0, MOTORS_PRESENT);
        tgtAll(FLOOR_LEVEL);
        tgtMotor(m, max(30, L * 2 / 3));
        stepAtMs = now + random(300, 700);
        phase = 1;
      } else {
        tgtAll(FLOOR_LEVEL);
        stepAtMs = now + random(400, 1600);
        phase = 0;
      }
      break;

    case PAT_SHUFFLE: {
      if (now < stepAtMs) return;
      stepAtMs = now + random(1500, 4000);
      int z = random(0, NUM_ZONES);
      int lvl = random(max(30, L / 2), L + 1);
      for (int i = 0; i < NUM_ZONES; i++)
        tgtZone(i, i == z ? lvl : FLOOR_LEVEL);
      break;
    }

    default:
      break;
  }
}

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
// Console
// ============================================================
String inBuf;

void runCalib(int m) {
  if (m < 0 || m >= MOTORS_PRESENT) {
    Serial.println("bad motor #");
    return;
  }

  Serial.printf("Calibrating motor %d (%s, %s). Ramping duty slowly.\n",
                m, ZONE_NAME[MOTOR_ZONE[m]], MOTOR_BIG[m] ? "BIG" : "small");
  Serial.println("Watch the motor. Press ENTER the moment it starts.");

  while (Serial.available()) Serial.read();

  for (int duty = 30; duty <= 255; duty += 5) {
    pwmWrite(m, duty);
    Serial.printf("  duty = %d  (%.0f%%)\n", duty, duty * 100.0 / 255);

    unsigned long t0 = millis();

    while (millis() - t0 < 700) {
      updateDiagnostics();

      if (Serial.available()) {
        while (Serial.available()) Serial.read();
        pwmWrite(m, 0);
        Serial.printf("\nStart threshold: duty %d\n", duty);
        Serial.printf("Set %s to about %d (threshold plus margin).\n",
                      MOTOR_BIG[m] ? "MIN_DUTY_BIG" : "MIN_DUTY_SMALL",
                      duty + 8);
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
  Serial.println("---- RS485 LINK ----");
  Serial.printf(" bed id          : %u\n", MY_BED_ID);
  Serial.printf(" uart            : RX=%d TX=%d @115200\n", UART_RX, UART_TX);
  Serial.printf(" NeoPixel DATA   : GPIO%d, %d LEDs\n", NEO_PIN, NEO_COUNT);
  Serial.printf(" RX bytes        : %ld\n", rxBytesSeen);
  Serial.printf(" A5 valid frames : %ld\n", a5FramesSeen);
  Serial.printf("   TIME 0x10     : %ld\n", timeFramesSeen);
  Serial.printf("   ACK  0x11     : %ld\n", ackFramesSeen);
  Serial.printf(" legacy 4-byte   : %ld\n", legacyFramesSeen);
  Serial.printf(" checksum faults : %ld\n", checksumFaults);
  Serial.printf(" TX frames       : %ld\n", txFrames);

  if (lastFrameLen > 0) {
    Serial.printf(" last RX         : len=%d (%lu ms ago) :", lastFrameLen, millis() - lastFrameMs);
    for (int i = 0; i < lastFrameLen; i++) Serial.printf(" %02X", lastFrame[i]);
    Serial.println();
  } else {
    Serial.println(" last RX         : none yet");
  }

  Serial.println("--------------------");
}

void handleLine(String line) {
  line.trim();
  line.toLowerCase();

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
  else if (cmd == "ledtest") {
    Serial.println("NeoPixel startup test.");
    startupLedTest();
  }
  else if (cmd == "demo") {
    demoActive = true;
    Serial.println("Demo starting - press ENTER to stop.");
  }
  else if (cmd == "list") {
    for (int p = 0; p < PAT_COUNT; p++)
      Serial.printf("  %2d  %s\n", p, PATTERN_NAME[p]);
  }
  else {
    Serial.printf("Unknown command: \"%s\" (%d chars)\n",
                  line.c_str(), line.length());
    Serial.println("motor M N | zone Z N | all N | preset P [N] | timer MIN | off");
    Serial.println("demo | calib M | status | link | ledtest | list");
  }
}

// ============================================================
// Demo
// ============================================================
void demoSet(int m, int level) {
  if (m < 0 || m >= MOTORS_PRESENT) return;
  motorLevel[m] = constrain(level, 0, 100);
  tgtLevel[m] = motorLevel[m];
  pwmWrite(m, levelToDuty(m, motorLevel[m]));
}

void stopDemoToManual() {
  demoActive = false;

  for (int m = 0; m < MOTORS_PRESENT; m++) {
    motorLevel[m] = 0;
    tgtLevel[m] = 0;
    pwmWrite(m, 0);
  }

  Serial.println();
  Serial.println("Demo stopped.");
}

bool demoWait(unsigned long ms) {
  unsigned long t0 = millis();

  while (millis() - t0 < ms) {
    pollUart();
    updateDiagnostics();

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
  Serial.printf("=== BED BOX SHEMI - TEST %03d - bed #%d - %d motors ===\n",
                TEST_NUMBER, MY_BED_ID, MOTORS_PRESENT);
  Serial.printf("LEDC mode: %s\n",
                LEDC_BY_PIN ? "core 3.x, by pin" : "core 2.x, by channel");
  Serial.println("Self-test running. Press any key to stop and get the console.");

  for (int m = 0; m < MOTORS_PRESENT; m++) {
    Serial.printf("[DEMO] motor %d %s %s ramp up and down\n",
                  m, ZONE_NAME[MOTOR_ZONE[m]],
                  MOTOR_BIG[m] ? "BIG" : "small");

    if (demoRamp(m, 0, 100, 2500)) return;
    if (demoWait(600)) return;
    if (demoRamp(m, 100, 0, 1800)) return;

    demoSet(m, 0);

    if (demoWait(400)) return;
  }

  Serial.println("[DEMO] all together");

  for (int i = 0; i <= 40; i++) {
    if (!demoActive) return;
    for (int m = 0; m < MOTORS_PRESENT; m++)
      demoSet(m, 100 * i / 40);
    if (demoWait(70)) return;
  }

  if (demoWait(900)) return;

  for (int i = 40; i >= 0; i--) {
    if (!demoActive) return;
    for (int m = 0; m < MOTORS_PRESENT; m++)
      demoSet(m, 100 * i / 40);
    if (demoWait(55)) return;
  }

  for (int m = 0; m < MOTORS_PRESENT; m++)
    demoSet(m, 0);

  Serial.println("[DEMO] pass complete, looping");

  if (demoWait(1200)) return;
}

void printAliveHeartbeat() {
  unsigned long now = millis();
  if ((long)(now - nextAlivePrint) < 0) return;
  nextAlivePrint = now + 1000;

  Serial.printf("[ALIVE] BEDBOX SHEMI TEST%03d ms=%lu RXbytes=%ld A5=%ld TIME=%ld ACK=%ld LEGACY=%ld CRCbad=%ld TX=%ld\n",
                TEST_NUMBER, now, rxBytesSeen, a5FramesSeen, timeFramesSeen,
                ackFramesSeen, legacyFramesSeen, checksumFaults, txFrames);
}

// ============================================================
// Setup / loop
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  delay(300);

  pixels.begin();
  pixels.setBrightness(NEO_BRIGHTNESS);

  C_RED    = pixels.Color(255, 0, 0, 0);
  C_GREEN  = pixels.Color(0, 180, 0, 0);
  C_YELLOW = pixels.Color(255, 170, 0, 0);
  C_BLUE   = pixels.Color(0, 80, 255, 0);
  C_WHITE  = pixels.Color(180, 180, 180, 0);
  C_OFF    = pixels.Color(0, 0, 0, 0);

  startupLedTest();
  ledStepAt = millis();
  nextAlivePrint = millis();

  for (int z = 0; z < NUM_ZONES; z++)
    lastSentZone[z] = -99;

  for (int m = 0; m < NUM_MOTORS; m++) {
    motorLevel[m] = 0;
    tgtLevel[m] = 0;

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
  Serial.printf("=== BED BOX SHEMI - TEST %03d - bed #%d - %d motors ===\n",
                TEST_NUMBER, MY_BED_ID, MOTORS_PRESENT);
  Serial.printf("RS485 UART: RX=%d TX=%d @115200\n", UART_RX, UART_TX);
  Serial.printf("NeoPixels: %d LEDs on GPIO%d\n", NEO_COUNT, NEO_PIN);
  Serial.println("LED code: GREEN 1-pixel=bus alive, RED 1-pixel=bus silent.");
  Serial.println("          BLUE 3-train -> command Panel->BedBox, YELLOW 3-train <- feedback.");
  Serial.println("          RED all-flash=bad checksum.");
  Serial.printf("Calibration: MIN_DUTY_SMALL=%d MIN_DUTY_BIG=%d\n",
                MIN_DUTY_SMALL, MIN_DUTY_BIG);
  Serial.printf("LEDC mode: %s\n",
                LEDC_BY_PIN ? "core 3.x, by pin" : "core 2.x, by channel");
  Serial.println("Type list for patterns, link for RS485 diagnostics, ledtest for LEDs.");

  loadState();

#if BOOT_SELFTEST
  demoActive = true;
  Serial.println("BOOT_SELFTEST is on - sweeping the motors now.");
#endif
}

void loop() {
  updateDiagnostics();
  printAliveHeartbeat();

  if (demoActive) {
    runDemoPass();
    return;
  }

  pollUart();

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (inBuf.length()) {
        handleLine(inBuf);
        inBuf = "";
      }
    } else {
      inBuf += c;
    }
  }

  if (timerEndMs && millis() > timerEndMs) {
    timerEndMs = 0;
    activePreset = -1;

    for (int m = 0; m < MOTORS_PRESENT; m++) {
      tgtLevel[m] = 0;
      setMotor(m, 0);
    }

    Serial.println("Session timer finished - motors stopped, settings kept.");
  }

  tickEngine();
  reportLevels();
}

// ============================================================
// BED BOX SHEMI - TEST 024 - end of file
// RS485 RX=16 TX=17, NeoPixel DATA=23 via external level shifter, 7 LEDs
// ============================================================
