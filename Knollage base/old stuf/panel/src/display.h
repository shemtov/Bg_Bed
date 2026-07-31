// =============================================================================
//  display.cpp / display.h  -  RGB panel + LVGL glue + GT911 touch
//  The one hardware-specific file: if anything looks wrong on first boot
//  (colors, mirrored touch), this is where to fix it.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <lvgl.h>

namespace display {
  // Bring up the panel, LVGL, and touch. Call once from setup().
  void begin();

  // Call every loop() iteration; pumps LVGL.
  void loop();

  // 0..255 visual brightness. Without the backlight mod this dims the CONTENT
  // (a black overlay); with ENABLE_BACKLIGHT_PWM it drives the real backlight.
  void setBrightness(uint8_t level);
}
