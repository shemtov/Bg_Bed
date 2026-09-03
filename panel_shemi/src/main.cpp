// ============================================================
// PANEL SHEMI - TEST 009  (on/off contrast)
// ============================================================
//
// Board: Waveshare ESP32-S3-Touch-LCD-7B, 1024 x 600
//
// WHAT THIS IS
//   The home screen from the old panel, laid out for the new one.
//   Tiles and touch only. No sensors, no clock, no RS485, no photos.
//   Look at it, decide whether the layout is right, and only then
//   port the deeper screens.
//
// WHY 4 x 2 AND NOT 3 x 2
//   The old grid was three 200 px tiles across, 22 px apart, starting
//   at x = 78. That is 644 px of tiles inside 800, leaving 78 px each
//   side.
//
//   Four of the same tiles is 866 px inside 1024, leaving 79 px each
//   side. Within one pixel of the old margin. So the wider screen
//   buys a whole extra column without changing a single dimension,
//   and every icon stays at its native 200 x 200 - no scaling, no
//   softening.
//
//   Vertically 600 - 416 = 184 spare, split as a taller status strip
//   above and breathing room below.
//
// THE TWO NEW SLOTS
//   Position 3 on the top row is the ceiling fan. Its 433 MHz
//   protocol is already decoded: protocol 2, 29 bits, address
//   0x3D755, fourteen buttons captured.
//
//   And it turns out the artwork already exists. fan_img.h was
//   sitting in include/ - 200x200, cut from a photograph of the
//   actual fan in the room. It was made for TEST 082, the delivery
//   that was never pushed and was assumed lost. The code went, the
//   picture survived. So the fan tile is a real photograph like the
//   other five, not a placeholder symbol.
//
//   Position 3 on the bottom row is left free on purpose.
//
// WHAT CHANGED vs TEST 008
//   A single dim value was applied to every tile whatever its state,
//   so on and off differed only by the artwork swap. Feedback from
//   the screen: keep the lit state as it is and push the unlit state
//   further down, so the difference is obvious at a glance rather
//   than something you have to read.
//
//   So the dim level is now a pair - dimOn and dimOff - and the tile
//   background darkens as well when unlit. Two effects stacking:
//   the picture recedes and the panel behind it recedes with it.
//
//   Both are tunable live, same as the backlight:
//       n<v>   dim when ON    0..255
//       f<v>   dim when OFF   0..255
//       b<v>   backlight      0..100
//
//   Defaults are a starting point only. Real calibration waits for
//   the BH1750 - a value picked by eye in daylight is wrong at night,
//   and with the sensor in place brightness becomes a curve rather
//   than a number. No point doing it twice.
//
// WHAT CHANGED IN TEST 008 vs TEST 007  -  A HARDWARE FACT WORTH KEEPING
//   The backlight control is INVERTED. Measured, not guessed:
//   b0 gave a bright, readable screen and b100 gave a dark one.
//
//   IO_EXTENSION_Pwm_Output() does not take brightness. It takes how
//   much to DIM. So every build from TEST 003 to TEST 007 called it
//   with 100 at startup and ran the panel at full dim, which is why
//   even d0 looked washed out. The artwork was never the problem and
//   neither was the dim value - the screen was simply turned down.
//
//   From here the code inverts it: applyBacklight(100) writes 0 to
//   the expander and gives full brightness. b100 is bright, b0 is
//   off, which is what anyone would expect.
//
//   Leave this inversion in place. It is a property of the board,
//   and it will silently break every screen that gets ported if it
//   is ever removed.
//
// WHAT CHANGED IN TEST 007 vs TEST 006
//   Dim 255 gave a black screen, dim 0 still looked dull. That rules
//   the dimming out: 0 means no darkening at all, so if the artwork
//   is still weak at 0 the problem was never the dim value.
//
//   The remaining suspect is the backlight. TEST 002 proved the
//   IO expander drives it - the slider visibly changed brightness.
//   From TEST 003 on it was set once in setup() and never again,
//   with no way to check or change it. So it is now a live command
//   too, and the two are separated:
//       d<n>   tile dim      0..255
//       b<n>   backlight     0..100
//   Change one, watch, change the other. Whichever moves the picture
//   is the one that matters.
//
//   Default dim is now 10, chosen from the live tuning.
//
//   The monitor also does not echo what is typed. That is a
//   platformio.ini setting, not code: monitor_filters = echo.
//
// WHAT CHANGED IN TEST 006 vs TEST 005
//   TEST 005 ran and all eight tiles came up, but the artwork was
//   almost black. TILE_DIM_OPA was 110, a value tuned on the old
//   800x480 Guition panel. This screen has a different backlight and
//   a different panel, so the number did not carry over. Carrying it
//   was reasonable; not checking it was not.
//
//   Rather than guess a new value and rebuild five times, the dim
//   level is now adjustable from the serial monitor while the screen
//   is running. Type a number, watch the tiles change, and when it
//   looks right that number goes into the code as the new default.
//
//   Default lowered from 110 to 40 as a starting point.
//
// WHAT CHANGED IN TEST 005 vs TEST 004
//   TEST 004 would not compile: lv_font_montserrat_40 and _20 are not
//   built. The Waveshare lv_conf.h enables only 12, 14, 16 and 26.
//   The old panel had its own lv_conf.h with more sizes turned on,
//   and I wrote against that from memory instead of reading the one
//   actually in use.
//
//   So 40 becomes 26 and 20 becomes 16. Their lv_conf.h is left
//   untouched on purpose: it is a config file and editing it is
//   legitimate, but every edit to the vendor port layer is one more
//   thing to remember and restore if it is ever updated. Fitting our
//   code to what exists costs nothing here.
//
//   To get bigger symbols later, set LV_FONT_MONTSERRAT_40 to 1 in
//   lib/lv_conf.h. Not needed for this.
//
// WHAT IS DELIBERATELY MISSING
//   Every tile prints to serial instead of acting. Nothing is wired
//   to the bus yet, and a tile that silently does nothing is worse
//   than one that says so.
//
// NOT YET RUN ON HARDWARE.
//
// ============================================================

