/* ============================================================
 *                        TEST  041
 * ============================================================
 *  PANEL FIRMWARE  (ESP32-8048S043, ESP32-S3-WROOM-1 N16R8)
 *  Adjustable Bed Massage Retrofit — Revision 4
 *  Stage 3: THE REAL LVGL UI — home + massage + RADIO screens
 * ------------------------------------------------------------
 *  NEW vs TEST 040:
 *   - RADIO SCREEN, replacing the placeholder. Eight station
 *     tiles in genre colours, a now-playing bar with a colour
 *     stripe, volume 0-21, Sleep 30 and Stop. Tapping a tile
 *     sends the STATION INDEX (not a URL) over the same UART
 *     that reaches the bed boxes; the audio node listens on
 *     address 9 and owns the actual stream URLs, so the radio
 *     keeps playing while the panel screen is dark.
 *   - Protocol extended, still 4 bytes {id, cmd, target, value}:
 *       6 RADIO_PLAY  (target = station 0..7, value = volume)
 *       7 RADIO_STOP
 *       8 RADIO_VOL   (value = 0..21)
 *       9 RADIO_SLEEP (value = minutes)
 *     AUDIO_NODE_ID 9 is used in the bed/id byte, so bed boxes
 *     (1, 2) and 'both' (0) are untouched.
 *   - FIX: on the massage screen the "MASSAGE" title was drawn
 *     underneath the Shemi bed button (title at TOP_MID -60
 *     overlapped the button spanning x=245..420). Title moved
 *     to TOP_LEFT so both are readable.
 *   - Station names/colours live in STATION_NAME/STATION_COL at
 *     the top of this file. The audio node's URL list MUST be in
 *     the same order — index is the only thing sent over UART.
 *  From TEST 040:
 *   - Fix file-preview leftover: on returning from a full-screen
 *     photo preview, clear the whole screen and force a COMPLETE
 *     LVGL repaint (was leaving bits of the photo showing through).
 *   - Bigger Clock rollers (montserrat_40) so the set-time digits
 *     are large and easy to read.
 *  From TEST 027:
 *  NEW vs TEST 026:
 *   - Fix: forward-declare the clock globals/functions so the
 *     Settings code (which appears earlier in the file) can use
 *     them. TEST 026 failed to compile: 'baseSecOfDay',
 *     'nowSecOfDay', 'ds3231SetHM' not declared in scope.
 *  From TEST 026:
 *  NEW vs TEST 025:
 *   - SETTINGS panel (gear icon, top-right of home). Tabs:
 *       Clock   : set time on-screen (hour/min rollers) -> writes
 *                 DS3231 if present. No serial cable needed.
 *       Display : brightness floor slider + screensaver timeout.
 *       Massage : motor thresholds (small/big) + Random character.
 *       Files   : SD-card browser; tap a photo to preview full
 *                 screen (tap to return). Videos are listed but
 *                 can't be played (ESP32 has no video decoder).
 *       About   : test #, sensors, RTC status.
 *     All settings saved to flash (Preferences) - survive power-off.
 *  From TEST 025:
 *  NEW vs TEST 024:
 *   - DS3231 real-time-clock support (I2C 0x68). If the module
 *     is present the clock reads accurate battery-backed time
 *     that survives power cuts. If absent, falls back to the
 *     manual serial time ('time HH:MM'). Set the DS3231 once
 *     with the same 'time HH:MM' command; it writes it into the
 *     module and remembers forever.
 *   - Clock screensaver now shows real time when DS3231 present.
 *  From TEST 024:
 *  NEW vs TEST 023:
 *   - HOME redesign: "Ben Panel" title; big COLORED ICON tiles
 *     (Massage / Radio / AC); top-right shows TEMPERATURE with
 *     PRESSURE TREND arrow (up/down/steady) beside it; lux hidden
 *     (auto-brightness still runs silently).
 *   - Bed selector: big colored NAME buttons - Shemi (blue),
 *     Ira (red), Both (green); no numbers.
 *   (Hebrew title deferred: needs a custom Hebrew font.)
 *  From TEST 023:
 *  NEW vs TEST 022:
 *   - RANDOM preset (4th button): picks a random zone + random
 *     intensity every ~2.5 s, animates that slider as a live
 *     preview, and sends the ZONE message. OFF or another preset
 *     stops it.
 *   - RADIO and AC tiles now open simple placeholder screens
 *     (feature stages come later) instead of doing nothing.
 *  From TEST 022:
 *  NEW vs TEST 021:
 *   CLOCK SCREENSAVER replaces the photo screensaver:
 *    - Big 7-segment digits HH:MM (~3.6 cm tall, screen-wide),
 *      blinking colon, temperature small in the top-right corner.
 *    - Time source: type   time 14:35   in the Serial Monitor
 *      (once per power-up; ESP32 keeps counting). Until set, the
 *      clock shows --:--. Later the audio node will supply NTP
 *      time over UART (RF-free-headboard rule stays pure).
 *   Welcome photo at boot stays. Photos in /photos are no longer
 *   used by the saver.
 *  From TEST 021 (kept):
 *   LVGL 8.3 user interface:
 *    - HOME screen: big clock-style sensor strip (temp, pressure,
 *      light) + tiles: MASSAGE / RADIO / AC (Radio & AC are
 *      placeholders for later stages).
 *    - MASSAGE screen: bed selector (Bed 1 / Bed 2 / Both),
 *      four zone sliders (Head, Shoulders, Back, Legs, 0-100%),
 *      presets (Wave / Pulse / Ripple), timer (15/30/60 min),
 *      big OFF, and Back.
 *    - Every control sends the REAL 4-byte protocol message
 *      {bedId, cmd, target, value} out UART1 (TX=IO17, RX=IO18)
 *      at 115200 — the same header P3 pins — AND logs it to the
 *      serial monitor so you can verify before the bed box is
 *      even connected.
 *   Kept: welcome photo, /photos screensaver (5 min idle,
 *   touch exits), BH1750 auto-brightness (floor 85%),
 *   BMP/BME280 temp/press/(hum).
 * ------------------------------------------------------------
 *  platformio.ini: v4 REQUIRED (adds lvgl 8.3). Paste BOTH files.
 *  TEST_NUMBER 21 — printed at boot AND shown on screen.
 * ============================================================ */

#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <JPEGDEC.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <Preferences.h>

#define TEST_NUMBER 41

// ---- backlight ----
#define GFX_BL 2
#define BL_CH 7
#define BL_FREQ 5000
#define BL_RES 8
const float LUX_FLOOR = 2.0f, LUX_CEIL = 400.0f;
int brightFloor = 85;         // settings-adjustable
float targetDuty = 255, curDuty = 255;

// ---- SD ----
#define SD_CS 10
#define SD_MOSI 11
#define SD_CLK 12
#define SD_MISO 13
#define WELCOME_FILE "/welcome_800x480.jpg"
#define PHOTO_DIR "/photos"
#define DIAL_FILE "/diver_dial.jpg"
bool haveDial = false;
#define WELCOME_MS 2000
uint32_t saverTimeoutMs = 300000UL;   // settings-adjustable
#define PHOTO_MS 2000

// ---- UART to bed box (P3 header) ----
#define BB_TX 17
#define BB_RX 18
enum Cmd { CMD_OFF = 0, CMD_ALL = 1, CMD_ZONE = 2,
           CMD_MOTOR = 3, CMD_PRESET = 4, CMD_TIMER = 5,
           CMD_RADIO_PLAY = 6, CMD_RADIO_STOP = 7,
           CMD_RADIO_VOL = 8, CMD_RADIO_SLEEP = 9 };
uint8_t curBedId = 1;        // 1=bed1 2=bed2 0=both

// ---- audio node ----
// Shares the bed-box UART. The id byte addresses it, so the bed
// boxes ignore radio traffic and the node ignores massage traffic.
#define AUDIO_NODE_ID    9
#define RADIO_SLEEP_MIN  30
#define RADIO_VOL_MAX    21
#define NUM_STATIONS     8

// Index is the ONLY thing sent over the wire. The audio node holds
// the matching stream URLs in the same order - keep them in step.
const char *STATION_NAME[NUM_STATIONS] = {
  "Galgalatz",  "Kan Gimel",  "Eco 99",     "Kan 88",
  "102 FM",     "Kol Hamusica","SomaFM",    "Jazz24"
};
const uint32_t STATION_COL[NUM_STATIONS] = {
  0x2e8a5c, 0x2c58a8, 0xb5483a, 0xb07818,
  0x5f6673, 0x7a5bb5, 0xa04f7e, 0x1d7a94
};
bool randomActive = false;
uint32_t lastRandomMs = 0;

// ---- I2C ----
#define TOUCH_SDA 19
#define TOUCH_SCL 20
#define GT911_ADDR1 0x5D
#define GT911_ADDR2 0x14
#define BH1750_ADDR 0x23
#define BME280_ADDR1 0x76
#define BME280_ADDR2 0x77
#define DS3231_ADDR 0x68
bool haveDS3231 = false;
uint8_t gt911Addr = 0, bme280Addr = 0;
bool haveBH1750 = false, haveBME280 = false, isBME = false;

const int TOUCH_RAW_W = 480, TOUCH_RAW_H = 272;
const int SCREEN_W = 800, SCREEN_H = 480;

// ---- display ----
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  40, 41, 39, 42,
  45, 48, 47, 21, 14,
  5, 6, 7, 15, 16, 4,
  8, 3, 46, 9, 1,
  0, 8, 4, 8, 0, 8, 4, 8, 1, 14000000);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(SCREEN_W, SCREEN_H, rgbpanel, 0, true);
JPEGDEC jpeg;

// ---- app state ----
enum AppState { ST_UI, ST_SAVER, ST_PREVIEW };
AppState state = ST_UI;
uint32_t lastTouchMs = 0, photoShownMs = 0, lastSensorMs = 0, lastEaseMs = 0;

#define MAX_PHOTOS 60
char photoList[MAX_PHOTOS][64];
int photoCount = 0, lastPhotoIdx = -1;

// ---- sensors ----
float gLux = 0, gTemp = 0, gHum = -1, gPress = 0;
float pressRef = 0;            // reference for trend (updated slowly)
uint32_t pressRefMs = 0;
int pressTrend = 0;            // -1 down, 0 steady, +1 up
bool luxOK = false, tempOK = false;

