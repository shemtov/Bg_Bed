// ============================================================
// PANEL SHEMI - TEST 001  (7B bring-up)
// ============================================================
//
// Board: Waveshare ESP32-S3-Touch-LCD-7B, 1024 x 600
//
// WHY TEST 001 AND NOT 082
//   This is a different board. The Guition ESP32-8048S043 that ran
//   TEST 081 was damaged while soldering. The 7B has a different
//   resolution, a different RGB pin map, RS485 and backlight control
//   onboard. The display layer is new, so the count starts again.
//   TEST 081 stays in the repo untouched as the reference for the UI
//   that has to be ported afterwards.
//
// THIS SKETCH PROVES HARDWARE, NOT UI
//   Screen, touch, backlight, LVGL. No tiles, no clock, no bus.
//   Port the interface only once these four are confirmed.
//
// BEFORE IT WILL BUILD
//   Copy from the Waveshare demo, PlatformIO/13_LVGL_TRANSPLANT:
//     lib/     the whole folder
//     boards/  ESP32-S3-Touch-LCD-7B.json
//   Their port layer is used as-is. It carries the panel timings and
//   the GT911 setup, both proven on this board.
//
// WHAT IS ALREADY KNOWN AND CARRIES OVER FROM panel_ira
//   The touch controller is the same GT911. So: five touch points,
//   not ten. Wire.endTransmission(false) for repeated-start reads.
//   bb_captouch is unusable - the IO expander answers on 0x38 and is
//   mistaken for a FocalTech part. None of that is re-litigated here
//   because the vendor's own touch driver is used.
//
// THE BACKLIGHT DEBT IS PAID
//   On panel_ira real dimming needed a hand-soldered wire to the
//   MP3302 EN pin and was deferred. Here the IO expander at 0x24 has
//   a PWM register at 0x05, so brightness is one I2C write.
//   IO_EXTENSION_Pwm_Output(0..100).
//
// EXPECT IT TO BE SLOWER THAN THE 4.3 INCH PANEL
//   1024 x 600 is 60 percent more pixels than 800 x 480. Waveshare
//   themselves measure about 17 fps on the LVGL benchmark at a 30 MHz
//   pixel clock. Judge the UI against that, not against the old panel.
//
// NOT YET RUN ON HARDWARE.
//
// ============================================================

#include <Arduino.h>
#include "lvgl_port.h"
#include "rgb_lcd_port.h"
#include "io_extension.h"

#define TEST_NUMBER 1

// ------------------------------------------------------------

static lv_obj_t *lblTouch  = NULL;
static lv_obj_t *lblCount  = NULL;
static lv_obj_t *lblBright = NULL;

static uint32_t touchCount = 0;

// ------------------------------------------------------------
// Anything touching an LVGL object must hold the port mutex. The
// LVGL task runs on core 1 and the API is not thread safe.
// ------------------------------------------------------------

static void onScreenPressed(lv_event_t *e) {
  lv_point_t p;
  lv_indev_t *indev = lv_indev_get_act();
  if (!indev) return;
  lv_indev_get_point(indev, &p);

  touchCount++;
  lv_label_set_text_fmt(lblTouch, "x %d    y %d", (int)p.x, (int)p.y);
  lv_label_set_text_fmt(lblCount, "touches: %lu", (unsigned long)touchCount);

  Serial.printf("touch %lu  at %d,%d\n",
                (unsigned long)touchCount, (int)p.x, (int)p.y);
}

static void onBrightness(lv_event_t *e) {
  lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
  int v = lv_slider_get_value(sl);

  // Straight through the IO expander. No GPIO, no wire mod.
  IO_EXTENSION_Pwm_Output(v);

  lv_label_set_text_fmt(lblBright, "backlight: %d %%", v);
  Serial.printf("backlight %d %%\n", v);
}

// ------------------------------------------------------------