#include <Arduino.h>
#include "lvgl_port.h"
#include "rgb_lcd_port.h"
#include "io_extension.h"

#include "tile_img.h"       // img_bed_off/on, img_radio_off/on,
                            // img_ac_off/on, img_shade_open/closed
#include "lamp_img.h"       // img_lamp
#include "fan_img.h"        // img_fan - recovered from TEST 082
#include "font_hebrew.h"    // font_hebrew_28

#define TEST_NUMBER 9

// ------------------------------------------------------------
// Layout. Every number here is derived, not guessed - see the
// header note on why four columns fit exactly.
// ------------------------------------------------------------

#define SCREEN_W    1024
#define SCREEN_H     600

#define TILE_SZ      200
#define TILE_GAPX     22
#define TILE_GAPY     16
#define TILE_COLS      4
#define TILE_ROWS      2

// (1024 - (4*200 + 3*22)) / 2 = 79
#define TILE_X0       79
// status strip above, a little air below
#define TILE_Y0      108

// How far the tile artwork is pulled toward black, 0..255. Carried
// over from TEST 080 unchanged: 110 takes the glare off the white
// bed without making the icons hard to read.
// Runtime, not a #define, so it can be tuned live. 0 is the original
// artwork, 255 is a black square.
// Lit tiles keep the artwork close to original. Unlit tiles sink
// well back. The gap between the two is the whole point.
#define DIM_ON_DEFAULT   10
#define DIM_OFF_DEFAULT 120
static uint8_t dimOn  = DIM_ON_DEFAULT;
static uint8_t dimOff = DIM_OFF_DEFAULT;

// The panel behind the artwork moves too, so an unlit tile recedes
// twice over.
#define TILE_BG_ON  0x181C22
#define TILE_BG_OFF 0x0C0F13

// Backlight, 0..100, straight through the IO expander at 0x24.
#define BACKLIGHT_DEFAULT 100
static uint8_t backlight = BACKLIGHT_DEFAULT;

// Hebrew is stored pre-reversed in the font header, so the string is
// written here in visual order and needs no bidi engine.
#define HEB_BEDROOM  "\xD7\x94\xD7\xA0\xD7\x99\xD7\xA9\x20\xD7\xA8\xD7\x93\xD7\x97"
#define TITLE_HOME   "BG " HEB_BEDROOM

// ------------------------------------------------------------

static lv_obj_t *scrHome     = NULL;
static lv_obj_t *lblStatus   = NULL;

static lv_obj_t *tileBedImg   = NULL;
static lv_obj_t *tileRadioImg = NULL;
static lv_obj_t *tileACImg    = NULL;
static lv_obj_t *tileShadeImg = NULL;
static lv_obj_t *tileLampImg  = NULL;
static lv_obj_t *tileFanImg   = NULL;

