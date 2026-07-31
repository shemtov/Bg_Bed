// =============================================================================
//  massage_ui  -  the Massage tab: bed pills, mode row (All / Zones / Motors /
//  Presets), the per-mode content area, the timer slider, and STOP.
// =============================================================================
#pragma once
#include <lvgl.h>

namespace massage_ui {
  // Build the massage UI inside the given parent (a tab page).
  void build(lv_obj_t *parent);

  // Which bed the panel currently controls (1..NUM_BEDS).
  int  selectedBed();
  void selectBed(int bed);   // also used by the light-button module
}
