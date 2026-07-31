#include "comfort.h"
#include "config.h"
#include "display.h"
#include "massage_ui.h"
#include <Arduino.h>
#include <time.h>

// ===========================================================================
//  autobright
// ===========================================================================
namespace autobright {

static uint32_t acc = 0;
static int nSamples = 0;
static unsigned long lastSample = 0;
static uint8_t current = 255;      // current brightness (for slow glide)

void begin() {
#if ENABLE_AUTOBRIGHT
  pinMode(PIN_LDR, INPUT);
  analogReadResolution(12);
#endif
}

void loop() {
#if ENABLE_AUTOBRIGHT
  unsigned long now = millis();
  if (now - lastSample < BRIGHT_SAMPLE_MS) return;
  lastSample = now;

  acc += analogRead(PIN_LDR);
  if (++nSamples < BRIGHT_AVG_WINDOW) return;

  int avg = acc / nSamples;          // 0..4095, higher = brighter room
  acc = 0; nSamples = 0;

  // Map the averaged room light to a target brightness. The floor (40)
  // keeps the screen readable even in a pitch-dark room.
  uint8_t target = map(constrain(avg, 100, 3500), 100, 3500, 40, 255);

  // Night hours override: force dim regardless of room light (a lamp
  // switched on at 2am shouldn't blast the screen).
#if ENABLE_WIFI
  struct tm ti;
  if (getLocalTime(&ti, 10)) {
    int h = ti.tm_hour;
    bool night = (h >= BRIGHT_NIGHT_START || h < BRIGHT_NIGHT_END);
    if (night && target > 90) target = 90;
  }
#endif

  // Glide gently toward the target - never jump (no twitching screen).
  if (target > current) current = min<int>(current + 8, target);
  else if (target < current) current = max<int>(current - 8, target);
  display::setBrightness(current);
#endif
}

} // namespace autobright

// ===========================================================================
//  bedselect  -  "last press wins" on the two sensed light-button lines
// ===========================================================================
namespace bedselect {

static int last1 = -1, last2 = -1;
static unsigned long debounce1 = 0, debounce2 = 0;

void begin() {
#if ENABLE_LIGHT_BUTTONS
  pinMode(PIN_BEDBTN_1, INPUT);    // add INPUT_PULLDOWN if your sense circuit
  pinMode(PIN_BEDBTN_2, INPUT);    // floats when the light is off
  last1 = digitalRead(PIN_BEDBTN_1);
  last2 = digitalRead(PIN_BEDBTN_2);
#endif
}

void loop() {
#if ENABLE_LIGHT_BUTTONS
  unsigned long now = millis();

  int v1 = digitalRead(PIN_BEDBTN_1);
  if (v1 != last1 && now - debounce1 > 150) {   // ANY change = a press
    debounce1 = now;
    last1 = v1;
    massage_ui::selectBed(1);
  }
  int v2 = digitalRead(PIN_BEDBTN_2);
  if (v2 != last2 && now - debounce2 > 150) {
    debounce2 = now;
    last2 = v2;
    massage_ui::selectBed(2);
  }
#endif
}

} // namespace bedselect