// Each photographic tile is tracked as a button plus its image plus
// a pointer to the flag that says whether it is lit, so one function
// can repaint all of them.
struct Tile {
  lv_obj_t *btn;
  lv_obj_t *img;
  bool     *state;
};
static Tile  tiles[8];
static int   tileCount = 0;

// Panel-side state. Nothing reports back yet, so this is the only
// truth the screen has - exactly as on the old panel.
static bool bedRunning  = false;
static bool radioOn     = false;
static bool acPower     = false;
static bool shadeOpen   = true;
static bool lampOn      = false;
static bool fanOn       = false;

// ------------------------------------------------------------
// Defined below setup(), declared here so setup() can call them.
static void applyBacklight(uint8_t v);
static void printHelp();

static void say(const char *what) {
  Serial.printf("[tile] %s\n", what);
  if (lblStatus) lv_label_set_text(lblStatus, what);
}

// paintTile() is defined further down, next to the other drawing
// helpers, so it is declared here for the event handlers above it.
static void paintTile(const struct Tile &t);

// Find the tile that owns an image object, so a handler can repaint
// just its own tile without a second lookup table.
static void repaintOwner(lv_obj_t *img) {
  for (int i = 0; i < tileCount; i++)
    if (tiles[i].img == img) { paintTile(tiles[i]); return; }
}

static void evBed(lv_event_t *e) {
  bedRunning = !bedRunning;
  lv_img_set_src(tileBedImg, bedRunning ? &img_bed_on : &img_bed_off);
  repaintOwner(tileBedImg);
  say(bedRunning ? "massage on" : "massage off");
}

static void evRadio(lv_event_t *e) {
  radioOn = !radioOn;
  lv_img_set_src(tileRadioImg, radioOn ? &img_radio_on : &img_radio_off);
  repaintOwner(tileRadioImg);
  say(radioOn ? "radio playing" : "radio stopped");
}

static void evAC(lv_event_t *e) {
  acPower = !acPower;
  lv_img_set_src(tileACImg, acPower ? &img_ac_on : &img_ac_off);
  repaintOwner(tileACImg);
  say(acPower ? "air con on" : "air con off");
}

static void evShade(lv_event_t *e) {
  shadeOpen = !shadeOpen;
  lv_img_set_src(tileShadeImg, shadeOpen ? &img_shade_open : &img_shade_closed);
  repaintOwner(tileShadeImg);
  say(shadeOpen ? "shutters open" : "shutters closed");
}

static void evLamp(lv_event_t *e) {
  lampOn = !lampOn;
  // The lamp has one photograph and two colours, applied once on
  // press. Nothing repaints on a timer - that was the old jitter.
  // The lamp is tinted rather than blackened - amber when lit, cold
  // grey when not - so it needs its own recolour, then the shared
  // opacity and background rule on top.
  lv_obj_set_style_img_recolor(tileLampImg,
      lv_color_hex(lampOn ? 0xE8A030 : 0x9098A0), 0);
  repaintOwner(tileLampImg);
  say(lampOn ? "lamp on" : "lamp off");
}

static void evFan(lv_event_t *e) {
  fanOn = !fanOn;
  // One photograph, brightened when running. No animation - a tile
  // that repaints on a timer is what caused the old screen jitter.
  repaintOwner(tileFanImg);
  say(fanOn ? "fan on   (433 not wired yet)" : "fan off  (433 not wired yet)");
}

static void evSettings(lv_event_t *e) { say("settings - not built yet"); }
static void evSpare(lv_event_t *e)    { say("spare tile"); }

// ------------------------------------------------------------

static int tileX(int c) { return TILE_X0 + c * (TILE_SZ + TILE_GAPX); }
static int tileY(int r) { return TILE_Y0 + r * (TILE_SZ + TILE_GAPY); }

