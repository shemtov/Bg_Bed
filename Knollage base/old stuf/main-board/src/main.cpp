// =============================================================================
//  TEST NUMBER - bumped on EVERY change, no exceptions. Printed at boot.
// =============================================================================
#define TEST_NUMBER 1

/*
 * =============================================================================
 *  MAIN MOTOR BOARD  (PlatformIO / VS Code)  -  classic ESP32 under the bed
 * =============================================================================
 *  Controls 10 vibration motors via 5x L298N. Receives ESP-NOW commands that
 *  can address: ALL motors / a ZONE / a single MOTOR / a PRESET pattern, plus
 *  a session timer. Remembers the last state in flash and resumes it on power-on.
 *
 *  Zones (2 motors each):
 *    0 Shoulders {0,1}  1 UpperBack {2,3}  2 LowerBack {4,5}
 *    3 Thighs {6,7}     4 Calves {8,9}
 *
 *  Command protocol (4-byte message, must match the remote):
 *    CMD_OFF   0                         all motors off
 *    CMD_ALL   1  value                  all motors = value(0..100)
 *    CMD_ZONE  2  target=zone  value     both motors of a zone = value
 *    CMD_MOTOR 3  target=motor value     one motor = value
 *    CMD_PRESET4  target=presetId        run a pattern (0 wave,1 pulse,2 ripple)
 *    CMD_TIMER 5  value=minutes          session length (5..120)
 *
 *  >>> FOR BED 2: set BED_ID to 2. <<<
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>

#define BED_ID      1
#define RF_CHANNEL  1

const int NUM_MOTORS = 10;
const int motorPins[NUM_MOTORS] = { 13, 14, 18, 19, 21, 22, 23, 25, 26, 27 };
const int PWM_FREQ = 20000;
const int PWM_RES  = 8;

const int NUM_ZONES = 5;
const int zoneMotors[NUM_ZONES][2] = { {0,1}, {2,3}, {4,5}, {6,7}, {8,9} };

enum { CMD_OFF=0, CMD_ALL=1, CMD_ZONE=2, CMD_MOTOR=3, CMD_PRESET=4, CMD_TIMER=5 };

// ---- persisted / runtime state ---------------------------------------------
uint8_t level[NUM_MOTORS] = { 0 };   // manual per-motor levels 0..100
uint8_t activePreset = 255;          // 255 = none
uint8_t sDuration = 10;              // session minutes
bool    outputActive = true;         // false after the session timer expires
Preferences prefs;

unsigned long sessionStart = 0;
bool sessionRunning = false;
unsigned long sessionMs() { return (unsigned long)sDuration * 60000UL; }

unsigned long presetLast = 0;
int presetStep = 0;

// ---- message ---------------------------------------------------------------
typedef struct __attribute__((packed)) {
  uint8_t bedId; uint8_t cmd; uint8_t target; uint8_t value;
} Msg;
volatile bool haveMsg = false;
Msg inbox;

// ---- PWM (classic LEDC API: channel == motor index) ------------------------
void outMotor(int i, int pct) {              // drive only, don't store
  pct = constrain(pct, 0, 100);
  ledcWrite(i, (pct * 255) / 100);
}
void writeMotor(int i, int pct) {            // store + drive
  pct = constrain(pct, 0, 100);
  level[i] = pct;
  outMotor(i, pct);
}
void applyManual() { for (int i = 0; i < NUM_MOTORS; i++) outMotor(i, level[i]); }
void allOutputsOff() { for (int i = 0; i < NUM_MOTORS; i++) outMotor(i, 0); }

// ---- persistence -----------------------------------------------------------
void saveState() {
  prefs.putBytes("levels", level, NUM_MOTORS);
  prefs.putUChar("preset", activePreset);
  prefs.putUChar("dur", sDuration);
}

void beginSession() { sessionRunning = true; outputActive = true; sessionStart = millis(); }

// ---- command handling ------------------------------------------------------
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len == sizeof(Msg)) { memcpy(&inbox, data, sizeof(Msg)); haveMsg = true; }
}

void handleMsg() {
  if (inbox.bedId != BED_ID) return;
  switch (inbox.cmd) {
    case CMD_OFF:
      activePreset = 255;
      for (int i = 0; i < NUM_MOTORS; i++) writeMotor(i, 0);
      sessionRunning = false; outputActive = true;
      saveState();
      break;

    case CMD_ALL:
      activePreset = 255;
      for (int i = 0; i < NUM_MOTORS; i++) writeMotor(i, inbox.value);
      beginSession(); saveState();
      break;

    case CMD_ZONE:
      if (inbox.target < NUM_ZONES) {
        activePreset = 255;
        writeMotor(zoneMotors[inbox.target][0], inbox.value);
        writeMotor(zoneMotors[inbox.target][1], inbox.value);
        beginSession(); saveState();
      }
      break;

    case CMD_MOTOR:
      if (inbox.target < NUM_MOTORS) {
        activePreset = 255;
        writeMotor(inbox.target, inbox.value);
        beginSession(); saveState();
      }
      break;

    case CMD_PRESET:
      activePreset = inbox.target;
      presetStep = 0; presetLast = 0;
      beginSession(); saveState();
      break;

    case CMD_TIMER: {
      uint8_t mins = constrain(inbox.value, 5, 120);
      if (mins != sDuration) { sDuration = mins; prefs.putUChar("dur", sDuration); }
      if (sessionRunning) sessionStart = millis();
      break;
    }
  }
}

// ---- preset patterns -------------------------------------------------------
void runPreset() {
  unsigned long now = millis();
  if (activePreset == 0) {                      // WAVE - moving bump
    if (now - presetLast < 600) return; presetLast = now;
    for (int i = 0; i < NUM_MOTORS; i++) {
      int d = abs(i - (presetStep % NUM_MOTORS));
      outMotor(i, d == 0 ? 80 : d == 1 ? 45 : 15);
    }
    presetStep++;
  } else if (activePreset == 1) {               // PULSE - all on/off together
    if (now - presetLast < 500) return; presetLast = now;
    int v = (presetStep % 2) ? 70 : 0;
    for (int i = 0; i < NUM_MOTORS; i++) outMotor(i, v);
    presetStep++;
  } else if (activePreset == 2) {               // RIPPLE - alternate odd/even
    if (now - presetLast < 450) return; presetLast = now;
    for (int i = 0; i < NUM_MOTORS; i++)
      outMotor(i, ((i + presetStep) % 2 == 0) ? 70 : 10);
    presetStep++;
  }
}

void runSessionTimer() {
  if (!sessionRunning) return;
  if (millis() - sessionStart >= sessionMs()) {
    outputActive = false;
    allOutputsOff();                 // motors quiet; level[]/preset kept in memory
    sessionRunning = false;
    Serial.println("Session ended - motors stopped, setting remembered.");
  }
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.printf("\n[bed] booting - TEST %d\n", TEST_NUMBER);

  for (int i = 0; i < NUM_MOTORS; i++) {
    ledcSetup(i, PWM_FREQ, PWM_RES);
    ledcAttachPin(motorPins[i], i);
    ledcWrite(i, 0);
  }

  prefs.begin("massage", false);
  size_t got = prefs.getBytes("levels", level, NUM_MOTORS);
  if (got != NUM_MOTORS) for (int i = 0; i < NUM_MOTORS; i++) level[i] = 0;
  activePreset = prefs.getUChar("preset", 255);
  sDuration    = prefs.getUChar("dur", 10);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(RF_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed"); delay(1000); ESP.restart(); }
  esp_now_register_recv_cb(onRecv);

  // resume last massage
  outputActive = true;
  if (activePreset == 255) applyManual();
  beginSession();
  Serial.printf("Bed %d ready. preset=%d timer=%d min\n", BED_ID, activePreset, sDuration);
}

void loop() {
  if (haveMsg) { haveMsg = false; handleMsg(); }
  if (outputActive && activePreset != 255) runPreset();
  runSessionTimer();
}