// ---- BME280 calibration ----
uint16_t dig_T1; int16_t dig_T2, dig_T3;
uint16_t dig_P1; int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
uint8_t dig_H1, dig_H3; int16_t dig_H2, dig_H4, dig_H5; int8_t dig_H6;
int32_t t_fine;

// ---- LVGL ----
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *lvbuf1;
lv_obj_t *scrHome, *scrMassage, *scrRadio, *scrAC, *scrSettings, *scrBlank, *scrClock;
lv_obj_t *clkGlow, *clkMain, *clkColon, *clkTemp;
// analog clock
lv_obj_t *anaScr, *anaFace, *anaHour, *anaMin, *anaSec, *anaTemp, *anaCenter;
lv_point_t hourPts[2], minPts[2], secPts[2];
lv_style_t stHour, stMin, stSec;
Preferences prefs;
int setMinSmall = 60, setMinBig = 165;   // motor thresholds (panel copy)
int randomChar = 1;                      // 0 gentle 1 lively 2 wild
// file browser
#define MAX_BROWSE 80
char browseList[MAX_BROWSE][80];
int browseCount = 0;
char previewPath[96];
lv_obj_t *lblSensors, *lblTestNum;
lv_obj_t *sliderZone[4], *lblZoneVal[4];
const char *ZONE_NAME[4] = {"Head", "Shoulders", "Back", "Legs"};

// ---- forward declarations ----
int  jpegDrawCb(JPEGDRAW *p);
bool showPhoto(const char *path);
void scanPhotos();
void showRandomPhoto();
bool gt911Probe();
bool gt911Read(int16_t &x, int16_t &y, bool &touched);
void i2cScan();
bool bh1750Begin();
bool bh1750Read(float &lx);
bool bme280Begin();
bool bme280Read(float &t, float &h, float &p);
void setupBacklight();
void computeTargetFromLux();
void easeBacklight();
void sendMsg(uint8_t bed, uint8_t cmd, uint8_t target, uint8_t value);
void buildHome();
void buildMassage();
void buildRadio();
void buildAC();
void buildBedSelector();
void refreshBedButtons();
void buildSettings();
void loadSettings();
void saveSettings();
void populateFiles(lv_obj_t *list);
void buildClock();
void updateClockFace();
void buildAnalog();
void updateAnalog();
void drawDiverClock();
// clock globals/functions (defined later, used by Settings above them)
extern bool timeSet;
extern uint32_t baseMillis;
extern long baseSecOfDay;
long nowSecOfDay();
void ds3231SetHM(int hh, int mm);
void updateSensorLabel();
void enterSaver();
void exitSaver();

// ============================================================
//  UART protocol out
// ============================================================
void sendMsg(uint8_t bed, uint8_t cmd, uint8_t target, uint8_t value) {
  uint8_t m[4] = { bed, cmd, target, value };
  Serial1.write(m, 4);
  Serial.printf("UART TX -> bed=%d cmd=%d target=%d value=%d\n", bed, cmd, target, value);
}

// ============================================================
//  Backlight
// ============================================================
void setupBacklight() {
  ledcSetup(BL_CH, BL_FREQ, BL_RES);
  ledcAttachPin(GFX_BL, BL_CH);
  curDuty = 255; targetDuty = 255;
  ledcWrite(BL_CH, 255);
}
void computeTargetFromLux() {
  if (!haveBH1750 || !luxOK) { targetDuty = 255; return; }
  float l = constrain(gLux, LUX_FLOOR, LUX_CEIL);
  float f = (logf(l + 1) - logf(LUX_FLOOR + 1)) /
            (logf(LUX_CEIL + 1) - logf(LUX_FLOOR + 1));
  int pct = brightFloor + (int)((100 - brightFloor) * f);
  targetDuty = pct * 255.0f / 100.0f;
}
void easeBacklight() {
  curDuty += (targetDuty - curDuty) * 0.15f;
  curDuty = constrain(curDuty, 0.0f, 255.0f);
  ledcWrite(BL_CH, (int)(curDuty + 0.5f));
}

// ============================================================
//  JPEG / photos
// ============================================================
int jpegDrawCb(JPEGDRAW *p) {
  gfx->draw16bitRGBBitmap(p->x, p->y, p->pPixels, p->iWidth, p->iHeight);
  return 1;
}
bool showPhoto(const char *path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  size_t sz = f.size();
  uint8_t *buf = (uint8_t *)ps_malloc(sz);
  if (!buf) { f.close(); return false; }
  f.read(buf, sz); f.close();
  bool ok = false;
  if (jpeg.openRAM(buf, sz, jpegDrawCb)) {
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    ok = jpeg.decode(0, 0, 0);
    jpeg.close();
  }
  free(buf);
  return ok;
}
void scanPhotos() {
  photoCount = 0;
  File dir = SD.open(PHOTO_DIR);
  if (!dir || !dir.isDirectory()) { Serial.println("no /photos dir"); return; }
  File e;
  while ((e = dir.openNextFile()) && photoCount < MAX_PHOTOS) {
    if (!e.isDirectory()) {
      String n = e.name(); String low = n; low.toLowerCase();
      if (low.endsWith(".jpg") || low.endsWith(".jpeg")) {
        if (!n.startsWith("/")) n = String(PHOTO_DIR) + "/" + n;
        n.toCharArray(photoList[photoCount], 64); photoCount++;
      }
    }
    e.close();
  }
  dir.close();
  Serial.printf("screensaver: %d photos\n", photoCount);
}
void showRandomPhoto() {
  if (photoCount == 0) return;
  int idx = random(0, photoCount);
  if (photoCount > 1) while (idx == lastPhotoIdx) idx = random(0, photoCount);
  lastPhotoIdx = idx;
  showPhoto(photoList[idx]); photoShownMs = millis();
}

// ============================================================
//  GT911
// ============================================================
bool gt911WriteReg(uint16_t reg, uint8_t val) {
  Wire.beginTransmission(gt911Addr);
  Wire.write(reg >> 8); Wire.write(reg & 0xFF); Wire.write(val);
  return Wire.endTransmission() == 0;
}
int gt911ReadRegs(uint16_t reg, uint8_t *buf, int len) {
  Wire.beginTransmission(gt911Addr);
  Wire.write(reg >> 8); Wire.write(reg & 0xFF);
  if (Wire.endTransmission(false) != 0) return -1;
  int n = Wire.requestFrom((int)gt911Addr, len);
  for (int i = 0; i < n; i++) buf[i] = Wire.read();
  return n;
}
bool gt911Probe() {
  const uint8_t a[2] = { GT911_ADDR1, GT911_ADDR2 };
  for (int i = 0; i < 2; i++) {
    Wire.beginTransmission(a[i]);
    if (Wire.endTransmission() == 0) { gt911Addr = a[i];
      Serial.printf("GT911 at 0x%02X\n", gt911Addr); return true; }
  }
  Serial.println("GT911 not found"); return false;
}
bool gt911Read(int16_t &x, int16_t &y, bool &touched) {
  uint8_t st = 0;
  if (gt911ReadRegs(0x814E, &st, 1) != 1) return false;
  touched = false;
  if (st & 0x80) {
    if ((st & 0x0F) > 0) {
      uint8_t d[4];
      if (gt911ReadRegs(0x8150, d, 4) == 4) {
        int32_t rx = d[0] | (d[1] << 8), ry = d[2] | (d[3] << 8);
        x = constrain(rx * SCREEN_W / TOUCH_RAW_W, 0, SCREEN_W - 1);
        y = constrain(ry * SCREEN_H / TOUCH_RAW_H, 0, SCREEN_H - 1);
        touched = true;
      }
    }
    gt911WriteReg(0x814E, 0);
  }
  return true;
}

