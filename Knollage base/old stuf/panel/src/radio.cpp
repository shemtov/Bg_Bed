#include "radio.h"
#include "config.h"
#include <Preferences.h>

#if ENABLE_RADIO
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"          // ESP32-audioI2S
static Audio audio;
#endif

namespace radio {

// ---------------------------------------------------------------------------
//  Genres: label + Radio Browser tag + tile color (muted for night use)
// ---------------------------------------------------------------------------
struct Genre { const char *label; const char *tag; uint32_t color; };
static const Genre genres[] = {
  {"Greek",     "greek",     0x2c58a8},
  {"Classical", "classical", 0x7a5bb5},
  {"Rock",      "rock",      0xb5483a},
  {"Blues",     "blues",     0x1d7a94},
  {"Jazz",      "jazz",      0xb07818},
  {"Israeli",   "israel",    0x2e8a5c},
  {"News",      "news",      0x5f6673},
  {"Chill",     "chillout",  0xa04f7e},
};
static const int NUM_GENRES = sizeof(genres) / sizeof(genres[0]);

// ---------------------------------------------------------------------------
//  Presets (persisted). Each stores name, URL, and its genre color.
// ---------------------------------------------------------------------------
struct Preset { char name[28]; char url[160]; uint32_t color; };
static const int MAX_PRESETS = 6;
static Preset presets[MAX_PRESETS];
static int numPresets = 0;
static Preferences prefs;

static int  volume = RADIO_VOLUME_DEFAULT;
static bool playing = false;
static unsigned long sleepUntil = 0;     // 0 = no sleep timer

static lv_obj_t *nowLabel = nullptr;
static lv_obj_t *listPage = nullptr;     // genre-results overlay

// ---------------------------------------------------------------------------
//  Persistence
// ---------------------------------------------------------------------------
static void loadPresets() {
  numPresets = prefs.getInt("np", 0);
  for (int i = 0; i < numPresets && i < MAX_PRESETS; i++) {
    char key[8];
    snprintf(key, sizeof(key), "p%d", i);
    prefs.getBytes(key, &presets[i], sizeof(Preset));
  }
  if (numPresets == 0) {   // starter examples - replace with your favorites
    strcpy(presets[0].name, "Example FM");
    strcpy(presets[0].url, "http://stream.example.com/live");
    presets[0].color = 0x2c58a8;
    numPresets = 1;
  }
}

static void savePreset(int idx) {
  char key[8];
  snprintf(key, sizeof(key), "p%d", idx);
  prefs.putBytes(key, &presets[idx], sizeof(Preset));
  prefs.putInt("np", numPresets);
}

// ---------------------------------------------------------------------------
//  Playback
// ---------------------------------------------------------------------------
static void playUrl(const char *name, const char *url) {
#if ENABLE_RADIO
  audio.stopSong();
  playing = audio.connecttohost(url);
#endif
  if (nowLabel) lv_label_set_text_fmt(nowLabel, "%s", name);
  Serial.printf("[radio] play %s -> %s\n", name, url);
}

static void stopPlay() {
#if ENABLE_RADIO
  audio.stopSong();
#endif
  playing = false;
  if (nowLabel) lv_label_set_text(nowLabel, "Stopped");
}

// ---------------------------------------------------------------------------
//  Genre browse: query Radio Browser for top stations of a tag
// ---------------------------------------------------------------------------
static void onStationPick(lv_event_t *e);

static void openGenre(const Genre &g) {
#if ENABLE_RADIO
  HTTPClient http;
  // votes-sorted top 12 stations for the tag
  String url = String("http://all.api.radio-browser.info/json/stations/bytag/") +
               g.tag + "?limit=12&order=votes&reverse=true&hidebroken=true";
  http.begin(url);
  http.addHeader("User-Agent", "BedPanel/1.0");
  int code = http.GET();
  if (code != 200) { http.end(); return; }

  JsonDocument doc;
  if (deserializeJson(doc, http.getStream())) { http.end(); return; }
  http.end();

  // overlay list page
  if (listPage) lv_obj_del(listPage);
  listPage = lv_obj_create(lv_scr_act());
  lv_obj_set_size(listPage, 800, 480);
  lv_obj_set_style_bg_color(listPage, lv_color_hex(0x14161f), 0);
  lv_obj_set_flex_flow(listPage, LV_FLEX_FLOW_COLUMN);

  lv_obj_t *back = lv_btn_create(listPage);
  lv_obj_set_size(back, 120, 44);
  lv_obj_add_event_cb(back, [](lv_event_t *ev) {
    lv_obj_del(listPage); listPage = nullptr;
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, "Back");
  lv_obj_center(bl);

  static char names[12][28];
  static char urls[12][160];
  static uint32_t colors[12];
  int n = 0;
  for (JsonObject st : doc.as<JsonArray>()) {
    if (n >= 12) break;
    const char *nm = st["name"] | "";
    const char *u  = st["url_resolved"] | "";
    if (!*nm || !*u) continue;
    strlcpy(names[n], nm, sizeof(names[n]));
    strlcpy(urls[n], u, sizeof(urls[n]));
    colors[n] = g.color;

    lv_obj_t *b = lv_btn_create(listPage);
    lv_obj_set_size(b, lv_pct(96), 44);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x1b1e2a), 0);
    lv_obj_set_style_border_side(b, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(b, 5, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(g.color), 0);
    lv_obj_add_event_cb(b, onStationPick, LV_EVENT_CLICKED,
                        (void *)(intptr_t)n);
    lv_obj_add_event_cb(b, onStationPick, LV_EVENT_LONG_PRESSED,
                        (void *)(intptr_t)(n + 100));   // long-press = save
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, names[n]);
    lv_obj_center(l);
    n++;
  }
  // stash pointers for the pick handler
  lv_obj_set_user_data(listPage, (void *)names);
  static void *stash[3];
  stash[0] = names; stash[1] = urls; stash[2] = colors;
  lv_obj_set_user_data(listPage, stash);
#else
  Serial.printf("[radio OFF] would browse genre %s\n", g.label);
#endif
}

