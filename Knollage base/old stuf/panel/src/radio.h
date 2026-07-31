// =============================================================================
//  radio  -  the Radio tab: presets (color-coded by genre), genre browse via
//  the Radio Browser directory, volume, play/pause, sleep timer, and the
//  I2S audio pipeline (PCM5102A DAC -> TPA3116 amp -> speakers).
// =============================================================================
#pragma once
#include <lvgl.h>

namespace radio {
  void build(lv_obj_t *parent);   // build the Radio tab UI
  void loop();                    // audio pump + sleep timer; call every loop()
}