// ============================================================
//  I2C + sensors (BH1750, BME/BMP280)
// ============================================================
void i2cScan() {
  Serial.println("I2C scan:");
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("  0x%02X\n", a);
  }
}
bool bh1750Begin() {
  Wire.beginTransmission(BH1750_ADDR);
  if (Wire.endTransmission() != 0) return false;
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(0x10); return Wire.endTransmission() == 0;
}
bool bh1750Read(float &lx) {
  if (Wire.requestFrom(BH1750_ADDR, 2) != 2) return false;
  uint16_t raw = (Wire.read() << 8) | Wire.read();
  lx = raw / 1.2f; return true;
}
uint8_t bmeReadReg(uint8_t reg) {
  Wire.beginTransmission(bme280Addr);
  Wire.write(reg); Wire.endTransmission(false);
  Wire.requestFrom((int)bme280Addr, 1);
  return Wire.read();
}
void bmeReadRegs(uint8_t reg, uint8_t *buf, int len) {
  Wire.beginTransmission(bme280Addr);
  Wire.write(reg); Wire.endTransmission(false);
  Wire.requestFrom((int)bme280Addr, len);
  for (int i = 0; i < len; i++) buf[i] = Wire.read();
}
void bmeWriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(bme280Addr);
  Wire.write(reg); Wire.write(val); Wire.endTransmission();
}
bool bme280Begin() {
  const uint8_t addrs[2] = { BME280_ADDR1, BME280_ADDR2 };
  for (int i = 0; i < 2; i++) {
    Wire.beginTransmission(addrs[i]);
    if (Wire.endTransmission() == 0) {
      bme280Addr = addrs[i];
      uint8_t id = bmeReadReg(0xD0);
      if (id == 0x60) isBME = true;
      else if (id == 0x58) isBME = false;
      else return false;
      Serial.printf("%s at 0x%02X\n", isBME ? "BME280" : "BMP280", bme280Addr);
      bmeWriteReg(0xE0, 0xB6); delay(10);
      uint8_t c[26];
      bmeReadRegs(0x88, c, 26);
      dig_T1 = c[0] | (c[1] << 8);
      dig_T2 = (int16_t)(c[2] | (c[3] << 8));
      dig_T3 = (int16_t)(c[4] | (c[5] << 8));
      dig_P1 = c[6] | (c[7] << 8);
      dig_P2 = (int16_t)(c[8] | (c[9] << 8));
      dig_P3 = (int16_t)(c[10] | (c[11] << 8));
      dig_P4 = (int16_t)(c[12] | (c[13] << 8));
      dig_P5 = (int16_t)(c[14] | (c[15] << 8));
      dig_P6 = (int16_t)(c[16] | (c[17] << 8));
      dig_P7 = (int16_t)(c[18] | (c[19] << 8));
      dig_P8 = (int16_t)(c[20] | (c[21] << 8));
      dig_P9 = (int16_t)(c[22] | (c[23] << 8));
      dig_H1 = c[25];
      if (isBME) {
        uint8_t h[7];
        bmeReadRegs(0xE1, h, 7);
        dig_H2 = (int16_t)(h[0] | (h[1] << 8));
        dig_H3 = h[2];
        dig_H4 = (int16_t)((h[3] << 4) | (h[4] & 0x0F));
        dig_H5 = (int16_t)(((h[4] >> 4) & 0x0F) | (h[5] << 4));
        dig_H6 = (int8_t)h[6];
        bmeWriteReg(0xF2, 0x01);
      }
      bmeWriteReg(0xF5, 0xA0);
      bmeWriteReg(0xF4, 0x57);
      return true;
    }
  }
  return false;
}
bool bme280Read(float &t, float &h, float &p) {
  uint8_t d[8];
  bmeReadRegs(0xF7, d, 8);
  int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
  int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
  int32_t adc_H = ((int32_t)d[6] << 8) | d[7];
  int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * (int32_t)dig_T2) >> 11;
  int32_t var2 = (((((adc_T >> 4) - (int32_t)dig_T1) * ((adc_T >> 4) - (int32_t)dig_T1)) >> 12) * (int32_t)dig_T3) >> 14;
  t_fine = var1 + var2;
  t = (t_fine * 5 + 128) / 256 / 100.0f;
  int64_t v1 = (int64_t)t_fine - 128000;
  int64_t v2 = v1 * v1 * (int64_t)dig_P6;
  v2 = v2 + ((v1 * (int64_t)dig_P5) << 17);
  v2 = v2 + (((int64_t)dig_P4) << 35);
  v1 = ((v1 * v1 * (int64_t)dig_P3) >> 8) + ((v1 * (int64_t)dig_P2) << 12);
  v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)dig_P1) >> 33;
  if (v1 == 0) p = 0;
  else {
    int64_t pp = 1048576 - adc_P;
    pp = (((pp << 31) - v2) * 3125) / v1;
    v1 = (((int64_t)dig_P9) * (pp >> 13) * (pp >> 13)) >> 25;
    v2 = (((int64_t)dig_P8) * pp) >> 19;
    pp = ((pp + v1 + v2) >> 8) + (((int64_t)dig_P7) << 4);
    p = (float)pp / 256.0f / 100.0f;
  }
  if (isBME) {
    int32_t hv = t_fine - 76800;
    hv = (((((adc_H << 14) - ((int32_t)dig_H4 << 20) - ((int32_t)dig_H5 * hv)) + 16384) >> 15) *
          (((((((hv * (int32_t)dig_H6) >> 10) * (((hv * (int32_t)dig_H3) >> 11) + 32768)) >> 10) + 2097152) *
          (int32_t)dig_H2 + 8192) >> 14));
    hv = hv - (((((hv >> 15) * (hv >> 15)) >> 7) * (int32_t)dig_H1) >> 4);
    hv = constrain(hv, 0, 419430400);
    h = (float)(hv >> 12) / 1024.0f;
  } else h = -1;
  return true;
}

// ============================================================
//  LVGL glue
// ============================================================
void lvglFlushCb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  int w = area->x2 - area->x1 + 1, h = area->y2 - area->y1 + 1;
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  lv_disp_flush_ready(disp);
}
void lvglTouchCb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  int16_t x, y; bool t = false;
  if (gt911Addr && gt911Read(x, y, t) && t) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x; data->point.y = y;
    lastTouchMs = millis();
  } else data->state = LV_INDEV_STATE_REL;
}

// ============================================================
//  UI events
// ============================================================
static void evGoMassage(lv_event_t *e) { lv_scr_load(scrMassage); }
static void evGoRadio(lv_event_t *e)   { lv_scr_load(scrRadio); }
static void evGoAC(lv_event_t *e)      { lv_scr_load(scrAC); }
static void evGoHome(lv_event_t *e)    { lv_scr_load(scrHome); }
lv_obj_t *bedBtn[3];
const uint32_t BED_COL[3] = { 0x2060D0, 0xC02020, 0x209040 }; // Shemi blue, Ira red, Both green
const uint32_t BED_DIM[3] = { 0x142848, 0x481414, 0x14380f };
void refreshBedButtons() {
  int sel = (curBedId == 1) ? 0 : (curBedId == 2) ? 1 : 2;
  for (int i = 0; i < 3; i++)
    lv_obj_set_style_bg_color(bedBtn[i],
      lv_color_hex(i == sel ? BED_COL[i] : BED_DIM[i]), 0);
}
static void evBedSel(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  curBedId = (i == 0) ? 1 : (i == 1) ? 2 : 0;
  refreshBedButtons();
  Serial.printf("bed selector -> %d\n", curBedId);
}
void buildBedSelector() {
  const char *bn[3] = { "Shemi", "Ira", "Both" };
  for (int i = 0; i < 3; i++) {
    bedBtn[i] = lv_btn_create(scrMassage);
    lv_obj_set_size(bedBtn[i], 175, 66);
    lv_obj_align(bedBtn[i], LV_ALIGN_TOP_RIGHT, -10 - (2 - i) * 185, 8);
    lv_obj_set_style_radius(bedBtn[i], 14, 0);
    lv_obj_add_event_cb(bedBtn[i], evBedSel, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(bedBtn[i]);
    lv_label_set_text(l, bn[i]);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);
    lv_obj_center(l);
  }
  refreshBedButtons();
}
static void evZoneSlider(lv_event_t *e) {
  lv_obj_t *s = lv_event_get_target(e);
  int zone = (int)(intptr_t)lv_event_get_user_data(e);
  int v = lv_slider_get_value(s);
  lv_label_set_text_fmt(lblZoneVal[zone], "%d%%", v);
  if (lv_event_get_code(e) == LV_EVENT_RELEASED)
    sendMsg(curBedId, CMD_ZONE, zone, v);
}
static void evPreset(lv_event_t *e) {
  int p = (int)(intptr_t)lv_event_get_user_data(e);
  if (p == 3) { randomActive = true; lastRandomMs = 0; Serial.println("Random preset ON"); }
  else { randomActive = false; sendMsg(curBedId, CMD_PRESET, p, 60); }
}
static void evTimer(lv_event_t *e) {
  int mins = (int)(intptr_t)lv_event_get_user_data(e);
  sendMsg(curBedId, CMD_TIMER, 0, mins);
}
static void evOff(lv_event_t *e) {
  randomActive = false;
  sendMsg(curBedId, CMD_OFF, 0, 0);
  for (int z = 0; z < 4; z++) {
    lv_slider_set_value(sliderZone[z], 0, LV_ANIM_ON);
    lv_label_set_text(lblZoneVal[z], "0%");
  }
}

// ============================================================
//  Build screens
// ============================================================
void buildHome() {
  scrHome = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrHome, lv_color_hex(0x101418), 0);

  lv_obj_t *title = lv_label_create(scrHome);
  lv_label_set_text(title, "Ben Panel");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 20, 14);

  lblTestNum = lv_label_create(scrHome);
  lv_label_set_text_fmt(lblTestNum, "TEST %03d", TEST_NUMBER);
  lv_obj_set_style_text_font(lblTestNum, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(lblTestNum, lv_color_hex(0x556070), 0);
  lv_obj_align(lblTestNum, LV_ALIGN_TOP_MID, 0, 18);

  // top-right: temperature + pressure trend arrow
  lblSensors = lv_label_create(scrHome);
  lv_label_set_text(lblSensors, "-- C");
  lv_obj_set_style_text_font(lblSensors, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lblSensors, lv_color_hex(0x60D0FF), 0);
  lv_obj_align(lblSensors, LV_ALIGN_TOP_RIGHT, -20, 14);

  // icon tiles
  const char *icons[3] = { LV_SYMBOL_SETTINGS, LV_SYMBOL_AUDIO, LV_SYMBOL_POWER };
  const char *names[3] = { "Massage", "Radio", "AC" };
  const uint32_t cols[3] = { 0x2080FF, 0x30A060, 0xE08020 };
  for (int i = 0; i < 3; i++) {
    lv_obj_t *b = lv_btn_create(scrHome);
    lv_obj_set_size(b, 235, 260);
    lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, 22 + i * 252, -20);
    lv_obj_set_style_bg_color(b, lv_color_hex(cols[i]), 0);
    lv_obj_set_style_radius(b, 20, 0);
    if (i == 0) lv_obj_add_event_cb(b, evGoMassage, LV_EVENT_CLICKED, NULL);
    else if (i == 1) lv_obj_add_event_cb(b, evGoRadio, LV_EVENT_CLICKED, NULL);
    else lv_obj_add_event_cb(b, evGoAC, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ic = lv_label_create(b);
    lv_label_set_text(ic, icons[i]);
    lv_obj_set_style_text_font(ic, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(ic, lv_color_white(), 0);
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, names[i]);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(l, lv_color_white(), 0);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, 45);
  }

  // gear -> settings
  lv_obj_t *gear = lv_btn_create(scrHome);
  lv_obj_set_size(gear, 60, 60);
  lv_obj_align(gear, LV_ALIGN_TOP_RIGHT, -20, 60);
  lv_obj_set_style_bg_color(gear, lv_color_hex(0x303840), 0);
  lv_obj_set_style_radius(gear, 30, 0);
  lv_obj_add_event_cb(gear, [](lv_event_t*e){ lv_scr_load(scrSettings); }, LV_EVENT_CLICKED, NULL);
  lv_obj_t *gi = lv_label_create(gear);
  lv_label_set_text(gi, LV_SYMBOL_SETTINGS);
  lv_obj_set_style_text_font(gi, &lv_font_montserrat_28, 0);
  lv_obj_center(gi);
}