static void buildScreen() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x14161F), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "PANEL SHEMI  -  TEST 001");
  lv_obj_set_style_text_color(title, lv_color_hex(0xE8E8E8), LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

  lv_obj_t *sub = lv_label_create(scr);
  lv_label_set_text_fmt(sub, "Waveshare 7B   %d x %d",
                        LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_set_style_text_color(sub, lv_color_hex(0x8A8A8A), LV_PART_MAIN);
  lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 56);

  // Corner markers. If any is off-screen or clipped, the resolution
  // or the rotation is wrong - a fault that is easy to miss on a
  // screen that otherwise looks fine.
  const int m = 6;
  const lv_align_t corner[4] = {
    LV_ALIGN_TOP_LEFT, LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_RIGHT
  };
  for (int i = 0; i < 4; i++) {
    lv_obj_t *box = lv_obj_create(scr);
    lv_obj_set_size(box, 40, 40);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x3FA96B), LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(box, 4, LV_PART_MAIN);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(box, corner[i], (i & 1) ? -m : m, (i & 2) ? -m : m);
  }

  lblTouch = lv_label_create(scr);
  lv_label_set_text(lblTouch, "touch anywhere");
  lv_obj_set_style_text_color(lblTouch, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(lblTouch, LV_ALIGN_CENTER, 0, -40);

  lblCount = lv_label_create(scr);
  lv_label_set_text(lblCount, "touches: 0");
  lv_obj_set_style_text_color(lblCount, lv_color_hex(0x8A8A8A), LV_PART_MAIN);
  lv_obj_align(lblCount, LV_ALIGN_CENTER, 0, -10);

  lblBright = lv_label_create(scr);
  lv_label_set_text(lblBright, "backlight: 100 %");
  lv_obj_set_style_text_color(lblBright, lv_color_hex(0x8A8A8A), LV_PART_MAIN);
  lv_obj_align(lblBright, LV_ALIGN_CENTER, 0, 40);

  lv_obj_t *sl = lv_slider_create(scr);
  lv_obj_set_width(sl, 520);
  lv_slider_set_range(sl, 5, 100);      // never to zero, or the screen
  lv_slider_set_value(sl, 100, LV_ANIM_OFF);  // looks dead
  lv_obj_align(sl, LV_ALIGN_CENTER, 0, 80);
  lv_obj_add_event_cb(sl, onBrightness, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_add_event_cb(scr, onScreenPressed, LV_EVENT_PRESSED, NULL);
}

// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("=== PANEL SHEMI - TEST 001 - Waveshare 7B bring-up ===");
  Serial.printf("resolution %d x %d\n", LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  Serial.printf("psram: %lu bytes free of %lu\n",
                (unsigned long)ESP.getFreePsram(),
                (unsigned long)ESP.getPsramSize());
  if (ESP.getPsramSize() < 4 * 1024 * 1024) {
    Serial.println("*** PSRAM under 4 MB. This board should report 8 MB.");
    Serial.println("*** Two 1024x600 framebuffers will not fit. Check the");
    Serial.println("*** board JSON and the module marking.");
  }

  static esp_lcd_panel_handle_t panel = NULL;
  static esp_lcd_touch_handle_t tp    = NULL;

  // Touch first, exactly as the vendor demo does. The GT911 reset
  // runs through the IO expander, so ordering matters.
  Serial.println("init touch ...");
  tp = touch_gt911_init();
  Serial.println(tp ? "  touch ok" : "  TOUCH FAILED");

  Serial.println("init rgb panel ...");
  panel = waveshare_esp32_s3_rgb_lcd_init();
  Serial.println(panel ? "  panel ok" : "  PANEL FAILED");

  wavesahre_rgb_lcd_bl_on();          // vendor spelling, not a typo here

  Serial.println("init lvgl ...");
  ESP_ERROR_CHECK(lvgl_port_init(panel, tp));

  if (lvgl_port_lock(-1)) {
    buildScreen();
    lvgl_port_unlock();
  }

  IO_EXTENSION_Pwm_Output(100);

  Serial.println();
  Serial.println("Four green corner squares mean the full 1024x600 is");
  Serial.println("addressable. Touch prints coordinates here and on the");
  Serial.println("screen. The slider drives the backlight through the IO");
  Serial.println("expander - no wire mod needed on this board.");
}

void loop() {
  // LVGL runs in its own task on core 1, started by lvgl_port_init.
  // Nothing to pump here.
  delay(1000);
}

// ============================================================
// END OF FILE - PANEL SHEMI - TEST 001 (7B bring-up)
// ============================================================
