#include "massage_ui.h"
#include "config.h"
#include "espnow_link.h"
#include <Preferences.h>

namespace massage_ui {

// ---------------------------------------------------------------------------
//  State
// ---------------------------------------------------------------------------
static int bed = DEFAULT_BED;
static int mode = 1;               // 0 all, 1 zones, 2 motors, 3 presets
static int timerMin = 10;
static Preferences prefs;

static const char *zoneNames[NUM_ZONES] = {
    "Shoulders", "Upper back", "Lower back", "Thighs", "Calves"};
static const char *presetNames[3] = {"Wave", "Pulse", "Ripple"};

// ---------------------------------------------------------------------------
//  Widgets we need to reach later
// ---------------------------------------------------------------------------
static lv_obj_t *bedBtns[NUM_BEDS];
static lv_obj_t *modeBtns[4];
static lv_obj_t *content;          // swapped per mode
static lv_obj_t *timerLabel;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------
static void styleCard(lv_obj_t *o) {
  lv_obj_set_style_bg_color(o, lv_color_hex(0x1b1e2a), 0);
  lv_obj_set_style_border_color(o, lv_color_hex(0x2a2f40), 0);
  lv_obj_set_style_border_width(o, 1, 0);
  lv_obj_set_style_radius(o, 10, 0);
}

static void refreshBedPills() {
  for (int i = 0; i < NUM_BEDS; i++) {
    bool sel = (bed == i + 1);
    lv_obj_set_style_bg_color(bedBtns[i],
        lv_color_hex(sel ? 0x26406b : 0x1b1e2a), 0);
  }
}

static void refreshModeBtns() {
  for (int i = 0; i < 4; i++) {
    bool sel = (mode == i);
    lv_obj_set_style_bg_color(modeBtns[i],
        lv_color_hex(sel ? 0x26406b : 0x1b1e2a), 0);
  }
}

// ---------------------------------------------------------------------------
//  Event handlers
// ---------------------------------------------------------------------------
static void buildContent();   // fwd

static void onBed(lv_event_t *e) {
  selectBed((int)(intptr_t)lv_event_get_user_data(e));
}

static void onMode(lv_event_t *e) {
  mode = (int)(intptr_t)lv_event_get_user_data(e);
  refreshModeBtns();
  buildContent();
}

static void onAllSlider(lv_event_t *e) {
  int v = lv_slider_get_value(lv_event_get_target(e));
  espnow_link::send(bed, espnow_link::ALL, 0, v);
}

static void onZoneSlider(lv_event_t *e) {
  int zone = (int)(intptr_t)lv_event_get_user_data(e);
  int v = lv_slider_get_value(lv_event_get_target(e));
  espnow_link::send(bed, espnow_link::ZONE, zone, v);
}

static void onMotorSlider(lv_event_t *e) {
  int motor = (int)(intptr_t)lv_event_get_user_data(e);
  int v = lv_slider_get_value(lv_event_get_target(e));
  espnow_link::send(bed, espnow_link::MOTOR, motor, v);
}

static void onPreset(lv_event_t *e) {
  int p = (int)(intptr_t)lv_event_get_user_data(e);
  espnow_link::send(bed, espnow_link::PRESET, p, 0);
}

static void onStop(lv_event_t *e) {
  espnow_link::send(bed, espnow_link::OFF, 0, 0);
}

static void onTimer(lv_event_t *e) {
  lv_obj_t *s = lv_event_get_target(e);
  int v = lv_slider_get_value(s);
  v = ((v + TIMER_STEP_MIN / 2) / TIMER_STEP_MIN) * TIMER_STEP_MIN;
  timerMin = constrain(v, TIMER_MIN_MIN, TIMER_MAX_MIN);
  lv_label_set_text_fmt(timerLabel, "%d min", timerMin);
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    espnow_link::send(bed, espnow_link::TIMER, 0, timerMin);
    prefs.putInt("timer", timerMin);
  }
}

// ---------------------------------------------------------------------------
//  Content builders (one per mode)
// ---------------------------------------------------------------------------
static lv_obj_t *makeSliderRow(lv_obj_t *parent, const char *name,
                               lv_event_cb_t cb, int userdata) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, lv_pct(100), 44);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_t *lbl = lv_label_create(row);
  lv_label_set_text(lbl, name);
  lv_obj_set_width(lbl, 150);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xc9cee0), 0);

  lv_obj_t *sl = lv_slider_create(row);
  lv_obj_set_flex_grow(sl, 1);
  lv_obj_set_height(sl, 14);
  lv_slider_set_range(sl, 0, 100);
  lv_obj_add_event_cb(sl, cb, LV_EVENT_VALUE_CHANGED,
                      (void *)(intptr_t)userdata);
  return row;
}