void buildMassage() {
  scrMassage = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrMassage, lv_color_hex(0x101418), 0);

  // Back
  lv_obj_t *back = lv_btn_create(scrMassage);
  lv_obj_set_size(back, 90, 50);
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_add_event_cb(back, evGoHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, "<");
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0);
  lv_obj_center(bl);

  lv_obj_t *title = lv_label_create(scrMassage);
  lv_label_set_text(title, "MASSAGE");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  // FIX (TEST 041): was TOP_MID -60, which put the title under the
  // Shemi button (x=245..420, y=8..74). Now tucked beside the back arrow.
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 115, 22);

  // Bed selector: big colored name buttons (Shemi/Ira/Both)
  buildBedSelector();

  // Zone sliders
  for (int z = 0; z < 4; z++) {
    int y = 90 + z * 62;
    lv_obj_t *nm = lv_label_create(scrMassage);
    lv_label_set_text(nm, ZONE_NAME[z]);
    lv_obj_set_style_text_font(nm, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(nm, lv_color_hex(0x60D0FF), 0);
    lv_obj_align(nm, LV_ALIGN_TOP_LEFT, 20, y + 8);

    sliderZone[z] = lv_slider_create(scrMassage);
    lv_obj_set_size(sliderZone[z], 440, 22);
    lv_obj_align(sliderZone[z], LV_ALIGN_TOP_LEFT, 170, y + 10);
    lv_slider_set_range(sliderZone[z], 0, 100);
    lv_obj_add_event_cb(sliderZone[z], evZoneSlider, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)z);
    lv_obj_add_event_cb(sliderZone[z], evZoneSlider, LV_EVENT_RELEASED, (void *)(intptr_t)z);

    lblZoneVal[z] = lv_label_create(scrMassage);
    lv_label_set_text(lblZoneVal[z], "0%");
    lv_obj_set_style_text_font(lblZoneVal[z], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lblZoneVal[z], lv_color_white(), 0);
    lv_obj_align(lblZoneVal[z], LV_ALIGN_TOP_LEFT, 640, y + 8);
  }

  // Presets (Wave, Pulse, Ripple, Random)
  const char *pn[4] = { "Wave", "Pulse", "Ripple", "Random" };
  for (int i = 0; i < 4; i++) {
    lv_obj_t *b = lv_btn_create(scrMassage);
    lv_obj_set_size(b, 115, 55);
    lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, 20 + i * 130, -75);
    if (i == 3) lv_obj_set_style_bg_color(b, lv_color_hex(0x9040D0), 0); // Random = purple
    lv_obj_add_event_cb(b, evPreset, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, pn[i]);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_center(l);
  }

  // Timer buttons
  const int tm[3] = { 15, 30, 60 };
  for (int i = 0; i < 3; i++) {
    lv_obj_t *b = lv_btn_create(scrMassage);
    lv_obj_set_size(b, 100, 55);
    lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, 20 + i * 115, -10);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x308050), 0);
    lv_obj_add_event_cb(b, evTimer, LV_EVENT_CLICKED, (void *)(intptr_t)tm[i]);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text_fmt(l, "%d min", tm[i]);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_center(l);
  }

  // OFF
  lv_obj_t *off = lv_btn_create(scrMassage);
  lv_obj_set_size(off, 200, 120);
  lv_obj_align(off, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
  lv_obj_set_style_bg_color(off, lv_color_hex(0xD03030), 0);
  lv_obj_add_event_cb(off, evOff, LV_EVENT_CLICKED, NULL);
  lv_obj_t *ol = lv_label_create(off);
  lv_label_set_text(ol, "OFF");
  lv_obj_set_style_text_font(ol, &lv_font_montserrat_40, 0);
  lv_obj_center(ol);
}


