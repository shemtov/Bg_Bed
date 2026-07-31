/* ============================================================
 *                        TEST  011
 * ============================================================
 *  PANEL FIRMWARE  (ESP32-8048S043, ESP32-S3-WROOM-1 N16R8)
 *  Adjustable Bed Massage Retrofit — Revision 4
 *  Stage 3: WELCOME PHOTO from microSD at boot + touch screen
 * ------------------------------------------------------------
 *  NEW vs TEST 010: at power-on the panel mounts the microSD
 *  card, decodes /welcome_800x480.jpg (the photo of the couple
 *  by the sea) and shows it FULL SCREEN for 2 seconds, then
 *  continues to the touch test screen. If the card or file is
 *  missing it prints why, shows a short note, and continues —
 *  the panel never gets stuck on a missing photo.
 *
 *  SD wiring on this board (fixed): CS=IO10, MOSI=IO11,
 *  CLK=IO12, MISO=IO13 (SPI). Card format: FAT32.
 *  Put the file  welcome_800x480.jpg  in the card's root.
 * ------------------------------------------------------------
 *  Touch: GT911 at 0x5D, SDA=19 SCL=20, raw 480x272 scaled to
 *  800x480 (calibrated in TEST 010).
 *  Display: 800x480 ST7262 parallel RGB. Backlight GPIO2.
 *  Libraries: GFX Library for Arduino @ 1.3.8, JPEGDEC.
 *  platformio.ini: v3 (adds JPEGDEC).
 * ------------------------------------------------------------
 *  TEST_NUMBER 11 — printed at boot AND shown on screen.
 * ============================================================ */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <JPEGDEC.h>
#include <Arduino_GFX_Library.h>

#define TEST_NUMBER 11

#define GFX_BL 2

// ---- SD card (SPI) pins on this board ----
#define SD_CS   10
#define SD_MOSI 11
#define SD_CLK  12
#define SD_MISO 13

#define WELCOME_FILE "/welcome_800x480.jpg"
#define WELCOME_MS   2000        // show the photo this long (1-2 s)

// ---- GT911 touch ----
#define TOUCH_SDA  19
#define TOUCH_SCL  20
#define GT911_ADDR1 0x5D
#define GT911_ADDR2 0x14
uint8_t gt911Addr = 0;

const int TOUCH_RAW_W = 480;
const int TOUCH_RAW_H = 272;
const int SCREEN_W = 800;
const int SCREEN_H = 480;

// ---- ST7262 800x480 parallel-RGB panel ----
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  40, 41, 39, 42,
  45, 48, 47, 21, 14,
  5, 6, 7, 15, 16, 4,
  8, 3, 46, 9, 1,
  0, 8, 4, 8,
  0, 8, 4, 8,
  1, 14000000
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  SCREEN_W, SCREEN_H, rgbpanel, 0, true
);

JPEGDEC jpeg;

// ---- forward declarations ----
bool gt911Probe();
bool gt911Read(int16_t &x, int16_t &y, bool &touched);
void drawBaseScreen();
bool showWelcomePhoto();
int  jpegDrawCb(JPEGDRAW *pDraw);

// ============================================================
//  JPEG draw callback — blits each decoded block to the screen
// ============================================================
int jpegDrawCb(JPEGDRAW *pDraw) {
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y,
                          pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

// ============================================================
//  Welcome photo from SD. Returns true if shown.
// ============================================================
bool showWelcomePhoto() {
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("SD: no card / mount failed — skipping welcome photo.");
    return false;
  }
  Serial.printf("SD mounted (%llu MB). Opening %s\n",
                SD.cardSize() / (1024ULL * 1024ULL), WELCOME_FILE);

  File f = SD.open(WELCOME_FILE, FILE_READ);
  if (!f) {
    Serial.println("SD: welcome file not found — skipping.");
    return false;
  }
  size_t sz = f.size();
  uint8_t *buf = (uint8_t *)ps_malloc(sz);      // PSRAM buffer
  if (!buf) { Serial.println("PSRAM alloc failed."); f.close(); return false; }
  f.read(buf, sz);
  f.close();

  if (!jpeg.openRAM(buf, sz, jpegDrawCb)) {
    Serial.println("JPEG open failed.");
    free(buf);
    return false;
  }
  jpeg.setPixelType(RGB565_BIG_ENDIAN);
  bool ok = jpeg.decode(0, 0, 0);
  jpeg.close();
  free(buf);
  Serial.println(ok ? "Welcome photo shown." : "JPEG decode failed.");
  return ok;
}