static void buildContent() {
  lv_obj_clean(content);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(content, 6, 0);

  if (mode == 0) {                       // ALL - one big slider
    lv_obj_t *lbl = lv_label_create(content);
    lv_label_set_text(lbl, "All motors");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xc9cee0), 0);
    lv_obj_t *sl = lv_slider_create(content);
    lv_obj_set_size(sl, lv_pct(96), 24);
    lv_slider_set_range(sl, 0, 100);
    lv_obj_add_event_cb(sl, onAllSlider, LV_EVENT_VALUE_CHANGED, nullptr);
  } else if (mode == 1) {                // ZONES - five rows
    for (int z = 0; z < NUM_ZONES; z++)
      makeSliderRow(content, zoneNames[z], onZoneSlider, z);
  } else if (mode == 2) {                // MOTORS - ten compact rows
    static char names[NUM_MOTORS][12];
    for (int m = 0; m < NUM_MOTORS; m++) {
      snprintf(names[m], sizeof(names[m]), "Motor %d", m + 1);
      lv_obj_t *row = makeSliderRow(content, names[m], onMotorSlider, m);
      lv_obj_set_height(row, 30);
    }
  } else {                               // PRESETS - three big buttons
    lv_obj_t *rowc = lv_obj_create(content);
    lv_obj_remove_style_all(rowc);
    lv_obj_set_size(rowc, lv_pct(100), 150);
    lv_obj_set_flex_flow(rowc, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(rowc, 10, 0);
    for (int p = 0; p < 3; p++) {
      lv_obj_t *b = lv_btn_create(rowc);
      lv_obj_set_flex_grow(b, 1);
      lv_obj_set_height(b, 140);
      styleCard(b);
      lv_obj_add_event_cb(b, onPreset, LV_EVENT_CLICKED, (void *)(intptr_t)p);
      lv_obj_t *l = lv_label_create(b);
      lv_label_set_text(l, presetNames[p]);
      lv_obj_center(l);
    }
  }
}

// ---------------------------------------------------------------------------
//  Public
// ---------------------------------------------------------------------------
int selectedBed() { return bed; }

void selectBed(int b) {
  bed = constrain(b, 1, NUM_BEDS);
  refreshBedPills();
  // tell the newly-selected bed the panel's timer value so they agree
  espnow_link::send(bed, espnow_link::TIMER, 0, timerMin);
}

void build(lv_obj_t *parent) {
  prefs.begin("panel", false);
  timerMin = constrain(prefs.getInt("timer", 10), TIMER_MIN_MIN, TIMER_MAX_MIN);

  lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(parent, 8, 0);
  lv_obj_set_style_pad_row(parent, 8, 0);

  // ---- top row: bed pills ----
  lv_obj_t *top = lv_obj_create(parent);
  lv_obj_remove_style_all(top);
  lv_obj_set_size(top, lv_pct(100), 44);
  lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(top, 8, 0);
  for (int i = 0; i < NUM_BEDS; i++) {
    lv_obj_t *b = lv_btn_create(top);
    lv_obj_set_size(b, 110, 40);
    styleCard(b);
    bedBtns[i] = b;
    lv_obj_add_event_cb(b, onBed, LV_EVENT_CLICKED, (void *)(intptr_t)(i + 1));
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text_fmt(l, "Bed %d", i + 1);
    lv_obj_center(l);
  }
  refreshBedPills();

  // ---- mode row ----
  lv_obj_t *modes = lv_obj_create(parent);
  lv_obj_remove_style_all(modes);
  lv_obj_set_size(modes, lv_pct(100), 48);
  lv_obj_set_flex_flow(modes, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(modes, 8, 0);
  static const char *modeNames[4] = {"All", "Zones", "Motors", "Presets"};
  for (int i = 0; i < 4; i++) {
    lv_obj_t *b = lv_btn_create(modes);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_height(b, 44);
    styleCard(b);
    modeBtns[i] = b;
    lv_obj_add_event_cb(b, onMode, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, modeNames[i]);
    lv_obj_center(l);
  }
  refreshModeBtns();

  // ---- content area ----
  content = lv_obj_create(parent);
  lv_obj_remove_style_all(content);
  lv_obj_set_width(content, lv_pct(100));
  lv_obj_set_flex_grow(content, 1);
  buildContent();

  // ---- bottom row: timer + STOP ----
  lv_obj_t *bottom = lv_obj_create(parent);
  lv_obj_remove_style_all(bottom);
  lv_obj_set_size(bottom, lv_pct(100), 52);
  lv_obj_set_flex_flow(bottom, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bottom, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(bottom, 12, 0);

  lv_obj_t *tl = lv_label_create(bottom);
  lv_label_set_text(tl, "Timer");
  lv_obj_set_style_text_color(tl, lv_color_hex(0x8b92ab), 0);

  lv_obj_t *ts = lv_slider_create(bottom);
  lv_obj_set_flex_grow(ts, 1);
  lv_obj_set_height(ts, 14);
  lv_slider_set_range(ts, TIMER_MIN_MIN, TIMER_MAX_MIN);
  lv_slider_set_value(ts, timerMin, LV_ANIM_OFF);
  lv_obj_add_event_cb(ts, onTimer, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ts, onTimer, LV_EVENT_RELEASED, nullptr);

  timerLabel = lv_label_create(bottom);
  lv_label_set_text_fmt(timerLabel, "%d min", timerMin);
  lv_obj_set_style_text_color(timerLabel, lv_color_hex(0xc9cee0), 0);

  lv_obj_t *stop = lv_btn_create(bottom);
  lv_obj_set_size(stop, 120, 46);
  lv_obj_set_style_bg_color(stop, lv_color_hex(0x5c2430), 0);
  lv_obj_set_style_radius(stop, 8, 0);
  lv_obj_add_event_cb(stop, onStop, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *sl2 = lv_label_create(stop);
  lv_label_set_text(sl2, "STOP");
  lv_obj_set_style_text_color(sl2, lv_color_hex(0xffd6dd), 0);
  lv_obj_center(sl2);
}

} // namespace massage_ui