void buildPlaceholder(lv_obj_t **scr, const char *name) {
  *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(*scr, lv_color_hex(0x101418), 0);
  lv_obj_t *back = lv_btn_create(*scr);
  lv_obj_set_size(back, 90, 50);
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_add_event_cb(back, evGoHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, "<");
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0);
  lv_obj_center(bl);
  lv_obj_t *t = lv_label_create(*scr);
  lv_label_set_text_fmt(t, "%s", name);
  lv_obj_set_style_text_font(t, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(t, lv_color_white(), 0);
  lv_obj_align(t, LV_ALIGN_CENTER, 0, -30);
  lv_obj_t *s = lv_label_create(*scr);
  lv_label_set_text(s, "coming soon");
  lv_obj_set_style_text_font(s, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(s, lv_color_hex(0x7B90A0), 0);
  lv_obj_align(s, LV_ALIGN_CENTER, 0, 40);
}

// ============================================================
//  File browser
// ============================================================
lv_obj_t *fileList = NULL;
void populateFiles(lv_obj_t *list) {
  browseCount = 0;
  lv_obj_clean(list);
  const char *dirs[2] = { "/", "/photos" };
  for (int di = 0; di < 2; di++) {
    File dir = SD.open(dirs[di]);
    if (!dir || !dir.isDirectory()) continue;
    File e;
    while ((e = dir.openNextFile()) && browseCount < MAX_BROWSE) {
      if (!e.isDirectory()) {
        String n = e.name();
        if (!n.startsWith("/")) n = String(dirs[di]) + (di==0?"":"/") + n;
        String low = n; low.toLowerCase();
        const char *sym = LV_SYMBOL_FILE;
        if (low.endsWith(".jpg") || low.endsWith(".jpeg")) sym = LV_SYMBOL_IMAGE;
        else if (low.endsWith(".mp4") || low.endsWith(".avi") || low.endsWith(".mov")) sym = LV_SYMBOL_VIDEO;
        n.toCharArray(browseList[browseCount], 80);
        lv_obj_t *btn = lv_list_add_btn(list, sym, browseList[browseCount]);
        lv_obj_add_event_cb(btn, [](lv_event_t *ev){
          int idx = (int)(intptr_t)lv_event_get_user_data(ev);
          strncpy(previewPath, browseList[idx], sizeof(previewPath));
          String p = String(previewPath); p.toLowerCase();
          if (p.endsWith(".jpg") || p.endsWith(".jpeg")) {
            state = ST_PREVIEW;
            lv_scr_load(scrBlank);          // hide the list/UI
            lv_refr_now(NULL);
            if (!showPhoto(previewPath)) {
              gfx->fillScreen(BLACK);
              gfx->setTextColor(WHITE); gfx->setTextSize(3);
              gfx->setCursor(180, 220); gfx->print("cannot open image");
            }
          } else {
            state = ST_PREVIEW;
            lv_scr_load(scrBlank);          // hide the list/UI
            lv_refr_now(NULL);
            gfx->fillScreen(BLACK);
            gfx->setTextColor(0xFD20); gfx->setTextSize(3);
            gfx->setCursor(120, 200); gfx->print("Video files can't play");
            gfx->setTextColor(0x7BEF); gfx->setTextSize(2);
            gfx->setCursor(120, 250); gfx->print("(ESP32 has no video decoder)");
            gfx->setCursor(120, 300); gfx->print("Tap to return");
          }
        }, LV_EVENT_CLICKED, (void*)(intptr_t)browseCount);
        browseCount++;
      }
      e.close();
    }
    dir.close();
  }
  if (browseCount == 0) lv_list_add_text(list, "SD empty / not mounted");
}

// ============================================================
//  Settings screen
// ============================================================
lv_obj_t *rollH, *rollM, *lblFloorVal, *ddSaver, *sldSmall, *sldBig, *lblSmall, *lblBig, *ddRandom;

static void evSetTime(lv_event_t *e) {
  int hh = lv_roller_get_selected(rollH);
  int mm = lv_roller_get_selected(rollM);
  baseSecOfDay = (long)hh*3600 + (long)mm*60; baseMillis = millis(); timeSet = true;
  if (haveDS3231) ds3231SetHM(hh, mm);
  Serial.printf("time set %02d:%02d\n", hh, mm);
}
static void evFloor(lv_event_t *e) {
  brightFloor = lv_slider_get_value(lv_event_get_target(e));
  lv_label_set_text_fmt(lblFloorVal, "%d%%", brightFloor);
  saveSettings();
}
static void evSaver(lv_event_t *e) {
  const uint32_t opts[4] = {30000UL,60000UL,300000UL,600000UL};
  saverTimeoutMs = opts[lv_dropdown_get_selected(ddSaver)];
  saveSettings();
}
static void evSmall(lv_event_t *e) {
  setMinSmall = lv_slider_get_value(lv_event_get_target(e));
  lv_label_set_text_fmt(lblSmall, "%d", setMinSmall); saveSettings();
}
static void evBig(lv_event_t *e) {
  setMinBig = lv_slider_get_value(lv_event_get_target(e));
  lv_label_set_text_fmt(lblBig, "%d", setMinBig); saveSettings();
}
static void evRandom(lv_event_t *e) {
  randomChar = lv_dropdown_get_selected(ddRandom); saveSettings();
}




// ============================================================
//  Diver-watch clock (dial image + green arrow hands)
// ============================================================
void fillGlowLine(int x0,int y0,int x1,int y1,int w,uint16_t core,uint16_t glow){
  // simple glow: draw a few offset thick lines then the core
  for(int gx=-2; gx<=2; gx++) for(int gy=-2; gy<=2; gy++){
    if(gx==0&&gy==0) continue;
    for(int t=-(w/2); t<=w/2; t++){
      // thickness via perpendicular offset approx: draw parallel lines
    }
  }
}

// draw a thick line with rounded feel by stamping filled circles along it
void thickLine(int x0,int y0,int x1,int y1,int r,uint16_t col){
  int dx=abs(x1-x0), dy=abs(y1-y0);
  int steps = (dx>dy?dx:dy); if(steps<1) steps=1;
  for(int i=0;i<=steps;i++){
    int x = x0 + (x1-x0)*i/steps;
    int y = y0 + (y1-y0)*i/steps;
    gfx->fillCircle(x,y,r,col);
  }
}

void drawHand(float ang, int lenTail, int lenTip, int r, uint16_t glow, uint16_t core){
  // ang radians, 0=12 o'clock
  int cx=400, cy=240;
  int tx = cx + (int)(sinf(ang)*lenTip);
  int ty = cy - (int)(cosf(ang)*lenTip);
  int bx = cx - (int)(sinf(ang)*lenTail);
  int by = cy + (int)(cosf(ang)*lenTail);
  // glow underlay (thicker, dim green)
  thickLine(bx,by,tx,ty,r+3,glow);
  // core
  thickLine(bx,by,tx,ty,r,core);
  // arrow tip
  int ax = cx + (int)(sinf(ang)*(lenTip-18));
  int ay = cy - (int)(cosf(ang)*(lenTip-18));
  gfx->fillCircle(tx,ty,r+5,glow);
  gfx->fillCircle(ax,ay,r+3,core);
}

void drawDiverClock(){
  if (haveDial) { if(!showPhoto(DIAL_FILE)) gfx->fillScreen(BLACK); }
  else gfx->fillScreen(0x0842);

  long s = nowSecOfDay(); if (s<0) s=0;
  int hh=(s/3600)%12, mm=(s/60)%60, ss=s%60;
  const float D=3.14159265f/180.0f;
  const uint16_t GCORE=0x37E9;   // bright green
  const uint16_t GGLOW=0x0C86;   // dim green glow

  // hour (short, thick), minute (long), second (thin bright)
  drawHand((hh*30+mm*0.5f)*D, 22, 120, 7, GGLOW, GCORE);
  drawHand((mm*6+ss*0.1f)*D,  26, 175, 5, GGLOW, GCORE);
  // second hand: thinner, brighter, longer, red-ish tip
  drawHand((ss*6)*D, 40, 195, 2, 0x2985, 0xF800);

  // center cap
  gfx->fillCircle(400,240,10,GCORE);
  gfx->fillCircle(400,240,5,0xFFFF);

  // temperature (green) top-right
  if (tempOK){ char t[16]; snprintf(t,sizeof(t),"%.1f C",gTemp);
    gfx->setTextColor(0x37E9); gfx->setTextSize(3);
    gfx->setCursor(600,20); gfx->print(t); }
}

// ============================================================
//  Analog round clock (screensaver)
// ============================================================
#define ANA_CX 400
#define ANA_CY 250
#define ANA_R  230
void buildAnalog() {
  anaScr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(anaScr, lv_color_hex(0x131C26), 0);
  lv_obj_set_style_bg_grad_color(anaScr, lv_color_hex(0x0A1017), 0);
  lv_obj_set_style_bg_grad_dir(anaScr, LV_GRAD_DIR_VER, 0);
  lv_obj_clear_flag(anaScr, LV_OBJ_FLAG_SCROLLABLE);

  // face circle
  anaFace = lv_obj_create(anaScr);
  lv_obj_set_size(anaFace, ANA_R*2, ANA_R*2);
  lv_obj_align(anaFace, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(anaFace, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(anaFace, lv_color_hex(0x1B2836), 0);
  lv_obj_set_style_bg_grad_color(anaFace, lv_color_hex(0x101822), 0);
  lv_obj_set_style_bg_grad_dir(anaFace, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_border_color(anaFace, lv_color_hex(0x4C7C9C), 0);
  lv_obj_set_style_border_width(anaFace, 6, 0);
  lv_obj_clear_flag(anaFace, LV_OBJ_FLAG_SCROLLABLE);

  // 12 tick marks
  for (int i = 0; i < 12; i++) {
    float a = i * 30.0f * 3.14159265f / 180.0f;
    int r1 = ANA_R - 22, r2 = ANA_R - 8;
    lv_obj_t *tick = lv_obj_create(anaScr);
    lv_obj_set_size(tick, (i%3==0)?10:5, (i%3==0)?10:5);
    lv_obj_set_style_radius(tick, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(tick, lv_color_hex((i%3==0)?0xE0A860:0x546B80), 0);
    lv_obj_set_style_border_width(tick, 0, 0);
    int mr = (r1+r2)/2;
    lv_obj_align(tick, LV_ALIGN_CENTER, (int)(sinf(a)*mr), (int)(-cosf(a)*mr));
    lv_obj_clear_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
  }

  // hour numbers 1..12
  for (int n = 1; n <= 12; n++) {
    float a = n * 30.0f * 3.14159265f / 180.0f;
    int nr = ANA_R - 52;
    lv_obj_t *num = lv_label_create(anaScr);
    lv_label_set_text_fmt(num, "%d", n);
    lv_obj_set_style_text_font(num, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(num, lv_color_hex(0xF0E6D2), 0);
    lv_obj_align(num, LV_ALIGN_CENTER, (int)(sinf(a)*nr), (int)(-cosf(a)*nr));
    lv_obj_clear_flag(num, LV_OBJ_FLAG_SCROLLABLE);
  }

  // hand styles
  lv_style_init(&stHour); lv_style_set_line_width(&stHour, 16);
  lv_style_set_line_color(&stHour, lv_color_hex(0xE8A23C));
  lv_style_set_line_rounded(&stHour, true);
  lv_style_init(&stMin); lv_style_set_line_width(&stMin, 11);
  lv_style_set_line_color(&stMin, lv_color_hex(0xBFEFFF));
  lv_style_set_line_rounded(&stMin, true);
  lv_style_init(&stSec); lv_style_set_line_width(&stSec, 4);
  lv_style_set_line_color(&stSec, lv_color_hex(0xF06055));
  lv_style_set_line_rounded(&stSec, true);

  anaHour = lv_line_create(anaScr); lv_obj_add_style(anaHour, &stHour, 0);
  anaMin  = lv_line_create(anaScr); lv_obj_add_style(anaMin, &stMin, 0);
  anaSec  = lv_line_create(anaScr); lv_obj_add_style(anaSec, &stSec, 0);

  // center hub
  anaCenter = lv_obj_create(anaScr);
  lv_obj_set_size(anaCenter, 26, 26);
  lv_obj_align(anaCenter, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(anaCenter, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(anaCenter, lv_color_hex(0xE8A23C), 0);
  lv_obj_set_style_border_width(anaCenter, 0, 0);
  lv_obj_clear_flag(anaCenter, LV_OBJ_FLAG_SCROLLABLE);

  // temperature (green), top-right
  anaTemp = lv_label_create(anaScr);
  lv_label_set_text(anaTemp, "");
  lv_obj_set_style_text_font(anaTemp, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(anaTemp, lv_color_hex(0x35D06A), 0);
  lv_obj_align(anaTemp, LV_ALIGN_TOP_RIGHT, -30, 24);
}

void setHand(lv_obj_t *line, lv_point_t *pts, float ang, int len) {
  pts[0].x = ANA_CX; pts[0].y = ANA_CY;
  pts[1].x = ANA_CX + (int)(sinf(ang) * len);
  pts[1].y = ANA_CY - (int)(cosf(ang) * len);
  lv_line_set_points(line, pts, 2);
}

void updateAnalog() {
  long s = nowSecOfDay();
  if (s < 0) s = 0;
  int hh = (s/3600) % 12, mm = (s/60)%60, ss = s%60;
  const float D = 3.14159265f / 180.0f;
  setHand(anaHour, hourPts, (hh*30 + mm*0.5f) * D, ANA_R - 115);
  setHand(anaMin,  minPts,  (mm*6 + ss*0.1f)  * D, ANA_R - 45);
  setHand(anaSec,  secPts,  (ss*6)            * D, ANA_R - 30);
  if (tempOK) { char t[16]; snprintf(t, sizeof(t), "%.1f C", gTemp); lv_label_set_text(anaTemp, t); }
  else lv_label_set_text(anaTemp, "");
}

// ============================================================
//  Elegant LVGL clock (screensaver)
// ============================================================
void buildClock() {
  scrClock = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrClock, lv_color_black(), 0);
  lv_obj_clear_flag(scrClock, LV_OBJ_FLAG_SCROLLABLE);

  // soft glow: a dim, slightly larger copy behind
  clkGlow = lv_label_create(scrClock);
  lv_label_set_text(clkGlow, "--:--");
  lv_obj_set_style_text_font(clkGlow, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(clkGlow, lv_color_hex(0x0E5075), 0);
  lv_obj_set_style_text_letter_space(clkGlow, 16, 0);
  lv_obj_align(clkGlow, LV_ALIGN_CENTER, 2, -8);

  // main sharp digits
  clkMain = lv_label_create(scrClock);
  lv_label_set_text(clkMain, "--:--");
  lv_obj_set_style_text_font(clkMain, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(clkMain, lv_color_hex(0xBFEFFF), 0);
  lv_obj_set_style_text_letter_space(clkMain, 16, 0);
  lv_obj_align(clkMain, LV_ALIGN_CENTER, 0, -10);

  // temperature, small and refined, top-right
  clkTemp = lv_label_create(scrClock);
  lv_label_set_text(clkTemp, "");
  lv_obj_set_style_text_font(clkTemp, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(clkTemp, lv_color_hex(0x35D06A), 0);   // green
  lv_obj_set_style_text_letter_space(clkTemp, 2, 0);
  lv_obj_align(clkTemp, LV_ALIGN_TOP_RIGHT, -30, 24);
}

void updateClockFace() {
  long s = nowSecOfDay();
  char buf[8];
  if (s >= 0) snprintf(buf, sizeof(buf), "%02ld:%02ld", (s/3600), (s/60)%60);
  else snprintf(buf, sizeof(buf), "--:--");
  lv_label_set_text(clkMain, buf);
  lv_label_set_text(clkGlow, buf);
  if (tempOK) { char t[16]; snprintf(t, sizeof(t), "%.1f C", gTemp); lv_label_set_text(clkTemp, t); }
  else lv_label_set_text(clkTemp, "");
}

void buildSettings() {
  scrSettings = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrSettings, lv_color_hex(0x101418), 0);

  lv_obj_t *back = lv_btn_create(scrSettings);
  lv_obj_set_size(back, 90, 46);
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 8, 6);
  lv_obj_add_event_cb(back, evGoHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(back); lv_label_set_text(bl, "<");
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0); lv_obj_center(bl);

  lv_obj_t *tv = lv_tabview_create(scrSettings, LV_DIR_TOP, 48);
  lv_obj_set_size(tv, 800, 424);
  lv_obj_align(tv, LV_ALIGN_BOTTOM_MID, 0, 0);

  lv_obj_t *tClock = lv_tabview_add_tab(tv, "Clock");
  lv_obj_t *tDisp  = lv_tabview_add_tab(tv, "Display");
  lv_obj_t *tMass  = lv_tabview_add_tab(tv, "Massage");
  lv_obj_t *tFiles = lv_tabview_add_tab(tv, "Files");
  lv_obj_t *tAbout = lv_tabview_add_tab(tv, "About");

  // ---- Clock ----
  static char hopts[24*3+1] = ""; static char mopts[60*3+1] = "";
  hopts[0]=0; for (int i=0;i<24;i++){ char b[5]; snprintf(b,5,"%02d\n",i); strcat(hopts,b);} hopts[strlen(hopts)-1]=0;
  mopts[0]=0; for (int i=0;i<60;i++){ char b[5]; snprintf(b,5,"%02d\n",i); strcat(mopts,b);} mopts[strlen(mopts)-1]=0;

  // centered row: [hour] : [min]
  lv_obj_t *row = lv_obj_create(tClock);
  lv_obj_set_size(row, 560, 220);
  lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 6);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 18, 0);

  rollH = lv_roller_create(row); lv_roller_set_options(rollH,hopts,LV_ROLLER_MODE_INFINITE);
  lv_obj_set_style_text_font(rollH,&lv_font_montserrat_40,0);
  lv_roller_set_visible_row_count(rollH,3);
  lv_obj_set_style_bg_color(rollH, lv_color_hex(0x18202A), 0);
  lv_obj_set_style_radius(rollH, 14, 0);
  lv_obj_set_style_bg_color(rollH, lv_color_hex(0x244055), LV_PART_SELECTED);
  lv_obj_set_style_text_color(rollH, lv_color_hex(0xBFEFFF), LV_PART_SELECTED);
  lv_obj_set_width(rollH,160); lv_obj_set_height(rollH,190);
  lv_obj_set_style_flex_grow(rollH, 0, 0);

  lv_obj_t *cln = lv_label_create(row); lv_label_set_text(cln,":");
  lv_obj_set_style_text_font(cln,&lv_font_montserrat_40,0);
  lv_obj_set_style_text_color(cln, lv_color_hex(0xBFEFFF), 0);

  rollM = lv_roller_create(row); lv_roller_set_options(rollM,mopts,LV_ROLLER_MODE_INFINITE);
  lv_obj_set_style_text_font(rollM,&lv_font_montserrat_40,0);
  lv_roller_set_visible_row_count(rollM,3);
  lv_obj_set_style_bg_color(rollM, lv_color_hex(0x18202A), 0);
  lv_obj_set_style_radius(rollM, 14, 0);
  lv_obj_set_style_bg_color(rollM, lv_color_hex(0x244055), LV_PART_SELECTED);
  lv_obj_set_style_text_color(rollM, lv_color_hex(0xBFEFFF), LV_PART_SELECTED);
  lv_obj_set_width(rollM,160); lv_obj_set_height(rollM,190);
  lv_obj_set_style_flex_grow(rollM, 0, 0);

  { long s=nowSecOfDay(); if(s>=0){ lv_roller_set_selected(rollH,(s/3600),LV_ANIM_OFF); lv_roller_set_selected(rollM,(s/60)%60,LV_ANIM_OFF);} }

  lv_obj_t *setb = lv_btn_create(tClock); lv_obj_set_size(setb,220,64);
  lv_obj_align(setb,LV_ALIGN_BOTTOM_MID,0,-14);
  lv_obj_set_style_bg_color(setb,lv_color_hex(0x209040),0);
  lv_obj_add_event_cb(setb,evSetTime,LV_EVENT_CLICKED,NULL);
  lv_obj_t *sbl=lv_label_create(setb); lv_label_set_text(sbl,"Set Time");
  lv_obj_set_style_text_font(sbl,&lv_font_montserrat_28,0); lv_obj_center(sbl);

  // ---- Display ----
  lv_obj_t *fl=lv_label_create(tDisp); lv_label_set_text(fl,"Min brightness (night)");
  lv_obj_set_style_text_font(fl,&lv_font_montserrat_20,0); lv_obj_align(fl,LV_ALIGN_TOP_LEFT,20,20);
  lv_obj_t *sf=lv_slider_create(tDisp); lv_obj_set_size(sf,480,20);
  lv_slider_set_range(sf,30,100); lv_slider_set_value(sf,brightFloor,LV_ANIM_OFF);
  lv_obj_align(sf,LV_ALIGN_TOP_LEFT,20,60);
  lv_obj_add_event_cb(sf,evFloor,LV_EVENT_VALUE_CHANGED,NULL);
  lblFloorVal=lv_label_create(tDisp); lv_label_set_text_fmt(lblFloorVal,"%d%%",brightFloor);
  lv_obj_set_style_text_font(lblFloorVal,&lv_font_montserrat_20,0); lv_obj_align(lblFloorVal,LV_ALIGN_TOP_LEFT,520,55);
  lv_obj_t *tl=lv_label_create(tDisp); lv_label_set_text(tl,"Screensaver after");
  lv_obj_set_style_text_font(tl,&lv_font_montserrat_20,0); lv_obj_align(tl,LV_ALIGN_TOP_LEFT,20,130);
  ddSaver=lv_dropdown_create(tDisp); lv_dropdown_set_options(ddSaver,"30 sec\n1 min\n5 min\n10 min");
  lv_obj_set_width(ddSaver,200); lv_obj_align(ddSaver,LV_ALIGN_TOP_LEFT,20,165);
  { int sel=2; if(saverTimeoutMs==30000UL)sel=0; else if(saverTimeoutMs==60000UL)sel=1; else if(saverTimeoutMs==600000UL)sel=3; lv_dropdown_set_selected(ddSaver,sel);}
  lv_obj_add_event_cb(ddSaver,evSaver,LV_EVENT_VALUE_CHANGED,NULL);

  // ---- Massage ----
  lv_obj_t *m1=lv_label_create(tMass); lv_label_set_text(m1,"Small motor threshold");
  lv_obj_set_style_text_font(m1,&lv_font_montserrat_20,0); lv_obj_align(m1,LV_ALIGN_TOP_LEFT,20,15);
  sldSmall=lv_slider_create(tMass); lv_obj_set_size(sldSmall,430,18);
  lv_slider_set_range(sldSmall,30,140); lv_slider_set_value(sldSmall,setMinSmall,LV_ANIM_OFF);
  lv_obj_align(sldSmall,LV_ALIGN_TOP_LEFT,20,50); lv_obj_add_event_cb(sldSmall,evSmall,LV_EVENT_VALUE_CHANGED,NULL);
  lblSmall=lv_label_create(tMass); lv_label_set_text_fmt(lblSmall,"%d",setMinSmall);
  lv_obj_set_style_text_font(lblSmall,&lv_font_montserrat_20,0); lv_obj_align(lblSmall,LV_ALIGN_TOP_LEFT,470,46);
  lv_obj_t *m2=lv_label_create(tMass); lv_label_set_text(m2,"Big motor threshold");
  lv_obj_set_style_text_font(m2,&lv_font_montserrat_20,0); lv_obj_align(m2,LV_ALIGN_TOP_LEFT,20,95);
  sldBig=lv_slider_create(tMass); lv_obj_set_size(sldBig,430,18);
  lv_slider_set_range(sldBig,100,230); lv_slider_set_value(sldBig,setMinBig,LV_ANIM_OFF);
  lv_obj_align(sldBig,LV_ALIGN_TOP_LEFT,20,130); lv_obj_add_event_cb(sldBig,evBig,LV_EVENT_VALUE_CHANGED,NULL);
  lblBig=lv_label_create(tMass); lv_label_set_text_fmt(lblBig,"%d",setMinBig);
  lv_obj_set_style_text_font(lblBig,&lv_font_montserrat_20,0); lv_obj_align(lblBig,LV_ALIGN_TOP_LEFT,470,126);
  lv_obj_t *m3=lv_label_create(tMass); lv_label_set_text(m3,"Random character");
  lv_obj_set_style_text_font(m3,&lv_font_montserrat_20,0); lv_obj_align(m3,LV_ALIGN_TOP_LEFT,20,180);
  ddRandom=lv_dropdown_create(tMass); lv_dropdown_set_options(ddRandom,"Gentle\nLively\nWild");
  lv_obj_set_width(ddRandom,200); lv_obj_align(ddRandom,LV_ALIGN_TOP_LEFT,20,215);
  lv_dropdown_set_selected(ddRandom,randomChar); lv_obj_add_event_cb(ddRandom,evRandom,LV_EVENT_VALUE_CHANGED,NULL);
  lv_obj_t *mn=lv_label_create(tMass); lv_label_set_text(mn,"(applies to bed box when linked)");
  lv_obj_set_style_text_font(mn,&lv_font_montserrat_20,0);
  lv_obj_set_style_text_color(mn,lv_color_hex(0x7B90A0),0); lv_obj_align(mn,LV_ALIGN_TOP_LEFT,240,220);

  // ---- Files ----
  fileList = lv_list_create(tFiles);
  lv_obj_set_size(fileList, 760, 340);
  lv_obj_align(fileList, LV_ALIGN_TOP_MID, 0, 0);
  populateFiles(fileList);

  // ---- About ----
  lv_obj_t *ab=lv_label_create(tAbout);
  char info[256];
  snprintf(info,sizeof(info),
    "Ben Panel\nTEST %03d\n\nLight  : %s\nTemp/Press: %s\nHumidity: %s\nRTC    : %s\nTouch  : GT911",
    TEST_NUMBER,
    haveBH1750?"BH1750 ok":"absent",
    haveBME280?(isBME?"BME280":"BMP280"):"absent",
    (haveBME280&&isBME)?"yes":"no",
    haveDS3231?"DS3231 ok":"none (manual)");
  lv_label_set_text(ab,info);
  lv_obj_set_style_text_font(ab,&lv_font_montserrat_20,0);
  lv_obj_set_style_text_color(ab,lv_color_hex(0x90C0E0),0);
  lv_obj_align(ab,LV_ALIGN_TOP_LEFT,30,20);

  // blank screen used behind full-screen photo previews
  scrBlank = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrBlank, lv_color_black(), 0);
}

// ============================================================
//  RADIO screen  (new in TEST 041)
//  Sends only a station INDEX to the audio node (id 9) over the
//  same UART as the bed boxes. The node owns the stream URLs.
// ============================================================
int radioStation = -1;                 // -1 = stopped
int radioVol     = 8;                  // 0..RADIO_VOL_MAX
lv_obj_t *stationBtn[NUM_STATIONS];
lv_obj_t *lblNowName, *lblNowSub, *nowStripe, *lblRadioVolVal;

void refreshStationBtns() {
  for (int i = 0; i < NUM_STATIONS; i++) {
    bool on = (i == radioStation);
    lv_obj_set_style_border_width(stationBtn[i], on ? 4 : 0, 0);
    lv_obj_set_style_border_color(stationBtn[i], lv_color_white(), 0);
  }
}

static void evStation(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  radioStation = i;
  sendMsg(AUDIO_NODE_ID, CMD_RADIO_PLAY, i, radioVol);
  lv_label_set_text(lblNowName, STATION_NAME[i]);
  lv_label_set_text(lblNowSub, "connecting ...");
  lv_obj_set_style_bg_color(nowStripe, lv_color_hex(STATION_COL[i]), 0);
  refreshStationBtns();
  Serial.printf("radio -> station %d (%s)\n", i, STATION_NAME[i]);
}

static void evRadioStop(lv_event_t *e) {
  radioStation = -1;
  sendMsg(AUDIO_NODE_ID, CMD_RADIO_STOP, 0, 0);
  lv_label_set_text(lblNowName, "Radio off");
  lv_label_set_text(lblNowSub, "");
  lv_obj_set_style_bg_color(nowStripe, lv_color_hex(0x3A424C), 0);
  refreshStationBtns();
}

static void evRadioVol(lv_event_t *e) {
  lv_obj_t *s = lv_event_get_target(e);
  radioVol = lv_slider_get_value(s);
  lv_label_set_text_fmt(lblRadioVolVal, "%d", radioVol);
  if (lv_event_get_code(e) == LV_EVENT_RELEASED)
    sendMsg(AUDIO_NODE_ID, CMD_RADIO_VOL, 0, radioVol);
}

static void evRadioSleep(lv_event_t *e) {
  sendMsg(AUDIO_NODE_ID, CMD_RADIO_SLEEP, 0, RADIO_SLEEP_MIN);
  lv_label_set_text_fmt(lblNowSub, "sleep timer %d min", RADIO_SLEEP_MIN);
}

void buildRadio() {
  scrRadio = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrRadio, lv_color_hex(0x101418), 0);

  // ---- header ----
  lv_obj_t *back = lv_btn_create(scrRadio);
  lv_obj_set_size(back, 90, 50);
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_add_event_cb(back, evGoHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, "<");
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0);
  lv_obj_center(bl);

  lv_obj_t *title = lv_label_create(scrRadio);
  lv_label_set_text(title, "RADIO");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 115, 22);

  lv_obj_t *tn = lv_label_create(scrRadio);
  lv_label_set_text_fmt(tn, "TEST %03d", TEST_NUMBER);
  lv_obj_set_style_text_font(tn, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(tn, lv_color_hex(0x556070), 0);
  lv_obj_align(tn, LV_ALIGN_TOP_RIGHT, -12, 22);

  // ---- now playing ----
  nowStripe = lv_obj_create(scrRadio);
  lv_obj_set_size(nowStripe, 8, 52);
  lv_obj_align(nowStripe, LV_ALIGN_TOP_LEFT, 12, 78);
  lv_obj_set_style_bg_color(nowStripe, lv_color_hex(0x3A424C), 0);
  lv_obj_set_style_border_width(nowStripe, 0, 0);
  lv_obj_set_style_radius(nowStripe, 3, 0);
  lv_obj_clear_flag(nowStripe, LV_OBJ_FLAG_SCROLLABLE);

  lblNowName = lv_label_create(scrRadio);
  lv_label_set_text(lblNowName, "Radio off");
  lv_obj_set_style_text_font(lblNowName, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lblNowName, lv_color_white(), 0);
  lv_obj_align(lblNowName, LV_ALIGN_TOP_LEFT, 34, 80);

  lblNowSub = lv_label_create(scrRadio);
  lv_label_set_text(lblNowSub, "");
  lv_obj_set_style_text_font(lblNowSub, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(lblNowSub, lv_color_hex(0x8B939C), 0);
  lv_obj_align(lblNowSub, LV_ALIGN_TOP_LEFT, 34, 110);

  // ---- station tiles: 4 x 2 ----
  for (int i = 0; i < NUM_STATIONS; i++) {
    int col = i % 4, row = i / 4;
    stationBtn[i] = lv_btn_create(scrRadio);
    lv_obj_set_size(stationBtn[i], 185, 92);
    lv_obj_align(stationBtn[i], LV_ALIGN_TOP_LEFT, 12 + col * 192, 148 + row * 100);
    lv_obj_set_style_radius(stationBtn[i], 10, 0);
    lv_obj_set_style_bg_color(stationBtn[i], lv_color_hex(STATION_COL[i]), 0);
    lv_obj_add_event_cb(stationBtn[i], evStation, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(stationBtn[i]);
    lv_label_set_text(l, STATION_NAME[i]);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_center(l);
  }

  // ---- volume ----
  lv_obj_t *vl = lv_label_create(scrRadio);
  lv_label_set_text(vl, "Vol");
  lv_obj_set_style_text_font(vl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(vl, lv_color_hex(0x8B939C), 0);
  lv_obj_align(vl, LV_ALIGN_TOP_LEFT, 14, 372);

  lv_obj_t *sld = lv_slider_create(scrRadio);
  lv_obj_set_size(sld, 380, 20);
  lv_obj_align(sld, LV_ALIGN_TOP_LEFT, 70, 374);
  lv_slider_set_range(sld, 0, RADIO_VOL_MAX);
  lv_slider_set_value(sld, radioVol, LV_ANIM_OFF);
  lv_obj_add_event_cb(sld, evRadioVol, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(sld, evRadioVol, LV_EVENT_RELEASED, NULL);

  lblRadioVolVal = lv_label_create(scrRadio);
  lv_label_set_text_fmt(lblRadioVolVal, "%d", radioVol);
  lv_obj_set_style_text_font(lblRadioVolVal, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(lblRadioVolVal, lv_color_white(), 0);
  lv_obj_align(lblRadioVolVal, LV_ALIGN_TOP_LEFT, 466, 372);

  // ---- sleep + stop ----
  lv_obj_t *slp = lv_btn_create(scrRadio);
  lv_obj_set_size(slp, 120, 60);
  lv_obj_align(slp, LV_ALIGN_TOP_LEFT, 520, 352);
  lv_obj_set_style_bg_color(slp, lv_color_hex(0x308050), 0);
  lv_obj_add_event_cb(slp, evRadioSleep, LV_EVENT_CLICKED, NULL);
  lv_obj_t *sl = lv_label_create(slp);
  lv_label_set_text_fmt(sl, "Sleep %d", RADIO_SLEEP_MIN);
  lv_obj_set_style_text_font(sl, &lv_font_montserrat_20, 0);
  lv_obj_center(sl);

  lv_obj_t *stp = lv_btn_create(scrRadio);
  lv_obj_set_size(stp, 133, 60);
  lv_obj_align(stp, LV_ALIGN_TOP_LEFT, 655, 352);
  lv_obj_set_style_bg_color(stp, lv_color_hex(0xD03030), 0);
  lv_obj_add_event_cb(stp, evRadioStop, LV_EVENT_CLICKED, NULL);
  lv_obj_t *stl = lv_label_create(stp);
  lv_label_set_text(stl, "Stop");
  lv_obj_set_style_text_font(stl, &lv_font_montserrat_28, 0);
  lv_obj_center(stl);

  refreshStationBtns();
}
void buildAC()    { buildPlaceholder(&scrAC, "AC"); }


// ============================================================
//  Settings persistence (flash)
// ============================================================
void loadSettings() {
  prefs.begin("panel", true);
  brightFloor    = prefs.getInt("bfloor", 85);
  saverTimeoutMs = prefs.getUInt("saver", 300000UL);
  setMinSmall    = prefs.getInt("dsmall", 60);
  setMinBig      = prefs.getInt("dbig", 165);
  randomChar     = prefs.getInt("rchar", 1);
  prefs.end();
}
void saveSettings() {
  prefs.begin("panel", false);
  prefs.putInt("bfloor", brightFloor);
  prefs.putUInt("saver", saverTimeoutMs);
  prefs.putInt("dsmall", setMinSmall);
  prefs.putInt("dbig", setMinBig);
  prefs.putInt("rchar", randomChar);
  prefs.end();
}

void updateSensorLabel() {
  // pressure trend: compare to a reference taken every 30 min
  if (tempOK) {
    if (pressRef == 0) { pressRef = gPress; pressRefMs = millis(); }
    if (millis() - pressRefMs > 1800000UL) {     // 30 min
      if (gPress > pressRef + 0.3f) pressTrend = 1;
      else if (gPress < pressRef - 0.3f) pressTrend = -1;
      else pressTrend = 0;
      pressRef = gPress; pressRefMs = millis();
    }
  }
  const char *arrow = (pressTrend > 0) ? LV_SYMBOL_UP :
                      (pressTrend < 0) ? LV_SYMBOL_DOWN : "-";
  char t[16] = "--";
  if (tempOK) snprintf(t, sizeof(t), "%.1f C", gTemp);
  if (tempOK) lv_label_set_text_fmt(lblSensors, "%s  %s", t, arrow);
  else        lv_label_set_text(lblSensors, "-- C");
}


// ============================================================
//  Clock (kept by millis since last 'time HH:MM' command)
// ============================================================
bool timeSet = false;
uint32_t baseMillis = 0;
long baseSecOfDay = 0;
int shownMin = -1;

uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }

bool ds3231Present() {
  Wire.beginTransmission(DS3231_ADDR);
  return Wire.endTransmission() == 0;
}
long ds3231SecOfDay() {
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom(DS3231_ADDR, 3) != 3) return -1;
  uint8_t ss = bcd2dec(Wire.read() & 0x7F);
  uint8_t mm = bcd2dec(Wire.read() & 0x7F);
  uint8_t hh = bcd2dec(Wire.read() & 0x3F);
  return (long)hh * 3600 + (long)mm * 60 + ss;
}
void ds3231SetHM(int hh, int mm) {
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(0x00);
  Wire.write(dec2bcd(0));      // seconds = 0
  Wire.write(dec2bcd(mm));
  Wire.write(dec2bcd(hh));
  Wire.endTransmission();
}

long nowSecOfDay() {
  if (haveDS3231) return ds3231SecOfDay();
  if (!timeSet) return -1;
  return (baseSecOfDay + (long)((millis() - baseMillis) / 1000UL)) % 86400L;
}

// ---- 7-segment digit renderer ----
// segments: bit0=A top,1=B tr,2=C br,3=D bottom,4=E bl,5=F tl,6=G mid
const uint8_t SEGMAP[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
void drawSeg(int x,int y,int w,int h,int t,uint8_t segs,uint16_t on,uint16_t off){
  // A
  gfx->fillRect(x+t, y, w-2*t, t, (segs&0x01)?on:off);
  // B
  gfx->fillRect(x+w-t, y+t, t, h/2-t, (segs&0x02)?on:off);
  // C
  gfx->fillRect(x+w-t, y+h/2, t, h/2-t, (segs&0x04)?on:off);
  // D
  gfx->fillRect(x+t, y+h-t, w-2*t, t, (segs&0x08)?on:off);
  // E
  gfx->fillRect(x, y+h/2, t, h/2-t, (segs&0x10)?on:off);
  // F
  gfx->fillRect(x, y+t, t, h/2-t, (segs&0x20)?on:off);
  // G
  gfx->fillRect(x+t, y+h/2-t/2, w-2*t, t, (segs&0x40)?on:off);
}

const int DG_W=130, DG_H=300, DG_T=26, DG_GAP=28, COLON_W=50;
const uint16_t CLK_ON=0x07FF, CLK_OFF=0x0000;   // cyan on black

void drawClockDigits() {
  long s = nowSecOfDay();
  int totalW = 4*DG_W + 3*DG_GAP + COLON_W;
  int x0 = (SCREEN_W - totalW)/2;
  int y0 = (SCREEN_H - DG_H)/2 + 10;
  int hh = -1, mm = -1;
  if (s >= 0) { hh = s/3600; mm = (s/60)%60; }
  int d[4];
  if (s >= 0) { d[0]=hh/10; d[1]=hh%10; d[2]=mm/10; d[3]=mm%10; }
  int x = x0;
  for (int i=0;i<4;i++){
    uint8_t segs = (s>=0) ? SEGMAP[d[i]] : 0x40;   // '-' if unset
    drawSeg(x, y0, DG_W, DG_H, DG_T, segs, CLK_ON, CLK_OFF);
    x += DG_W + DG_GAP;
    if (i==1) x += COLON_W;
  }
}
void drawColon(bool on){
  int totalW = 4*DG_W + 3*DG_GAP + COLON_W;
  int x0 = (SCREEN_W - totalW)/2;
  int cx = x0 + 2*DG_W + DG_GAP + DG_GAP/2 + COLON_W/2;
  int y0 = (SCREEN_H - DG_H)/2 + 10;
  uint16_t c = on ? CLK_ON : CLK_OFF;
  gfx->fillCircle(cx, y0 + DG_H/3, 16, c);
  gfx->fillCircle(cx, y0 + 2*DG_H/3, 16, c);
}
void drawSaverTemp(){
  gfx->fillRect(SCREEN_W-260, 0, 260, 80, BLACK);
  if (tempOK) {
    char v[16]; snprintf(v, sizeof(v), "%.1fC", gTemp);
    gfx->setTextColor(0x7BEF); gfx->setTextSize(8);   // ~0.75cm digits
    gfx->setCursor(SCREEN_W-250, 8); gfx->print(v);
  }
}
void drawClockScreen(){
  gfx->fillScreen(BLACK);
  drawClockDigits();
  drawColon(true);
  drawSaverTemp();
  if (!timeSet) {
    gfx->setTextColor(0x7BEF); gfx->setTextSize(2);
    gfx->setCursor(160, SCREEN_H-30);
    gfx->print("Set time: type  time HH:MM  in Serial Monitor");
  }
  shownMin = (nowSecOfDay()>=0) ? (int)((nowSecOfDay()/60)%60) : -2;
}

// ---- serial console: time HH:MM ----
String conBuf;
void pollSerialCommands(){
  while (Serial.available()){
    char c = Serial.read();
    if (c=='\n'){
      conBuf.trim();
      if (conBuf.startsWith("time ")){
        int hh, mm;
        if (sscanf(conBuf.c_str()+5, "%d:%d", &hh, &mm)==2 && hh>=0 && hh<24 && mm>=0 && mm<60){
          baseSecOfDay = (long)hh*3600 + (long)mm*60;
          baseMillis = millis();
          timeSet = true;
          if (haveDS3231) { ds3231SetHM(hh, mm); Serial.printf("DS3231 + clock set to %02d:%02d\n", hh, mm); }
          else Serial.printf("clock set to %02d:%02d (no DS3231; will reset on power-off)\n", hh, mm);
          if (state == ST_SAVER) drawDiverClock();
        } else Serial.println("usage: time HH:MM");
      }
      conBuf = "";
    } else if (c!='\r') conBuf += c;
  }
}

// ============================================================
//  Screensaver enter/exit around LVGL
// ============================================================
void enterSaver() {
  state = ST_SAVER;
  lv_scr_load(scrBlank);      // hide LVGL widgets
  lv_refr_now(NULL);
  drawDiverClock();
}
void exitSaver() {
  state = ST_UI;
  lastTouchMs = millis();
  lv_scr_load(scrHome);
  lv_refr_now(NULL);
}

// ============================================================
void setup() {
  Serial.begin(115200); delay(300);
  Serial.printf("=== PANEL — TEST %03d — LVGL UI + Settings ===\n", TEST_NUMBER);
  loadSettings();

  Serial1.begin(115200, SERIAL_8N1, BB_RX, BB_TX);   // UART to bed box

  Wire.begin(TOUCH_SDA, TOUCH_SCL); Wire.setClock(400000);
  if (!gfx->begin()) Serial.println("gfx->begin FAILED");
  gfx->fillScreen(BLACK);
  setupBacklight();

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  bool sd = SD.begin(SD_CS, SPI);
  if (sd && showPhoto(WELCOME_FILE)) delay(WELCOME_MS);
  if (sd) scanPhotos();
  if (sd) { File df=SD.open(DIAL_FILE); haveDial = (bool)df; if(df) df.close(); }

  i2cScan();
  gt911Probe();
  haveBH1750 = bh1750Begin();
  haveBME280 = bme280Begin();
  haveDS3231 = ds3231Present();
  if (haveDS3231) { timeSet = true; Serial.println("DS3231 RTC found (0x68) - using battery-backed time"); }
  else Serial.println("DS3231 absent - clock uses manual 'time HH:MM'");
  Serial.printf("BH1750 %s, BME/BMP280 %s\n",
                haveBH1750 ? "ok" : "absent", haveBME280 ? "ok" : "absent");

  randomSeed(esp_random());
  if (haveBH1750) luxOK = bh1750Read(gLux);
  if (haveBME280) tempOK = bme280Read(gTemp, gHum, gPress);
  computeTargetFromLux();

  // ---- LVGL init ----
  lv_init();
  const int BUF_LINES = 60;
  lvbuf1 = (lv_color_t *)ps_malloc(SCREEN_W * BUF_LINES * sizeof(lv_color_t));
  lv_disp_draw_buf_init(&draw_buf, lvbuf1, NULL, SCREEN_W * BUF_LINES);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_W;
  disp_drv.ver_res = SCREEN_H;
  disp_drv.flush_cb = lvglFlushCb;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lvglTouchCb;
  lv_indev_drv_register(&indev_drv);

  buildHome();
  buildMassage();
  buildRadio();
  buildAC();
  buildSettings();
  buildClock();
  buildAnalog();
  updateSensorLabel();
  lv_scr_load(scrHome);

  lastTouchMs = millis();
  Serial.println("Ready — LVGL UI live. Taps send UART messages (see log).");
}

void loop() {
  pollSerialCommands();
  if (state == ST_UI) {
    lv_timer_handler();

    if (millis() - lastSensorMs > 1000) {
      lastSensorMs = millis();
      if (haveBH1750) luxOK = bh1750Read(gLux);
      if (haveBME280) tempOK = bme280Read(gTemp, gHum, gPress);
      computeTargetFromLux();
      updateSensorLabel();
    }
    if (millis() - lastEaseMs > 25) { lastEaseMs = millis(); easeBacklight(); }

    // Random preset: character from settings
    uint32_t rInt = (randomChar==0)?3500:(randomChar==2)?1500:2500;
    int rLo = (randomChar==0)?30:(randomChar==2)?20:40;
    if (randomActive && lv_scr_act() == scrMassage &&
        millis() - lastRandomMs > rInt) {
      lastRandomMs = millis();
      int z = random(0, 4);
      int v = random(rLo, 101);
      lv_slider_set_value(sliderZone[z], v, LV_ANIM_ON);
      lv_label_set_text_fmt(lblZoneVal[z], "%d%%", v);
      sendMsg(curBedId, CMD_ZONE, z, v);
    }

    if (millis() - lastTouchMs > saverTimeoutMs) enterSaver();
    delay(5);
  } else if (state == ST_PREVIEW) {
    int16_t x, y; bool t = false;
    if (gt911Addr) gt911Read(x, y, t);
    if (t) {
      state = ST_UI;
      gfx->fillScreen(BLACK);              // wipe the photo
      lv_scr_load(scrSettings);            // back to Settings
      lv_refr_now(NULL);                   // force a complete repaint now
      delay(200);                          // debounce the release
    }
    if (millis() - lastEaseMs > 25) { lastEaseMs = millis(); easeBacklight(); }
    delay(10);
  } else {
    // diver clock screensaver (raw gfx, LVGL paused)
    int16_t x, y; bool t = false;
    if (gt911Addr && gt911Read(x, y, t) && t) { exitSaver(); return; }

    static uint32_t lastTick = 0;
    if (millis() - lastTick >= 1000) {
      lastTick = millis();
      drawDiverClock();
    }
    if (millis() - lastEaseMs > 25) { lastEaseMs = millis(); easeBacklight(); }
    delay(10);
  }
}

/* ============================================================
 *                        TEST  041   (end of file)
 *  Panel — Stage 3 — RADIO SCREEN: 8 station tiles in genre
 *  colours, now-playing bar with colour stripe, volume 0-21,
 *  Sleep 30, Stop. Sends station INDEX to audio node id 9 over
 *  the shared bed-box UART (cmd 6..9). Station names/colours in
 *  STATION_NAME / STATION_COL at the top of this file - the
 *  node's URL list must be in the SAME ORDER.
 *  Also: massage title moved off the Shemi button.
 *  Next: audio node firmware (WT32-ETH01 + PCM5102A) as its
 *  own TEST 001, then verify a tap here starts a stream there.
 *  ---- previous, unchanged ----
 *  minute-roller fix + DIVER-watch clock
 *  time-picker rollers; preview uses blank screen;
 *  SETTINGS panel (Clock/Display/Massage/
 *  Files/About, saved to flash) + DS3231 RTC + home
 *  icon tiles + temp/pressure-trend,
 *  named colored bed selector (Shemi/Ira/Both), Random preset,
 *  Radio/AC
 *  placeholders + CLOCK screensaver (7-segment
 *  HH:MM ~3.6cm, temp small top-right). Time: 'time HH:MM' in
 *  Serial Monitor; audio node will supply NTP later over UART.
 *  Requires platformio.ini v4 (lvgl 8.3, unchanged from 021).
 *  Next: wire the bed box to P3 and watch taps drive motors.
 * ============================================================ */