static lv_obj_t *makeTile(int col, int row, uint32_t bg) {
  lv_obj_t *b = lv_btn_create(scrHome);
  lv_obj_set_size(b, TILE_SZ, TILE_SZ);
  lv_obj_set_pos(b, tileX(col), tileY(row));
  lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
  lv_obj_set_style_radius(b, 16, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  return b;
}

static lv_obj_t *addImg(lv_obj_t *parent, const lv_img_dsc_t *src) {
  lv_obj_t *im = lv_img_create(parent);
  lv_img_set_src(im, src);
  lv_obj_center(im);
  lv_obj_set_style_img_recolor(im, lv_color_black(), 0);
  lv_obj_set_style_img_recolor_opa(im, dimOff, 0);
  return im;
}

// Paint one tile according to its own state. Both the artwork and
// the panel behind it move together.
static void paintTile(const struct Tile &t) {
  bool on = t.state ? *t.state : false;
  lv_obj_set_style_img_recolor_opa(t.img, on ? dimOn : dimOff, 0);
  lv_obj_set_style_bg_color(t.btn,
      lv_color_hex(on ? TILE_BG_ON : TILE_BG_OFF), 0);
}

// Repaint everything. Called from the serial handler, so it takes
// the LVGL mutex itself.
static void repaintAll() {
  if (!lvgl_port_lock(-1)) return;
  for (int i = 0; i < tileCount; i++) paintTile(tiles[i]);
  lvgl_port_unlock();
}

static void addSymbol(lv_obj_t *parent, const char *sym, uint32_t colour) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, sym);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(colour), 0);
  lv_obj_center(l);
}

// ------------------------------------------------------------

static void buildHome() {
  scrHome = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrHome, lv_color_hex(0x101418), 0);
  lv_obj_clear_flag(scrHome, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(scrHome);
  lv_label_set_text(title, TITLE_HOME);
  lv_obj_set_style_text_font(title, &font_hebrew_28, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 20);

  lv_obj_t *tn = lv_label_create(scrHome);
  lv_label_set_text_fmt(tn, "TEST %03d", TEST_NUMBER);
  lv_obj_set_style_text_font(tn, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(tn, lv_color_hex(0x556070), 0);
  lv_obj_align(tn, LV_ALIGN_TOP_MID, 0, 24);

  lv_obj_t *res = lv_label_create(scrHome);
  lv_label_set_text_fmt(res, "%d x %d", SCREEN_W, SCREEN_H);
  lv_obj_set_style_text_font(res, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(res, lv_color_hex(0x60D0FF), 0);
  lv_obj_align(res, LV_ALIGN_TOP_RIGHT, -24, 24);

  // --- row 0: bed, radio, air conditioner, fan ---
  lv_obj_t *t;

  t = makeTile(0, 0, TILE_BG_OFF);
  tileBedImg = addImg(t, &img_bed_off);
  tiles[tileCount++] = { t, tileBedImg, &bedRunning };
  lv_obj_add_event_cb(t, evBed, LV_EVENT_CLICKED, NULL);

  t = makeTile(1, 0, TILE_BG_OFF);
  tileRadioImg = addImg(t, &img_radio_off);
  tiles[tileCount++] = { t, tileRadioImg, &radioOn };
  lv_obj_add_event_cb(t, evRadio, LV_EVENT_CLICKED, NULL);

  t = makeTile(2, 0, TILE_BG_OFF);
  tileACImg = addImg(t, &img_ac_off);
  tiles[tileCount++] = { t, tileACImg, &acPower };
  lv_obj_add_event_cb(t, evAC, LV_EVENT_CLICKED, NULL);

  t = makeTile(3, 0, TILE_BG_OFF);
  tileFanImg = addImg(t, &img_fan);
  tiles[tileCount++] = { t, tileFanImg, &fanOn };
  lv_obj_add_event_cb(t, evFan, LV_EVENT_CLICKED, NULL);

  // --- row 1: lamp, shutters, settings, spare ---
  t = makeTile(0, 1, TILE_BG_OFF);
  tileLampImg = addImg(t, &img_lamp);
  lv_obj_set_style_img_recolor(tileLampImg, lv_color_hex(0x9098A0), 0);
  tiles[tileCount++] = { t, tileLampImg, &lampOn };
  lv_obj_add_event_cb(t, evLamp, LV_EVENT_CLICKED, NULL);

  t = makeTile(1, 1, TILE_BG_OFF);
  tileShadeImg = addImg(t, &img_shade_open);
  tiles[tileCount++] = { t, tileShadeImg, &shadeOpen };
  lv_obj_add_event_cb(t, evShade, LV_EVENT_CLICKED, NULL);

  t = makeTile(2, 1, 0x232A33);
  addSymbol(t, LV_SYMBOL_SETTINGS, 0x8A94A0);
  lv_obj_add_event_cb(t, evSettings, LV_EVENT_CLICKED, NULL);

  t = makeTile(3, 1, 0x1A1E24);          // deliberately free
  addSymbol(t, LV_SYMBOL_PLUS, 0x3A424C);
  lv_obj_add_event_cb(t, evSpare, LV_EVENT_CLICKED, NULL);

  // Status line under the grid. Nothing is wired to the bus, so
  // every tile says what it WOULD do rather than pretending.
  lblStatus = lv_label_create(scrHome);
  lv_label_set_text(lblStatus, "nothing is wired to the bus yet");
  lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lblStatus, lv_color_hex(0x707C88), 0);
  lv_obj_align(lblStatus, LV_ALIGN_BOTTOM_MID, 0, -22);

  repaintAll();
  lv_scr_load(scrHome);
}

// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("=== PANEL SHEMI - TEST 009 - home screen ===");
  Serial.printf("psram: %lu free of %lu\n",
                (unsigned long)ESP.getFreePsram(),
                (unsigned long)ESP.getPsramSize());

  static esp_lcd_panel_handle_t panel = NULL;
  static esp_lcd_touch_handle_t tp    = NULL;

  tp    = touch_gt911_init();
  panel = waveshare_esp32_s3_rgb_lcd_init();
  wavesahre_rgb_lcd_bl_on();
  Serial.println(tp    ? "touch ok" : "TOUCH FAILED");
  Serial.println(panel ? "panel ok" : "PANEL FAILED");

  ESP_ERROR_CHECK(lvgl_port_init(panel, tp));

  if (lvgl_port_lock(-1)) {
    buildHome();
    lvgl_port_unlock();
  }

  applyBacklight(BACKLIGHT_DEFAULT);

  Serial.println();
  Serial.println("Eight tiles, 200 px each, native size - no scaling.");
  Serial.println("Every press prints here and on the status line.");
  Serial.println("Judge the spacing and the margins, not the behaviour.");
  Serial.println();
  printHelp();
}