// ============================================================
//  GT911 low-level
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
  const uint8_t addrs[2] = { GT911_ADDR1, GT911_ADDR2 };
  for (int i = 0; i < 2; i++) {
    Wire.beginTransmission(addrs[i]);
    if (Wire.endTransmission() == 0) {
      gt911Addr = addrs[i];
      uint8_t id[5] = {0};
      gt911ReadRegs(0x8140, id, 4);
      Serial.printf("GT911 found at 0x%02X, ID: %c%c%c%c\n",
                    gt911Addr, id[0], id[1], id[2], id[3]);
      return true;
    }
  }
  Serial.println("GT911 NOT found — touch unavailable.");
  return false;
}

bool gt911Read(int16_t &x, int16_t &y, bool &touched) {
  uint8_t status = 0;
  if (gt911ReadRegs(0x814E, &status, 1) != 1) return false;
  touched = false;
  if (status & 0x80) {
    uint8_t n = status & 0x0F;
    if (n > 0) {
      uint8_t d[4];
      if (gt911ReadRegs(0x8150, d, 4) == 4) {
        int32_t rx = d[0] | (d[1] << 8);
        int32_t ry = d[2] | (d[3] << 8);
        x = (int16_t)constrain(rx * SCREEN_W / TOUCH_RAW_W, 0, SCREEN_W - 1);
        y = (int16_t)constrain(ry * SCREEN_H / TOUCH_RAW_H, 0, SCREEN_H - 1);
        touched = true;
      }
    }
    gt911WriteReg(0x814E, 0);
  }
  return true;
}

// ============================================================
//  Base (touch test) screen
// ============================================================
void drawBaseScreen() {
  gfx->fillScreen(BLACK);

  gfx->fillRect(0,  0, 800, 16, RED);
  gfx->fillRect(0, 16, 800, 16, GREEN);
  gfx->fillRect(0, 32, 800, 16, BLUE);

  gfx->drawRect(10, 60, 120, 50, WHITE);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(28, 77);
  gfx->print("CLEAR");

  char buf[16];
  snprintf(buf, sizeof(buf), "TEST %03d", TEST_NUMBER);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(4);
  gfx->setCursor(540, 66);
  gfx->print(buf);

  gfx->setTextColor(CYAN);
  gfx->setTextSize(2);
  gfx->setCursor(190, 130);
  gfx->print("Welcome photo OK - touch to draw");

  gfx->drawCircle(8, 8, 6, WHITE);
  gfx->drawCircle(791, 8, 6, WHITE);
  gfx->drawCircle(8, 471, 6, WHITE);
  gfx->drawCircle(791, 471, 6, WHITE);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("=== PANEL — TEST %03d — welcome photo + touch ===\n", TEST_NUMBER);

  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Wire.setClock(400000);

  if (!gfx->begin()) Serial.println("gfx->begin() FAILED");
  gfx->fillScreen(BLACK);

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  // --- Welcome photo, 2 seconds, then continue ---
  if (showWelcomePhoto()) {
    delay(WELCOME_MS);
  } else {
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(230, 220);
    gfx->print("(no welcome photo on SD)");
    delay(800);
  }

  drawBaseScreen();
  gt911Probe();
  Serial.println("Ready. Touch to draw.");
}

void loop() {
  // heartbeat
  static bool on = false;
  static uint32_t tBeat = 0;
  if (millis() - tBeat > 500) {
    tBeat = millis();
    on = !on;
    gfx->fillCircle(770, 450, 12, on ? GREEN : BLACK);
  }

  // touch
  static uint32_t tPoll = 0;
  if (gt911Addr && millis() - tPoll >= 10) {
    tPoll = millis();
    int16_t x, y; bool touched;
    if (gt911Read(x, y, touched) && touched) {
      if (x >= 10 && x <= 130 && y >= 60 && y <= 110) {
        drawBaseScreen();
      } else {
        gfx->fillCircle(x, y, 6, MAGENTA);
      }
      static uint32_t tPrint = 0;
      if (millis() - tPrint > 150) {
        tPrint = millis();
        Serial.printf("touch x=%d y=%d\n", x, y);
      }
    }
  }
}

/* ============================================================
 *                        TEST  011   (end of file)
 *  Panel — Stage 3 — welcome photo (SD, 2s) + scaled touch
 *  SD: CS=10 MOSI=11 CLK=12 MISO=13. File: /welcome_800x480.jpg
 *  Next: TEST 012 — BH1750 + SHT31 sensors on the I2C bus.
 * ============================================================ */