static void onStationPick(lv_event_t *e) {
#if ENABLE_RADIO
  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  void **stash = (void **)lv_obj_get_user_data(listPage);
  char (*names)[28] = (char (*)[28])stash[0];
  char (*urls)[160] = (char (*)[160])stash[1];
  uint32_t *colors  = (uint32_t *)stash[2];

  if (idx >= 100) {                     // long-press: save as preset
    idx -= 100;
    if (numPresets < MAX_PRESETS) {
      strlcpy(presets[numPresets].name, names[idx], 28);
      strlcpy(presets[numPresets].url, urls[idx], 160);
      presets[numPresets].color = colors[idx];
      savePreset(numPresets);
      numPresets++;
      prefs.putInt("np", numPresets);
    }
    return;
  }
  playUrl(names[idx], urls[idx]);
  lv_obj_del(listPage);
  listPage = nullptr;
#endif
}

// ---------------------------------------------------------------------------
//  UI
// ---------------------------------------------------------------------------
void build(lv_obj_t *parent) {
  prefs.begin("radiop", false);
  loadPresets();

#if ENABLE_RADIO
  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT);
  audio.setVolume(volume);
#endif

  lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(parent, 8, 0);
  lv_obj_set_style_pad_row(parent, 8, 0);

  // ---- now playing + play/stop ----
  lv_obj_t *now = lv_obj_create(parent);
  lv_obj_remove_style_all(now);
  lv_obj_set_size(now, lv_pct(100), 56);
  lv_obj_set_flex_flow(now, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(now, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(now, 12, 0);

  nowLabel = lv_label_create(now);
  lv_label_set_text(nowLabel, "Not playing");
  lv_obj_set_flex_grow(nowLabel, 1);
  lv_obj_set_style_text_color(nowLabel, lv_color_hex(0xe8eaf2), 0);

  lv_obj_t *stopB = lv_btn_create(now);
  lv_obj_set_size(stopB, 100, 46);
  lv_obj_set_style_bg_color(stopB, lv_color_hex(0x3a2530), 0);
  lv_obj_add_event_cb(stopB, [](lv_event_t *e) { stopPlay(); },
                      LV_EVENT_CLICKED, nullptr);
  lv_obj_t *sl = lv_label_create(stopB);
  lv_label_set_text(sl, "Stop");
  lv_obj_center(sl);

  // ---- genre grid (2 rows x 4, color-coded) ----
  lv_obj_t *grid = lv_obj_create(parent);
  lv_obj_remove_style_all(grid);
  lv_obj_set_width(grid, lv_pct(100));
  lv_obj_set_flex_grow(grid, 1);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_column(grid, 8, 0);
  lv_obj_set_style_pad_row(grid, 8, 0);
  for (int i = 0; i < NUM_GENRES; i++) {
    lv_obj_t *b = lv_btn_create(grid);
    lv_obj_set_size(b, 186, 78);
    lv_obj_set_style_bg_color(b, lv_color_hex(genres[i].color), 0);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_add_event_cb(b, [](lv_event_t *e) {
      openGenre(genres[(int)(intptr_t)lv_event_get_user_data(e)]);
    }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, genres[i].label);
    lv_obj_center(l);
  }

  // ---- presets row (genre-colored left edge) ----
  lv_obj_t *pr = lv_obj_create(parent);
  lv_obj_remove_style_all(pr);
  lv_obj_set_size(pr, lv_pct(100), 52);
  lv_obj_set_flex_flow(pr, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(pr, 8, 0);
  for (int i = 0; i < numPresets; i++) {
    lv_obj_t *b = lv_btn_create(pr);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_height(b, 48);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x1b1e2a), 0);
    lv_obj_set_style_border_side(b, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(b, 5, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(presets[i].color), 0);
    lv_obj_add_event_cb(b, [](lv_event_t *e) {
      int i = (int)(intptr_t)lv_event_get_user_data(e);
      playUrl(presets[i].name, presets[i].url);
    }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, presets[i].name);
    lv_obj_center(l);
  }

  // ---- volume + sleep ----
  lv_obj_t *vb = lv_obj_create(parent);
  lv_obj_remove_style_all(vb);
  lv_obj_set_size(vb, lv_pct(100), 48);
  lv_obj_set_flex_flow(vb, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(vb, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(vb, 12, 0);

  lv_obj_t *vl = lv_label_create(vb);
  lv_label_set_text(vl, "Volume");
  lv_obj_set_style_text_color(vl, lv_color_hex(0x8b92ab), 0);

  lv_obj_t *vs = lv_slider_create(vb);
  lv_obj_set_flex_grow(vs, 1);
  lv_obj_set_height(vs, 14);
  lv_slider_set_range(vs, 0, 21);
  lv_slider_set_value(vs, volume, LV_ANIM_OFF);
  lv_obj_add_event_cb(vs, [](lv_event_t *e) {
    volume = lv_slider_get_value(lv_event_get_target(e));
#if ENABLE_RADIO
    audio.setVolume(volume);
#endif
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t *sleepB = lv_btn_create(vb);
  lv_obj_set_size(sleepB, 130, 44);
  lv_obj_set_style_bg_color(sleepB, lv_color_hex(0x3a2530), 0);
  lv_obj_add_event_cb(sleepB, [](lv_event_t *e) {
    sleepUntil = millis() + (unsigned long)SLEEP_TIMER_MIN * 60000UL;
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *sll = lv_label_create(sleepB);
  lv_label_set_text_fmt(sll, "Sleep %d", SLEEP_TIMER_MIN);
  lv_obj_center(sll);
}

void loop() {
#if ENABLE_RADIO
  audio.loop();
  if (sleepUntil && millis() > sleepUntil) {
    sleepUntil = 0;
    stopPlay();
  }
#endif
}

} // namespace radio