// v is BRIGHTNESS, 0 dark to 100 bright.
// The expander wants the opposite, so it is inverted here and
// nowhere else. Every other line in the project can now talk in
// plain brightness.
static void applyBacklight(uint8_t v) {
  if (v > 100) v = 100;
  backlight = v;
  IO_EXTENSION_Pwm_Output(100 - backlight);
  Serial.printf("backlight = %u %% brightness  (expander gets %u)\n",
                backlight, 100 - backlight);
}

static void printHelp() {
  Serial.println();
  Serial.println("  n<v>   dim when ON    0..255   0 = full colour");
  Serial.println("  f<v>   dim when OFF   0..255   255 = black");
  Serial.println("  b<v>   backlight      0..100   100 = brightest");
  Serial.println("  ?      this help");
  Serial.printf("  now: on %u, off %u, backlight %u %%\n",
                dimOn, dimOff, backlight);
  Serial.println();
  Serial.println("  The expander's PWM is inverted and the code already");
  Serial.println("  compensates. These numbers are plain brightness.");
}

void loop() {
  static String line = "";

  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      Serial.println();                 // echo, in case the filter is off
      if (line.length()) {
        line.trim();
        line.toLowerCase();
        if (line == "?") {
          printHelp();
        } else if (line.charAt(0) == 'n') {
          int v = line.substring(1).toInt();
          if (v >= 0 && v <= 255) {
            dimOn = (uint8_t)v; repaintAll();
            Serial.printf("dim ON  = %u\n", dimOn);
          } else Serial.println("  0..255");
        } else if (line.charAt(0) == 'f') {
          int v = line.substring(1).toInt();
          if (v >= 0 && v <= 255) {
            dimOff = (uint8_t)v; repaintAll();
            Serial.printf("dim OFF = %u\n", dimOff);
          } else Serial.println("  0..255");
        } else if (line.charAt(0) == 'b') {
          int v = line.substring(1).toInt();
          if (v >= 0 && v <= 100) applyBacklight((uint8_t)v);
          else Serial.println("  backlight is 0..100");
        } else {
          Serial.println("  n<v>, f<v>, b<v> or ?");
        }
        line = "";
      }
    } else if (line.length() < 8) {
      line += c;
      Serial.print(c);                  // echo each character as typed
    }
  }
  delay(20);
}

// ============================================================
// END OF FILE - PANEL SHEMI - TEST 009 (on/off contrast)
// ============================================================
