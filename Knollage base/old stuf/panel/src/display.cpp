#include "display.h"
#include "config.h"
#include <Arduino_GFX_Library.h>
#include <bb_captouch.h>

// ---------------------------------------------------------------------------
//  RGB panel wiring for the Waveshare ESP32-S3-Touch-LCD-4.3
//  (community-verified pin map; matches the earlier Arduino_GFX config)
// ---------------------------------------------------------------------------
static Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    5 /*DE*/, 3 /*VSYNC*/, 46 /*HSYNC*/, 7 /*PCLK*/,
    1, 2, 42, 41, 40,                    // R0..R4
    39, 0, 45, 48, 47, 21,               // G0..G5
    14, 38, 18, 17, 10,                  // B0..B4
    0, 40, 48, 88,                       // hsync pol/front/pulse/back
    0, 13, 3, 32,                        // vsync pol/front/pulse/back
    1, 16000000);
static Arduino_RGB_Display *gfx =
    new Arduino_RGB_Display(800, 480, rgbpanel, 0, true);

static BBCapTouch touch;

// ---------------------------------------------------------------------------
//  LVGL plumbing
// ---------------------------------------------------------------------------
static lv_disp_draw_buf_t drawBuf;
static lv_color_t *buf1 = nullptr;
static lv_obj_t *dimLayer = nullptr;   // content-dimming overlay (no-mod path)

static void flushCb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *px) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px, w, h);
  lv_disp_flush_ready(disp);
}

static void touchCb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  TOUCHINFO ti;
  if (touch.getSamples(&ti) && ti.count > 0) {
    int x = ti.x[0];
    int y = ti.y[0];
    // If touch is mirrored or swapped on your unit, fix it HERE:
    //   x = 799 - x;   y = 479 - y;   or swap x/y.
    data->point.x = constrain(x, 0, 799);
    data->point.y = constrain(y, 0, 479);
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

namespace display {

void begin() {
  gfx->begin();
  gfx->fillScreen(BLACK);

  touch.init(PIN_TOUCH_SDA, PIN_TOUCH_SCL, -1, -1);

  lv_init();

  // Draw buffer in PSRAM: 800 x 60 lines double-buffered is a good balance.
  size_t bufPx = 800 * 60;
  buf1 = (lv_color_t *)heap_caps_malloc(bufPx * sizeof(lv_color_t),
                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&drawBuf, buf1, nullptr, bufPx);

  static lv_disp_drv_t dispDrv;
  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = 800;
  dispDrv.ver_res = 480;
  dispDrv.flush_cb = flushCb;
  dispDrv.draw_buf = &drawBuf;
  lv_disp_drv_register(&dispDrv);

  static lv_indev_drv_t indevDrv;
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = touchCb;
  lv_indev_drv_register(&indevDrv);

  // Dark theme baseline for a bedroom device
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x14161f), 0);

  // Content-dim overlay: a full-screen black rect on the system layer whose
  // opacity we vary. Click-through so touches reach the UI below.
  dimLayer = lv_obj_create(lv_layer_sys());
  lv_obj_remove_style_all(dimLayer);
  lv_obj_set_size(dimLayer, 800, 480);
  lv_obj_set_style_bg_color(dimLayer, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(dimLayer, LV_OPA_TRANSP, 0);
  lv_obj_add_flag(dimLayer, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_clear_flag(dimLayer, LV_OBJ_FLAG_CLICKABLE);

#if ENABLE_BACKLIGHT_PWM
  ledcAttach(PIN_BACKLIGHT, BACKLIGHT_PWM_HZ, 8);
  ledcWrite(PIN_BACKLIGHT, 255);
#endif
}

void loop() {
  lv_timer_handler();
}

void setBrightness(uint8_t level) {
#if ENABLE_BACKLIGHT_PWM
  ledcWrite(PIN_BACKLIGHT, level);
  lv_obj_set_style_bg_opa(dimLayer, LV_OPA_TRANSP, 0);
#else
  // No hardware dimming: fade a black overlay over the content instead.
  // level 255 -> fully clear; level 0 -> nearly black (leave a floor so the
  // screen never looks dead).
  uint8_t opa = map(level, 0, 255, 200, 0);
  lv_obj_set_style_bg_opa(dimLayer, opa, 0);
#endif
}

} // namespace display
