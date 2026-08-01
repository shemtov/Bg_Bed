/* ============================================================
 *                        TEST  061
 * ============================================================
 *  PANEL FIRMWARE  (ESP32-8048S043, ESP32-S3-WROOM-1 N16R8)
 *  Adjustable Bed Massage Retrofit - Revision 4
 *  Stage 3: LVGL UI - home + massage + settings + screensaver
 * ------------------------------------------------------------
 *  WHAT THIS BUILD IS
 *  TEST 041-044 were lost (no copy on disk, Drive or chat).
 *  TEST 045 rebuilds those four steps on top of TEST 040,
 *  which was itself a FAILED minute-roller attempt. The broken
 *  rollers are gone. A new number was used instead of 044 so a
 *  reconstruction is never mistaken for the original.
 *
 *  REBUILT FROM 040:
 *   041 - memory-safe photos: 600 KB size guard, PSRAM buffer
 *         with heap fallback, always freed. No freeze at 38+.
 *       - Files tab slideshow, 2 s per photo, touch to stop.
 *   042 - time picker rebuilt as +/- buttons (rollers never
 *         worked), 12-hour display with AM/PM everywhere.
 *   043 - clock hands dimmed to about 70 percent.
 *   044 - null guards on every label update. Fixes the
 *         LoadProhibited reboot at EXCVADDR 0x00000022, which
 *         was a timer touching a label before it was built.
 *
 *  NEW IN 061 - IT ACTUALLY DETECTS THE WORD NOW:
 *   Measured on TEST 060 with three references learned from live
 *   speech: 10 repeats of the wake word scored 5.91-8.36, six
 *   other words scored 13.02-23.86. No overlap, 1.56x gap.
 *   WAKE_THRESHOLD is 10.5 - about 26% clear of both sides.
 *   Adjust live with `th 9.5`; `dsp` shows the current value.
 *   A duration gate runs BEFORE the DTW: a word whose length is
 *   outside 0.5x-1.7x of the learned references is rejected
 *   without the maths. On the measured data that threw out 4 of
 *   the 6 non-words and lost none of the 10 true ones.
 *   FIXED: the microphone was serviced only inside the ST_UI
 *   branch, so it stopped the moment the screensaver came on -
 *   exactly when the wake word matters. It now runs in every
 *   state, and a detection wakes the screen.
 *
 *  NEW IN 060 - 059 would not compile:
 *   the catch-all `else` for unknown commands was placed BEFORE
 *   `else if (conBuf == "mic")`, so a terminal else sat in the
 *   middle of the chain. Braces balanced, which is why a brace
 *   check did not catch it. Moved to the end where it belongs.
 *   059 is otherwise unchanged; a new number is used so there is
 *   never a question of which 059 someone has.
 *
 *  NEW IN 059 - the console never saw your commands:
 *   pollSerialCommands acted on '\n' and DISCARDED '\r'. Pressing
 *   Enter in the PlatformIO monitor sends CR, so `ref` was thrown
 *   away silently - 20 words captured, no reference ever stored.
 *   Either terminator now ends a line, and platformio.ini gains
 *   monitor_filters send_on_enter.
 *   Also, no serial needed at all any more: a LEARN button on the
 *   home screen stores the last word. THREE reference slots are
 *   kept and a word is scored against the closest one, which
 *   handles normal variation in how you say it far better than a
 *   single template.
 *
 *  NEW IN 058 - MFCC FEATURES + DTW MATCHING:
 *   TEST 057 measured 8 real words at 560-656 ms, spread 1.17x,
 *   sd 33 ms. That consistency is what makes templates viable.
 *   pre-emphasis -> 25 ms Hamming frames / 10 ms hop -> 512-pt FFT
 *   -> 26 mel filters 300-8000 Hz -> log -> DCT -> 12 coefficients
 *   -> cepstral mean normalisation. Matching is dynamic time
 *   warping normalised by path length, so a word said a little
 *   faster or slower still matches.
 *   NOTHING ACTS ON A MATCH YET. It prints a distance so the
 *   threshold comes from real numbers instead of a guess:
 *     `ref`   store the last word as the reference
 *     `dsp`   show feature and reference status
 *   Say the wake word several times, then other words. If the two
 *   groups separate, the threshold goes between them.
 *
 *  NEW IN 057 - PRE-ROLL, so the start of the word is not lost:
 *   Measured on real speech at TEST 056: 9 words, 304-704 ms,
 *   mean 516, sd 135. RMS is averaged over a 16 ms buffer and has
 *   to climb 4x above the noise floor before capture starts, so
 *   roughly the first 30-60 ms of every word was already gone -
 *   exactly the part a wake word needs most.
 *   A 250 ms ring buffer now runs continuously. When speech
 *   triggers, that recent past is prepended, so the recording
 *   begins BEFORE the sound that triggered it.
 *   `mic` now also reports the spread of the last 8 words, which
 *   is the number that says whether templates are viable.
 *
 *  NEW IN 056 - UTTERANCE DETECTION (still no recognition):
 *   054 proved audio arrives. This finds WORDS in it - start, end,
 *   duration, peak - and keeps the samples in a 2 s PSRAM buffer.
 *   Both recognition paths need this: a trained model needs recorded
 *   samples, template matching needs a clean utterance to compare.
 *   The threshold is ADAPTIVE. A fixed one works in a silent room
 *   and fails with a fan running, so a noise floor is tracked while
 *   nothing is being said and speech must rise well above it.
 *   Serial commands: `dump` prints the last utterance as base64,
 *   `mic` prints the current levels and noise floor.
 *
 *  NEW IN 055 - the 1 Hz screensaver flicker:
 *   Every tick repainted all 800x480: dial first, hands after. The
 *   RGB panel scans continuously, so it could show a frame with the
 *   dial and no hands. That gap was the flash. Caching the dial in
 *   046 removed the SD read and the decode, but not the gap.
 *   Now a 424x424 PSRAM buffer over the clock face is filled from
 *   the cached dial, the hands are rasterised INTO it, and the
 *   finished image is blitted ONCE - so no half-drawn frame is ever
 *   on screen. The temperature repaints only when the whole number
 *   changes, not once a second. Falls back to the old path if the
 *   buffer will not allocate.
 *
 *  NEW IN 054 - MICROPHONE BRING-UP (capture only, no keyword):
 *   INMP441 on I2S0.  SD=GPIO11(DIN)  SCK=GPIO12(BCK)  WS=GPIO13
 *   VDD=3.3V, GND=GND, L/R=GND (left channel).
 *   A live level bar sits at the bottom of the HOME screen and the
 *   serial log prints RMS and peak once a second. That is the whole
 *   purpose of this test: prove audio arrives before writing any
 *   recognition on top of it.
 *   Four bugs from the old uncompiled microphone.cpp are fixed here:
 *   32-bit reads (not 16), mck_io_num set explicitly (it was being
 *   left at GPIO0), STAND_I2S format, and a non-blocking read so
 *   LVGL is never stalled.
 *
 *  NEW IN 053 - SD CARD RETIRED, DIAL MOVED INTO FLASH:
 *   The panel exposes only 7 GPIOs (11,12,13,19,20,17,18) and all
 *   were taken. The INMP441 microphone needs 11/12/13, which were
 *   the SD card's MOSI/CLK/MISO. The card loses.
 *   - ENABLE_SD is a switch, not a deletion. Set it back to 1 when
 *     the microphone moves to the WT32-ETH01 and the pins are free.
 *   - With ENABLE_SD 0, SPI.begin() and SD.begin() are never called,
 *     so CS is never asserted, the card holds its data line high-Z
 *     and the microphone owns the bus safely.
 *   - the dial now lives in flash (include/dial_image.h, 16921 B of
 *     a 16 MB part) and no longer depends on the card at all.
 *   - LOST while ENABLE_SD is 0: the photo slideshow, the Files
 *     browser and the welcome photo.
 *
 *  NEW IN 052:
 *   - Settings > Clock: the hour, colon and minute were 0xBFEFFF,
 *     a very light blue. LV_CONF_SKIP leaves LV_THEME_DEFAULT_DARK
 *     at 0, so the tab page behind them is the LIGHT theme's near
 *     white - light blue on white, effectively invisible. They are
 *     black now. Named SET_TEXT_COL so it is one edit, not three.
 *   - the same washed-out combination applies to the About text and
 *     the massage note; left alone for now, see SET_TEXT_COL.
 *
 *  NEW IN 051 - temp_font.h regenerated, 050 would not compile:
 *   GFXglyph stores xOffset/yOffset as int8_t and GFXfont stores
 *   yAdvance as uint8_t. Measured from the baseline, 150 px digits
 *   need yOffset -147 and yAdvance 284 - both overflow. Offsets are
 *   now measured from the TOP OF THE DIGITS, so digits get yOffset 0
 *   and every field fits. The cursor y is therefore the digit top,
 *   not the baseline. '.' was dropped: it would sit ~150 px below
 *   the reference and cannot fit int8_t either.
 *
 *  NEW IN 050:
 *   - the temperature uses a real TrueType-derived GFX font
 *     (include/temp_font.h) instead of the built-in 5x7 bitmap
 *     scaled 20x. Digits are 150 px, the C is 46 px, both drawn
 *     from outlines so the curves are smooth.
 *   - placement now comes from getTextBounds() rather than the
 *     6x8 cell arithmetic, which no longer applies.
 *
 *  NEW IN 049:
 *   - temperature digits smaller; the C keeps its size and moves to
 *     the right of the digits, on the same bottom line.
 *   - dial buffer cleared to black before decode, so a failed decode
 *     shows black rather than uninitialised PSRAM (a blue gradient).
 *   - the decode callback counts blocks; the cache is only trusted if
 *     at least one block was actually written.
 *   - any dial failure is drawn ON SCREEN, no serial monitor needed.
 *
 *  NEW IN 048 - screensaver layout:
 *   - clock face moved LEFT. Its centre is now CLOCK_CX/CLOCK_CY,
 *     one place instead of 400,240 repeated three times.
 *   - temperature fills the freed space on the right, much larger
 *     (character cell is 6*size wide by 8*size tall, size 24 =
 *     144 x 192 px per digit), still whole degrees.
 *   - REQUIRES the matching left-positioned dial image on the SD
 *     card. A centred dial with CLOCK_CX=240 will not line up.
 *
 *  NEW IN 047 - fixes the confirmed boot crash:
 *   Guru Meditation (StoreProhibited) EXCVADDR 0x00000098 in
 *   lv_obj_class_create_obj <- lv_list_add_btn <- populateFiles.
 *   LVGL's heap defaults to 32 KB; one list button per SD file at
 *   boot exhausted it and the allocator's NULL was dereferenced.
 *   - the Files list is built when you OPEN the Files tab, and
 *     freed when you leave it. Nothing is built at boot.
 *   - populateFiles checks lv_mem_monitor() before every row and
 *     stops early instead of letting the allocation fail.
 *   - hard cap of BROWSE_MAX_SHOWN rows.
 *   - platformio.ini gains -DLV_MEM_SIZE=65536 for headroom.
 *
 *  NEW IN 046:
 *   - dial JPEG decoded ONCE into PSRAM at boot, then blitted each
 *     tick. 045 re-read and re-decoded it from SD every second.
 *   - temperature moved to the right margin, drawn very large,
 *     whole degrees only (no .1 / .2) on every clock face.
 *   - boot now prints whether the dial was found and cached, so a
 *     missing /diver_dial.jpg can no longer fail silently.
 *
 *  NOT TESTED ON HARDWARE. TEST 040 ran; 041-044 were tested by
 *  Shemi but their source is gone, so the code below is written
 *  from the change descriptions, not recovered. Flash it and
 *  watch the serial monitor.
 * ------------------------------------------------------------
 *  Board: ESP32-8048S043 | 800x480 ST7262 RGB | GT911 touch
 *  I2C SDA=19 SCL=20 | SD CS=10 MOSI=11 CLK=12 MISO=13 (FAT32)
 *  UART1 to bed box: TX=IO17 RX=IO18 @115200 via P3
 *  TEST_NUMBER 61 - printed at boot AND shown on screen.
 * ============================================================ */

#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <driver/i2s.h>        // TEST 054: INMP441 microphone
#include <JPEGDEC.h>
#include <Arduino_GFX_Library.h>
#include "temp_font.h"          // TEST 050: TempBig / TempSmall, put it in include/
#include "dial_image.h"         // TEST 053: the dial, embedded in flash
#include <lvgl.h>
#include <Preferences.h>

#define TEST_NUMBER 61

// ---- backlight ----
#define GFX_BL 2
#define BL_CH 7
#define BL_FREQ 5000
#define BL_RES 8
const float LUX_FLOOR = 2.0f, LUX_CEIL = 400.0f;
int brightFloor = 85;         // settings-adjustable
float targetDuty = 255, curDuty = 255;

// ---- SD ----
// ---- TEST 054: INMP441 microphone (I2S0) ----
// These are the SD card's old pins. Wired: SD->11, SCK->12, WS->13,
// VDD->3.3V, GND->GND, L/R->GND. ENABLE_SD must stay 0 while this is 1.
#define ENABLE_MIC       1
#define MIC_DIN_PIN      11
#define MIC_BCK_PIN      12
#define MIC_WS_PIN       13
#define MIC_PORT         I2S_NUM_0
#define MIC_SAMPLE_RATE  16000
#define MIC_FRAMES       256      // 32-bit samples per read

// ---- TEST 056: utterance detection ----
#define UTT_MAX_MS       2000     // longest word we will keep
#define UTT_MIN_MS       220      // shorter than this is a click, not a word
#define UTT_HANG_MS      280      // quiet for this long ends the utterance
#define UTT_MAX_SAMPLES  ((MIC_SAMPLE_RATE / 1000) * UTT_MAX_MS)
#define UTT_START_MULT   4.0f     // RMS must exceed noiseFloor * this
#define UTT_STOP_MULT    2.0f     // ...and fall below this to end
#define UTT_ABS_FLOOR    0.0035f  // absolute minimum, for a truly silent room
// TEST 057: audio kept before the trigger, so the word's onset survives
#define PRE_ROLL_MS      250
#define PRE_ROLL_SAMPLES ((MIC_SAMPLE_RATE / 1000) * PRE_ROLL_MS)
#define UTT_BUF_SAMPLES  (UTT_MAX_SAMPLES + PRE_ROLL_SAMPLES)
#define UTT_HISTORY      8        // durations kept for the spread report

// ---- TEST 058: MFCC + DTW ----
#define MFCC_FRAME       400      // 25 ms at 16 kHz
#define MFCC_HOP         160      // 10 ms
#define MFCC_NFFT        512
#define MFCC_NMEL        26
#define MFCC_NCEP        12       // c1..c12; c0 (loudness) is deliberately dropped
#define MFCC_FMIN        300.0f
#define MFCC_FMAX        8000.0f
#define MFCC_MAX_FRAMES  240      // 2.4 s of hops, above the 2.25 s capture cap

// ---- TEST 061: detection ----
#define WAKE_THRESHOLD   10.5f    // chosen from measured data, see the banner
#define WAKE_DUR_MIN     0.50f    // word length vs the learned average
#define WAKE_DUR_MAX     1.70f
#define WAKE_SHOW_MS     2500     // how long the banner stays up

// TEST 053: 0 = the microphone owns GPIO 11/12/13. Set to 1 only when the
// mic has moved to another board and the SPI pins are free again.
#define ENABLE_SD 0

#define SD_CS 10
#define SD_MOSI 11
#define SD_CLK 12
#define SD_MISO 13
#define WELCOME_FILE "/welcome_800x480.jpg"
#define PHOTO_DIR "/photos"
#define DIAL_FILE "/diver_dial.jpg"
// TEST 048: where the clock face sits on the screensaver. Must match the
// dial image on the card - the artwork in dial_left_*.jpg is centred on 240.
#define CLOCK_CX 240
#define CLOCK_CY 240
// Temperature block, centred in the space to the right of the dial.
#define TEMP_CX  628
// TEST 050: sizes now come from the font in temp_font.h (digits 150 px,
// C 46 px). TEMP_C_GAP is still the space between the digits and the C.
#define TEMP_C_GAP   16
// TEST 055: the off-screen compose region around the clock face. The
// longest hand reaches 195 px plus its tip circle, so 212 covers it.
#define CLK_REG_X0   28
#define CLK_REG_Y0   28
#define CLK_REG_W    424
#define CLK_REG_H    424
// Area wiped before the temperature is redrawn (only when it changes).
#define TEMP_CLR_X   456
#define TEMP_CLR_Y   130
#define TEMP_CLR_W   344
#define TEMP_CLR_H   250
// TEST 052: Settings > Clock text. The tabview page uses LVGL's light
// theme (near white), so light colours vanish. Black reads cleanly.
#define SET_TEXT_COL 0x000000
bool haveDial = false;
// TEST 054: microphone state
bool  micReady   = false;
float micRms     = 0.0f;   // 0..1
float micPeak    = 0.0f;   // 0..1
uint32_t micReads = 0;
size_t   micSamples = 0;          // TEST 056: samples in the last read
lv_obj_t *micBar = NULL, *micLbl = NULL;

// TEST 056: utterance capture
int16_t  *uttBuf     = NULL;      // PSRAM, UTT_MAX_SAMPLES int16
size_t    uttLen     = 0;         // samples held in uttBuf (last complete word)
size_t    uttFill    = 0;         // samples written while capturing
bool      uttActive  = false;
uint32_t  uttStartMs = 0, uttQuietMs = 0;
size_t    uttFillAtQuiet = 0;     // fill level when the quiet period began
uint32_t  uttCount   = 0;         // words accepted since boot
uint32_t  uttLastMs  = 0;         // duration of the last accepted word
float     uttLastPeak = 0.0f;
float     micFloor   = 0.01f;     // adaptive noise floor (RMS units)
lv_obj_t *uttLbl     = NULL;
// TEST 057: continuous pre-roll ring
int16_t  *preBuf     = NULL;
size_t    preHead    = 0;         // next write position
size_t    preCount   = 0;         // valid samples, saturates at PRE_ROLL_SAMPLES
uint32_t  uttHist[UTT_HISTORY] = {0};
uint8_t   uttHistN   = 0;

// TEST 058: DSP working set
static float fftRe[MFCC_NFFT], fftIm[MFCC_NFFT];
static float hamWin[MFCC_FRAME];
static int   melBin[MFCC_NMEL + 2];
static float dctTab[MFCC_NCEP][MFCC_NMEL];
static bool  dspReady = false;
#define REF_SLOTS 3
float   *mfccCur = NULL;                    // PSRAM: MAX_FRAMES * NCEP
float   *mfccRefs[REF_SLOTS] = { NULL, NULL, NULL };
int      refFrames[REF_SLOTS] = { 0, 0, 0 };
int      curFrames = 0, refNext = 0, refCount = 0;
uint32_t refMs[REF_SLOTS] = { 0, 0, 0 };    // learned word lengths
uint32_t lastVoiceMs = 0;
float    wakeThreshold = WAKE_THRESHOLD;
uint32_t wakeCount = 0, wakeShownMs = 0;
lv_obj_t *learnLbl = NULL, *wakeBanner = NULL;
float   *dtwA = NULL, *dtwB = NULL;         // two rolling DP rows
float    lastDist = -1.0f;

// TEST 046: the dial is decoded once into PSRAM and reused every tick.
uint16_t *dialBuf   = NULL;      // 800*480 RGB565 = 768000 bytes
bool      dialCached = false;
int       dialBlocks = 0;        // TEST 049: blocks the decoder actually wrote
uint16_t *clockBuf  = NULL;      // TEST 055: off-screen compose buffer
char      dialStatus[64] = "dial: not checked";
#define WELCOME_MS 2000
uint32_t saverTimeoutMs = 300000UL;   // settings-adjustable
#define PHOTO_MS 2000

// ---- UART to bed box (P3 header) ----
#define BB_TX 17
#define BB_RX 18
enum Cmd { CMD_OFF = 0, CMD_ALL = 1, CMD_ZONE = 2,
           CMD_MOTOR = 3, CMD_PRESET = 4, CMD_TIMER = 5 };
uint8_t curBedId = 1;        // 1=bed1 2=bed2 0=both
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

// TEST 041: Files-tab slideshow. Runs inside ST_PREVIEW; a touch stops it.
bool     slideOn     = false;
int      slideIdx    = 0;
uint32_t slideNextMs = 0;
#define  SLIDE_MS 2000

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
// TEST 044: every display pointer starts NULL so the guards below can work.
lv_obj_t *scrHome=NULL, *scrMassage=NULL, *scrRadio=NULL, *scrAC=NULL,
         *scrSettings=NULL, *scrBlank=NULL, *scrClock=NULL;
lv_obj_t *clkGlow=NULL, *clkMain=NULL, *clkColon=NULL, *clkTemp=NULL;
// analog clock
lv_obj_t *anaScr=NULL, *anaFace=NULL, *anaHour=NULL, *anaMin=NULL,
         *anaSec=NULL, *anaTemp=NULL, *anaCenter=NULL;
lv_point_t hourPts[2], minPts[2], secPts[2];
lv_style_t stHour, stMin, stSec;
Preferences prefs;
int setMinSmall = 60, setMinBig = 165;   // motor thresholds (panel copy)
int randomChar = 1;                      // 0 gentle 1 lively 2 wild
// file browser
#define MAX_BROWSE 80
#define SETTINGS_TAB_FILES 3     // Clock 0, Display 1, Massage 2, Files 3
// TEST 047: limits that keep the Files list from exhausting LVGL's heap
#define BROWSE_MAX_SHOWN 40      // hard cap on rows
#define LV_FREE_FLOOR    10240   // stop if LVGL free heap drops below this
#define LV_BIGGEST_FLOOR 3072    // ...or if the largest free block is small
char browseList[MAX_BROWSE][80];
int browseCount = 0;
char previewPath[96];
lv_obj_t *lblSensors=NULL, *lblTestNum=NULL;
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
void learnReference();
void clearReferences();
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
static void evSlideshow(lv_event_t *e);   // TEST 041
void buildClock();
void updateClockFace();
void buildAnalog();
void updateAnalog();
void drawDiverClock(bool full = false);
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
// TEST 041: memory-safe. A too-big file or a failed allocation used to
// leave the panel frozen once many photos had been opened. Now the size is
// checked first, PSRAM is tried then ordinary heap, and the buffer is always
// freed on every exit path.
#define MAX_JPG_BYTES 600000UL

bool showPhoto(const char *path) {
  File f = SD.open(path, FILE_READ);
  if (!f) { Serial.printf("photo: cannot open %s\n", path); return false; }
  size_t sz = f.size();
  if (sz == 0 || sz > MAX_JPG_BYTES) {
    Serial.printf("photo: %s is %u bytes - skipped (limit %lu)\n",
                  path, (unsigned)sz, MAX_JPG_BYTES);
    f.close();
    return false;
  }
  uint8_t *buf = (uint8_t *)ps_malloc(sz);        // PSRAM first
  if (!buf) buf = (uint8_t *)malloc(sz);          // then ordinary heap
  if (!buf) {
    Serial.printf("photo: no memory for %u bytes (free heap %u)\n",
                  (unsigned)sz, (unsigned)ESP.getFreeHeap());
    f.close();
    return false;
  }
  size_t got = f.read(buf, sz);
  f.close();
  bool ok = false;
  if (got == sz && jpeg.openRAM(buf, sz, jpegDrawCb)) {
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    ok = jpeg.decode(0, 0, 0);
    jpeg.close();
  }
  free(buf);                                      // always freed
  buf = NULL;
  return ok;
}
// ============================================================
//  TEST 046: decode the dial once into a PSRAM framebuffer
// ============================================================
int jpegToBufCb(JPEGDRAW *p) {
  if (!dialBuf) return 0;
  dialBlocks++;                  // TEST 049: prove the decoder reached us
  for (int row = 0; row < p->iHeight; row++) {
    int y = p->y + row;
    if (y < 0 || y >= SCREEN_H) continue;
    for (int col = 0; col < p->iWidth; col++) {
      int x = p->x + col;
      if (x < 0 || x >= SCREEN_W) continue;
      dialBuf[(size_t)y * SCREEN_W + x] = p->pPixels[row * p->iWidth + col];
    }
  }
  return 1;
}

bool cacheDial() {
  dialCached = false;
  dialBlocks = 0;

  if (!dialBuf)
    dialBuf = (uint16_t *)ps_malloc((size_t)SCREEN_W * SCREEN_H * sizeof(uint16_t));
  if (!dialBuf) {
    snprintf(dialStatus, sizeof(dialStatus), "dial: no PSRAM for the 768 KB buffer");
    Serial.println(dialStatus); return false;
  }
  memset(dialBuf, 0, (size_t)SCREEN_W * SCREEN_H * sizeof(uint16_t));

  // TEST 053: the dial is compiled into the firmware. JPEGDEC wants a
  // writable pointer, so copy the 17 KB out of flash into heap first.
  uint8_t *srcbuf = (uint8_t *)malloc(DIAL_JPG_LEN);
  if (!srcbuf) {
    snprintf(dialStatus, sizeof(dialStatus), "dial: no heap for %d bytes", DIAL_JPG_LEN);
    Serial.println(dialStatus); return false;
  }
  memcpy_P(srcbuf, DIAL_JPG, DIAL_JPG_LEN);

  bool ok = false;
  if (jpeg.openRAM(srcbuf, DIAL_JPG_LEN, jpegToBufCb)) {
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    ok = jpeg.decode(0, 0, 0);
    jpeg.close();
  }
  free(srcbuf);

  dialCached = ok && (dialBlocks > 0);
  if (dialCached)
    snprintf(dialStatus, sizeof(dialStatus), "dial: flash ok, %d bytes, %d blocks",
             DIAL_JPG_LEN, dialBlocks);
  else if (ok)
    snprintf(dialStatus, sizeof(dialStatus), "dial: decoded but 0 blocks written");
  else
    snprintf(dialStatus, sizeof(dialStatus), "dial: flash decode FAILED");
  Serial.println(dialStatus);
  return dialCached;
}

// ============================================================
//  TEST 054: INMP441 microphone
// ============================================================
static int32_t micBuf[MIC_FRAMES];

bool micBegin() {
#if !ENABLE_MIC
  Serial.println("mic: disabled");
  return false;
#else
  if (ENABLE_SD) {
    Serial.println("mic: REFUSING to start - ENABLE_SD is 1 and shares GPIO 11/12/13");
    return false;
  }

  i2s_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate          = MIC_SAMPLE_RATE;
  cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT;   // INMP441 is 24-bit in 32-bit slots
  cfg.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;   // L/R tied to GND
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count        = 4;
  cfg.dma_buf_len          = MIC_FRAMES;
  cfg.use_apll             = false;
  cfg.tx_desc_auto_clear   = false;
  cfg.fixed_mclk           = 0;

  esp_err_t err = i2s_driver_install(MIC_PORT, &cfg, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("mic: i2s_driver_install failed (%d)\n", (int)err);
    return false;
  }

  // Every field assigned. mck_io_num is FIRST in this struct and a
  // designated initialiser that omits it silently selects GPIO0.
  i2s_pin_config_t pins;
  pins.mck_io_num   = I2S_PIN_NO_CHANGE;
  pins.bck_io_num   = MIC_BCK_PIN;
  pins.ws_io_num    = MIC_WS_PIN;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num  = MIC_DIN_PIN;

  err = i2s_set_pin(MIC_PORT, &pins);
  if (err != ESP_OK) {
    Serial.printf("mic: i2s_set_pin failed (%d)\n", (int)err);
    i2s_driver_uninstall(MIC_PORT);
    return false;
  }

  i2s_zero_dma_buffer(MIC_PORT);
  micReady = true;
  Serial.printf("mic: INMP441 ready - %d Hz, 32-bit, DIN=%d BCK=%d WS=%d\n",
                MIC_SAMPLE_RATE, MIC_DIN_PIN, MIC_BCK_PIN, MIC_WS_PIN);
  return true;
#endif
}

// Non-blocking: returns false if no full buffer was ready this tick.
bool micRead() {
  if (!micReady) return false;

  size_t got = 0;
  // 20 ms timeout: at 16 kHz a 256-frame buffer fills in 16 ms, so this
  // usually returns immediately and never stalls the UI.
  esp_err_t err = i2s_read(MIC_PORT, (void *)micBuf, sizeof(micBuf), &got,
                           20 / portTICK_PERIOD_MS);
  if (err != ESP_OK || got < sizeof(int32_t)) return false;

  size_t n = got / sizeof(int32_t);
  micSamples = n;                 // TEST 056: uttProcess appends exactly this many
  double sumsq = 0.0;
  int32_t peak = 0;
  for (size_t i = 0; i < n; i++) {
    // INMP441 puts 24 bits left-justified in a 32-bit slot. Shift down
    // to a signed 16-bit range so the numbers are readable.
    int32_t s = micBuf[i] >> 14;
    if (s >  32767) s =  32767;
    if (s < -32768) s = -32768;
    int32_t a = (s < 0) ? -s : s;
    if (a > peak) peak = a;
    sumsq += (double)s * (double)s;
  }
  micRms  = (float)(sqrt(sumsq / (double)n) / 32768.0);
  micPeak = (float)peak / 32768.0f;
  micReads++;
  return true;
}

// ============================================================
//  TEST 056: find a word in the stream
//  Called once per captured buffer, after micRead() has filled
//  micBuf and updated micRms / micPeak.
// ============================================================
// ============================================================
//  TEST 058: MFCC front end
//  Radix-2 FFT validated against numpy before being written here
//  (max error 3e-13 on 512 random points).
// ============================================================
static void fftRadix2(float *re, float *im, int n) {
  // bit reversal
  int j = 0;
  for (int i = 1; i < n; i++) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j |= bit;
    if (i < j) {
      float t = re[i]; re[i] = re[j]; re[j] = t;
      t = im[i]; im[i] = im[j]; im[j] = t;
    }
  }
  for (int len = 2; len <= n; len <<= 1) {
    float ang = -2.0f * (float)M_PI / (float)len;
    float wr = cosf(ang), wi = sinf(ang);
    for (int i = 0; i < n; i += len) {
      float cr = 1.0f, ci = 0.0f;
      for (int k = 0; k < len / 2; k++) {
        float ur = re[i + k],           ui = im[i + k];
        float xr = re[i + k + len / 2], xi = im[i + k + len / 2];
        float vr = xr * cr - xi * ci;
        float vi = xr * ci + xi * cr;
        re[i + k] = ur + vr;  im[i + k] = ui + vi;
        re[i + k + len / 2] = ur - vr;  im[i + k + len / 2] = ui - vi;
        float ncr = cr * wr - ci * wi;
        ci = cr * wi + ci * wr;
        cr = ncr;
      }
    }
  }
}

static inline float hz2mel(float f) { return 2595.0f * log10f(1.0f + f / 700.0f); }
static inline float mel2hz(float m) { return 700.0f * (powf(10.0f, m / 2595.0f) - 1.0f); }

void dspInit() {
  for (int i = 0; i < MFCC_FRAME; i++)
    hamWin[i] = 0.54f - 0.46f * cosf(2.0f * (float)M_PI * i / (MFCC_FRAME - 1));

  float lo = hz2mel(MFCC_FMIN), hi = hz2mel(MFCC_FMAX);
  for (int i = 0; i < MFCC_NMEL + 2; i++) {
    float hz = mel2hz(lo + (hi - lo) * i / (MFCC_NMEL + 1));
    melBin[i] = (int)floorf((MFCC_NFFT + 1) * hz / (float)MIC_SAMPLE_RATE);
    if (melBin[i] > MFCC_NFFT / 2) melBin[i] = MFCC_NFFT / 2;
  }

  for (int c = 0; c < MFCC_NCEP; c++)
    for (int m = 0; m < MFCC_NMEL; m++)
      dctTab[c][m] = cosf((float)M_PI * (c + 1) * (m + 0.5f) / MFCC_NMEL);

  size_t fbytes = (size_t)MFCC_MAX_FRAMES * MFCC_NCEP * sizeof(float);
  mfccCur = (float *)ps_malloc(fbytes);
  bool refsOk = true;
  for (int i = 0; i < REF_SLOTS; i++) {
    mfccRefs[i] = (float *)ps_malloc(fbytes);
    if (!mfccRefs[i]) refsOk = false;
  }
  dtwA = (float *)ps_malloc((size_t)(MFCC_MAX_FRAMES + 1) * sizeof(float));
  dtwB = (float *)ps_malloc((size_t)(MFCC_MAX_FRAMES + 1) * sizeof(float));

  dspReady = mfccCur && refsOk && dtwA && dtwB;
  Serial.printf("dsp: %s  (%u bytes of features, %d reference slots)\n",
                dspReady ? "ready" : "ALLOCATION FAILED",
                (unsigned)((REF_SLOTS + 1) * fbytes), REF_SLOTS);
}

// Returns the number of frames written into out[].
int mfccExtract(const int16_t *sig, size_t len, float *out) {
  if (!dspReady || len < MFCC_FRAME) return 0;

  int frames = 0;
  for (size_t start = 0;
       start + MFCC_FRAME <= len && frames < MFCC_MAX_FRAMES;
       start += MFCC_HOP) {

    for (int i = 0; i < MFCC_NFFT; i++) { fftRe[i] = 0.0f; fftIm[i] = 0.0f; }
    // pre-emphasis inside the window, then Hamming
    for (int i = 0; i < MFCC_FRAME; i++) {
      size_t k = start + i;
      float prev = (k > 0) ? (float)sig[k - 1] : 0.0f;
      fftRe[i] = ((float)sig[k] - 0.97f * prev) * hamWin[i];
    }
    fftRadix2(fftRe, fftIm, MFCC_NFFT);

    float *dst = out + (size_t)frames * MFCC_NCEP;
    float mel[MFCC_NMEL];
    for (int m = 1; m <= MFCC_NMEL; m++) {
      int l = melBin[m - 1], c = melBin[m], r = melBin[m + 1];
      float s = 0.0f;
      for (int k = l; k < c; k++) {
        float p = (fftRe[k] * fftRe[k] + fftIm[k] * fftIm[k]) / MFCC_NFFT;
        if (c > l) s += p * (float)(k - l) / (float)(c - l);
      }
      for (int k = c; k < r; k++) {
        float p = (fftRe[k] * fftRe[k] + fftIm[k] * fftIm[k]) / MFCC_NFFT;
        if (r > c) s += p * (float)(r - k) / (float)(r - c);
      }
      mel[m - 1] = logf(s + 1e-10f);
    }
    for (int c = 0; c < MFCC_NCEP; c++) {
      float acc = 0.0f;
      for (int m = 0; m < MFCC_NMEL; m++) acc += dctTab[c][m] * mel[m];
      dst[c] = acc;
    }
    frames++;
  }

  // cepstral mean normalisation: removes the constant colour of the
  // microphone and the room, and most of the effect of loudness
  for (int c = 0; c < MFCC_NCEP; c++) {
    float mean = 0.0f;
    for (int f = 0; f < frames; f++) mean += out[(size_t)f * MFCC_NCEP + c];
    mean /= (float)frames;
    for (int f = 0; f < frames; f++) out[(size_t)f * MFCC_NCEP + c] -= mean;
  }
  return frames;
}

// Dynamic time warping, normalised by path length so long and short
// words are comparable.
float dtwDistance(const float *A, int n, const float *B, int m) {
  if (!dspReady || n < 2 || m < 2) return -1.0f;
  const float INF = 3.4e38f;
  float *prev = dtwA, *cur = dtwB;
  for (int j = 0; j <= m; j++) prev[j] = INF;
  prev[0] = 0.0f;

  for (int i = 1; i <= n; i++) {
    cur[0] = INF;
    const float *a = A + (size_t)(i - 1) * MFCC_NCEP;
    for (int j = 1; j <= m; j++) {
      const float *b = B + (size_t)(j - 1) * MFCC_NCEP;
      float d = 0.0f;
      for (int c = 0; c < MFCC_NCEP; c++) { float e = a[c] - b[c]; d += e * e; }
      d = sqrtf(d);
      float best = prev[j];
      if (cur[j - 1] < best) best = cur[j - 1];
      if (prev[j - 1] < best) best = prev[j - 1];
      cur[j] = d + best;
    }
    float *t = prev; prev = cur; cur = t;
  }
  return prev[m] / (float)(n + m);
}

// TEST 059: store the last word as a reference. Slots cycle, so pressing
// Learn four times keeps the three most recent.
void learnReference() {
  if (!dspReady || curFrames <= 0) {
    Serial.println("learn: say the word first");
    return;
  }
  memcpy(mfccRefs[refNext], mfccCur, (size_t)curFrames * MFCC_NCEP * sizeof(float));
  refFrames[refNext] = curFrames;
  refMs[refNext]     = lastVoiceMs;
  refNext = (refNext + 1) % REF_SLOTS;
  if (refCount < REF_SLOTS) refCount++;
  Serial.printf("learn: stored %d frames (%u ms), %d/%d slots filled\n",
                curFrames, (unsigned)lastVoiceMs, refCount, REF_SLOTS);
  if (learnLbl) lv_label_set_text_fmt(learnLbl, "Learn %d/%d", refCount, REF_SLOTS);
}

void clearReferences() {
  for (int i = 0; i < REF_SLOTS; i++) { refFrames[i] = 0; refMs[i] = 0; }
  refNext = 0; refCount = 0;
  Serial.println("learn: references cleared");
  if (learnLbl) lv_label_set_text_fmt(learnLbl, "Learn 0/%d", REF_SLOTS);
}

void uttProcess() {
  if (!uttBuf || !preBuf) return;

  const uint32_t now = millis();
  const float startTh = fmaxf(micFloor * UTT_START_MULT, UTT_ABS_FLOOR);
  const float stopTh  = fmaxf(micFloor * UTT_STOP_MULT,  UTT_ABS_FLOOR * 0.7f);

  // ---- TEST 057: the ring runs ALWAYS, speaking or not ----
  for (size_t i = 0; i < micSamples; i++) {
    int32_t s = micBuf[i] >> 14;
    if (s >  32767) s =  32767;
    if (s < -32768) s = -32768;
    preBuf[preHead] = (int16_t)s;
    preHead = (preHead + 1) % PRE_ROLL_SAMPLES;
    if (preCount < PRE_ROLL_SAMPLES) preCount++;
  }

  if (!uttActive) {
    micFloor = micFloor * 0.98f + micRms * 0.02f;   // track the room while quiet
    if (micRms <= startTh) return;

    // Speech. Start the recording with the pre-roll, oldest sample first.
    uttActive   = true;
    uttStartMs  = now;
    uttQuietMs  = 0;
    uttFillAtQuiet = 0;
    uttLastPeak = micPeak;
    uttFill     = 0;

    size_t start = (preHead + PRE_ROLL_SAMPLES - preCount) % PRE_ROLL_SAMPLES;
    for (size_t i = 0; i < preCount && uttFill < UTT_BUF_SAMPLES; i++) {
      uttBuf[uttFill++] = preBuf[(start + i) % PRE_ROLL_SAMPLES];
    }
    // the pre-roll already contains this buffer's samples - do not add them twice
  } else {
    for (size_t i = 0; i < micSamples && uttFill < UTT_BUF_SAMPLES; i++) {
      int32_t s = micBuf[i] >> 14;
      if (s >  32767) s =  32767;
      if (s < -32768) s = -32768;
      uttBuf[uttFill++] = (int16_t)s;
    }
    if (micPeak > uttLastPeak) uttLastPeak = micPeak;
  }

  bool tooLong = (uttFill >= UTT_BUF_SAMPLES) || ((now - uttStartMs) > UTT_MAX_MS);

  if (micRms < stopTh) {
    if (uttQuietMs == 0) { uttQuietMs = now; uttFillAtQuiet = uttFill; }
  } else {
    uttQuietMs = 0;
  }
  bool ended = (uttQuietMs && (now - uttQuietMs) >= UTT_HANG_MS);

  if (!ended && !tooLong) return;

  uttActive = false;
  // trim the hang time, but never below the pre-roll we deliberately kept
  if (ended && uttFillAtQuiet > PRE_ROLL_SAMPLES && uttFillAtQuiet < uttFill)
    uttFill = uttFillAtQuiet;

  uint32_t totalMs = (uint32_t)((uint64_t)uttFill * 1000ULL / MIC_SAMPLE_RATE);
  uint32_t voiceMs = (totalMs > PRE_ROLL_MS) ? (totalMs - PRE_ROLL_MS) : 0;

  if (voiceMs < UTT_MIN_MS) {
    Serial.printf("utt: rejected, %u ms of voice is too short\n", (unsigned)voiceMs);
    return;
  }

  uttLen    = uttFill;
  uttLastMs = totalMs;
  uttCount++;
  uttHist[uttHistN % UTT_HISTORY] = voiceMs;
  uttHistN++;

  Serial.printf("utt #%u: voice %u ms (+%d pre-roll = %u total), %u samples, "
                "peak %.3f, floor %.4f%s\n",
                (unsigned)uttCount, (unsigned)voiceMs, PRE_ROLL_MS,
                (unsigned)totalMs, (unsigned)uttLen, uttLastPeak, micFloor,
                tooLong ? "  (hit the limit)" : "");
  lastVoiceMs = voiceMs;

  // ---- TEST 058: features, and a distance if a reference exists ----
  uint32_t t0 = millis();
  curFrames = mfccExtract(uttBuf, uttLen, mfccCur);
  uint32_t tMfcc = millis() - t0;
  lastDist = -1.0f;

  // TEST 061: duration gate. Cheaper than DTW and it removes the
  // obviously-wrong lengths - sentences, coughs - before the maths.
  bool durOk = true;
  if (refCount > 0) {
    uint32_t sum = 0; int n = 0;
    for (int i = 0; i < REF_SLOTS; i++) if (refMs[i]) { sum += refMs[i]; n++; }
    if (n > 0) {
      float ratio = (float)voiceMs / ((float)sum / (float)n);
      if (ratio < WAKE_DUR_MIN || ratio > WAKE_DUR_MAX) {
        durOk = false;
        Serial.printf("     mfcc %d frames (%u ms), length %.2fx of learned - "
                      "rejected before DTW\n",
                      curFrames, (unsigned)tMfcc, ratio);
      }
    }
  }

  if (curFrames > 0 && refCount > 0 && durOk) {
    t0 = millis();
    float best = -1.0f; int bestSlot = -1;
    char per[80]; int off = 0;
    for (int i = 0; i < REF_SLOTS; i++) {
      if (refFrames[i] <= 0) continue;
      float d = dtwDistance(mfccCur, curFrames, mfccRefs[i], refFrames[i]);
      if (d >= 0.0f && (best < 0.0f || d < best)) { best = d; bestSlot = i; }
      if (off < (int)sizeof(per) - 12)
        off += snprintf(per + off, sizeof(per) - off, " %.2f", d);
    }
    lastDist = best;
    bool hit = (best >= 0.0f && best < wakeThreshold);
    Serial.printf("     mfcc %d frames (%u ms), dtw%s -> best %.3f (slot %d, %u ms)  %s\n",
                  curFrames, (unsigned)tMfcc, per, best, bestSlot,
                  (unsigned)(millis() - t0),
                  hit ? "*** WAKE WORD ***" : "(no match)");
    if (hit) {
      wakeCount++;
      Serial.printf("WAKE #%u  distance %.3f  threshold %.2f\n",
                    (unsigned)wakeCount, best, wakeThreshold);
      wakeShownMs = millis();
      if (wakeBanner) lv_obj_clear_flag(wakeBanner, LV_OBJ_FLAG_HIDDEN);
      if (state == ST_SAVER) exitSaver();     // wake the screen
    }
  } else if (!durOk) {
    /* already reported */
  } else {
    Serial.printf("     mfcc %d frames (%u ms), no reference yet - press Learn\n",
                  curFrames, (unsigned)tMfcc);
  }

  if (uttLbl) {
    if (lastDist >= 0.0f)
      lv_label_set_text_fmt(uttLbl, "word #%u  %u ms   d=%d.%02d",
                            (unsigned)uttCount, (unsigned)voiceMs,
                            (int)lastDist, (int)((lastDist - (int)lastDist) * 100));
    else
      lv_label_set_text_fmt(uttLbl, "word #%u  %u ms", (unsigned)uttCount,
                            (unsigned)voiceMs);
  }
}

// Print the last utterance as base64 so it can be copied off the board.
void uttDump() {
  if (!uttBuf || uttLen == 0) { Serial.println("utt: nothing captured yet"); return; }

  static const char *B64 =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const uint8_t *p = (const uint8_t *)uttBuf;
  size_t bytes = uttLen * sizeof(int16_t);

  Serial.printf("utt-dump begin  samples=%u  rate=%d  bits=16  le=1\n",
                (unsigned)uttLen, MIC_SAMPLE_RATE);
  size_t i = 0, col = 0;
  while (i < bytes) {
    uint32_t v = (uint32_t)p[i] << 16;
    int have = 1;
    if (i + 1 < bytes) { v |= (uint32_t)p[i+1] << 8; have = 2; }
    if (i + 2 < bytes) { v |= (uint32_t)p[i+2];      have = 3; }
    char q[4];
    q[0] = B64[(v >> 18) & 0x3F];
    q[1] = B64[(v >> 12) & 0x3F];
    q[2] = (have > 1) ? B64[(v >> 6) & 0x3F] : '=';
    q[3] = (have > 2) ? B64[v & 0x3F]        : '=';
    Serial.write(q, 4);
    i += 3;
    if ((col += 4) >= 76) { Serial.println(); col = 0; }
  }
  if (col) Serial.println();
  Serial.println("utt-dump end");
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

  // ---- TEST 054: microphone level meter along the bottom ----
  micLbl = lv_label_create(scrHome);
  lv_label_set_text(micLbl, "mic --");
  lv_obj_set_style_text_font(micLbl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(micLbl, lv_color_hex(0x90C0E0), 0);
  lv_obj_align(micLbl, LV_ALIGN_BOTTOM_LEFT, 20, -12);

  uttLbl = lv_label_create(scrHome);
  lv_label_set_text(uttLbl, "no word yet");
  lv_obj_set_style_text_font(uttLbl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(uttLbl, lv_color_hex(0x8FE0A0), 0);
  lv_obj_align(uttLbl, LV_ALIGN_BOTTOM_LEFT, 20, -44);

  // TEST 059: Learn button - no serial monitor needed
  lv_obj_t *learnBtn = lv_btn_create(scrHome);
  lv_obj_set_size(learnBtn, 160, 44);
  lv_obj_align(learnBtn, LV_ALIGN_BOTTOM_RIGHT, -20, -44);
  lv_obj_set_style_bg_color(learnBtn, lv_color_hex(0x2E6F4E), 0);
  lv_obj_add_event_cb(learnBtn, [](lv_event_t *e){ learnReference(); },
                      LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(learnBtn, [](lv_event_t *e){ clearReferences(); },
                      LV_EVENT_LONG_PRESSED, NULL);
  learnLbl = lv_label_create(learnBtn);
  lv_label_set_text_fmt(learnLbl, "Learn 0/%d", REF_SLOTS);
  lv_obj_set_style_text_font(learnLbl, &lv_font_montserrat_20, 0);
  lv_obj_center(learnLbl);

  // TEST 061: shown briefly when the wake word is recognised
  wakeBanner = lv_label_create(scrHome);
  lv_label_set_text(wakeBanner, "WAKE WORD");
  lv_obj_set_style_text_font(wakeBanner, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(wakeBanner, lv_color_hex(0x40E080), 0);
  lv_obj_align(wakeBanner, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(wakeBanner, LV_OBJ_FLAG_HIDDEN);

  micBar = lv_bar_create(scrHome);
  lv_obj_set_size(micBar, 560, 22);
  lv_obj_align(micBar, LV_ALIGN_BOTTOM_RIGHT, -20, -12);
  lv_bar_set_range(micBar, 0, 100);
  lv_bar_set_value(micBar, 0, LV_ANIM_OFF);
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
  lv_obj_align(title, LV_ALIGN_TOP_MID, -60, 20);

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
  if (!list) return;
  lv_obj_clean(list);
#if !ENABLE_SD
  // TEST 053: those pins are the microphone now. Do not touch SPI.
  lv_list_add_text(list, "SD disabled - GPIO 11/12/13 are the microphone");
  return;
#else
  bool truncated = false;
  const char *dirs[2] = { "/", "/photos" };
  for (int di = 0; di < 2 && !truncated; di++) {
    File dir = SD.open(dirs[di]);
    if (!dir || !dir.isDirectory()) continue;
    File e;
    while ((e = dir.openNextFile()) && browseCount < MAX_BROWSE) {
      if (!e.isDirectory()) {
        // TEST 047: never ask LVGL for a button it has no room for.
        // This exact call is what crashed TEST 046 inside lv_list_add_btn.
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        if (browseCount >= BROWSE_MAX_SHOWN ||
            mon.free_size < LV_FREE_FLOOR ||
            mon.free_biggest_size < LV_BIGGEST_FLOOR) {
          truncated = true;
          e.close();
          break;
        }
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
  if (truncated)
    lv_list_add_text(list, "... more files not shown (low memory)");
  else if (browseCount == 0)
    lv_list_add_text(list, "SD empty / not mounted");

  lv_mem_monitor_t mon; lv_mem_monitor(&mon);
  Serial.printf("files: %d rows%s, LVGL free %u, biggest %u\n",
                browseCount, truncated ? " (truncated)" : "",
                (unsigned)mon.free_size, (unsigned)mon.free_biggest_size);
#endif // ENABLE_SD
}

// TEST 047: build the Files list only while that tab is on screen.
static void evSettingsTab(lv_event_t *e) {
  lv_obj_t *tv = lv_event_get_target(e);
  if (!tv || !fileList) return;
  if (lv_tabview_get_tab_act(tv) == SETTINGS_TAB_FILES) {
    populateFiles(fileList);
  } else if (browseCount > 0) {
    lv_obj_clean(fileList);
    browseCount = 0;
  }
}

// ============================================================
//  Settings screen
// ============================================================
lv_obj_t *lblFloorVal=NULL, *ddSaver=NULL, *sldSmall=NULL, *sldBig=NULL,
         *lblSmall=NULL, *lblBig=NULL, *ddRandom=NULL;

// TEST 042: the LVGL rollers never worked (minute roller stayed empty, hour
// scrolled one way only). Replaced with plain +/- buttons and 12-hour AM/PM.
lv_obj_t *lblSetH=NULL, *lblSetM=NULL, *lblSetAP=NULL;
int setHour12 = 12;      // 1..12
int setMinute = 0;       // 0..59
bool setPM    = false;
void refreshSetterLabels();

void refreshSetterLabels() {
  if (!lblSetH || !lblSetM || !lblSetAP) return;   // TEST 044 guard
  lv_label_set_text_fmt(lblSetH, "%d", setHour12);
  lv_label_set_text_fmt(lblSetM, "%02d", setMinute);
  lv_label_set_text(lblSetAP, setPM ? "PM" : "AM");
}
static void evHourUp(lv_event_t *e) { setHour12++; if (setHour12 > 12) setHour12 = 1;  refreshSetterLabels(); }
static void evHourDn(lv_event_t *e) { setHour12--; if (setHour12 < 1)  setHour12 = 12; refreshSetterLabels(); }
static void evMinUp (lv_event_t *e) { setMinute++; if (setMinute > 59) setMinute = 0;  refreshSetterLabels(); }
static void evMinDn (lv_event_t *e) { setMinute--; if (setMinute < 0)  setMinute = 59; refreshSetterLabels(); }
static void evAmPm  (lv_event_t *e) { setPM = !setPM;                                  refreshSetterLabels(); }

static void evSetTime(lv_event_t *e) {
  int h24 = setHour12 % 12;              // 12 AM -> 0, 12 PM -> 12
  if (setPM) h24 += 12;
  baseSecOfDay = (long)h24*3600 + (long)setMinute*60;
  baseMillis = millis(); timeSet = true;
  if (haveDS3231) ds3231SetHM(h24, setMinute);
  Serial.printf("time set %d:%02d %s  (%02d:%02d 24h)\n",
                setHour12, setMinute, setPM ? "PM" : "AM", h24, setMinute);
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
static void evSlideshow(lv_event_t *e) {          // TEST 041
  if (photoCount == 0) { Serial.println("slideshow: no photos"); return; }
  slideOn     = true;
  slideIdx    = 0;
  slideNextMs = 0;                                // show the first one at once
  state       = ST_PREVIEW;
  lv_scr_load(scrBlank);
  lv_refr_now(NULL);
  Serial.printf("slideshow: %d photos, %d ms each\n", photoCount, SLIDE_MS);
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
  int cx=CLOCK_CX, cy=CLOCK_CY;
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

// ============================================================
//  TEST 055: rasterisers that draw into clockBuf, not the screen.
//  Coordinates are region-local (subtract CLK_REG_X0 / CLK_REG_Y0).
// ============================================================
static void cbFillCircle(int cx, int cy, int r, uint16_t col) {
  if (!clockBuf) return;
  for (int dy = -r; dy <= r; dy++) {
    int y = cy + dy;
    if (y < 0 || y >= CLK_REG_H) continue;
    int span = (int)(sqrtf((float)(r * r - dy * dy)) + 0.5f);
    int x0 = cx - span, x1 = cx + span;
    if (x0 < 0) x0 = 0;
    if (x1 >= CLK_REG_W) x1 = CLK_REG_W - 1;
    uint16_t *row = &clockBuf[(size_t)y * CLK_REG_W];
    for (int x = x0; x <= x1; x++) row[x] = col;
  }
}

static void cbThickLine(int x0, int y0, int x1, int y1, int r, uint16_t col) {
  int dx = abs(x1 - x0), dy = abs(y1 - y0);
  int steps = (dx > dy) ? dx : dy;
  if (steps < 1) steps = 1;
  for (int i = 0; i <= steps; i++) {
    cbFillCircle(x0 + (x1 - x0) * i / steps,
                 y0 + (y1 - y0) * i / steps, r, col);
  }
}

static void cbDrawHand(float ang, int lenTail, int lenTip, int r,
                       uint16_t glow, uint16_t core) {
  int cx = CLOCK_CX - CLK_REG_X0, cy = CLOCK_CY - CLK_REG_Y0;
  int tx = cx + (int)(sinf(ang) * lenTip);
  int ty = cy - (int)(cosf(ang) * lenTip);
  int bx = cx - (int)(sinf(ang) * lenTail);
  int by = cy + (int)(cosf(ang) * lenTail);
  cbThickLine(bx, by, tx, ty, r + 3, glow);
  cbThickLine(bx, by, tx, ty, r, core);
  int ax = cx + (int)(sinf(ang) * (lenTip - 18));
  int ay = cy - (int)(cosf(ang) * (lenTip - 18));
  cbFillCircle(tx, ty, r + 5, glow);
  cbFillCircle(ax, ay, r + 3, core);
}

void drawDiverClock(bool full){
  static int lastTempInt = -999;
  bool compose = (dialCached && dialBuf && clockBuf);

  // TEST 055: the full-screen background is painted only when entering
  // the screensaver, not once a second.
  if (full || !compose) {
    if (dialCached && dialBuf) {
      gfx->draw16bitRGBBitmap(0, 0, dialBuf, SCREEN_W, SCREEN_H);
    } else if (haveDial) {
      if (!showPhoto(DIAL_FILE)) gfx->fillScreen(BLACK);
    } else {
      gfx->fillScreen(0x0842);
    }
    lastTempInt = -999;            // force the temperature back on screen
  }

  long s = nowSecOfDay(); if (s<0) s=0;
  int hh=(s/3600)%12, mm=(s/60)%60, ss=s%60;
  const float D=3.14159265f/180.0f;
  const uint16_t GCORE=0x2586;   // TEST 043: green at ~70%
  const uint16_t GGLOW=0x0844;   // dimmer glow to match

  if (compose) {
    // TEST 055: rebuild the clock area off-screen - clean dial, then
    // hands on top - and push the finished image in a single blit.
    for (int y = 0; y < CLK_REG_H; y++) {
      memcpy(&clockBuf[(size_t)y * CLK_REG_W],
             &dialBuf[(size_t)(CLK_REG_Y0 + y) * SCREEN_W + CLK_REG_X0],
             (size_t)CLK_REG_W * sizeof(uint16_t));
    }
    cbDrawHand((hh*30+mm*0.5f)*D, 22, 120, 7, GGLOW, GCORE);
    cbDrawHand((mm*6+ss*0.1f)*D,  26, 175, 5, GGLOW, GCORE);
    cbDrawHand((ss*6)*D,          40, 195, 2, 0x1902, 0xA800);
    cbFillCircle(CLOCK_CX - CLK_REG_X0, CLOCK_CY - CLK_REG_Y0, 10, GCORE);
    cbFillCircle(CLOCK_CX - CLK_REG_X0, CLOCK_CY - CLK_REG_Y0,  5, 0xFFFF);
    gfx->draw16bitRGBBitmap(CLK_REG_X0, CLK_REG_Y0, clockBuf,
                            CLK_REG_W, CLK_REG_H);
  } else {
    // fallback: straight to the screen, as TEST 054 did
    drawHand((hh*30+mm*0.5f)*D, 22, 120, 7, GGLOW, GCORE);
    drawHand((mm*6+ss*0.1f)*D,  26, 175, 5, GGLOW, GCORE);
    drawHand((ss*6)*D, 40, 195, 2, 0x1902, 0xA800);
    gfx->fillCircle(CLOCK_CX,CLOCK_CY,10,GCORE);
    gfx->fillCircle(CLOCK_CX,CLOCK_CY,5,0xFFFF);
  }

  // TEST 049: if the dial is not on screen, say why, on the screen.
  if (full && !dialCached) {
    gfx->setTextColor(0xFFE0, BLACK);
    gfx->setTextSize(2);
    gfx->setCursor(10, SCREEN_H - 26);
    gfx->print(dialStatus);
  }

  // TEST 046: temperature in the right-hand margin, whole degrees,
  // large enough to read from the bed without glasses.
  // Character cell is 6*size wide by 8*size tall, so size 9 = 54 x 72 px.
  if (tempOK) {
    int tInt = (int)lroundf(gTemp);
    if (tInt < -99) tInt = -99;
    if (tInt >  99) tInt =  99;
    // TEST 055: repaint only when the whole number actually changes.
    if (tInt == lastTempInt) return;
    lastTempInt = tInt;
    gfx->fillRect(TEMP_CLR_X, TEMP_CLR_Y, TEMP_CLR_W, TEMP_CLR_H, BLACK);

    char t[8]; snprintf(t, sizeof(t), "%d", tInt);

    // Two digits get the big size; a three-character reading such as
    // "-12" drops a size so the block stays the same overall width.
    // TEST 051: real font. Offsets in temp_font.h are measured from the
    // top of the digits (not the baseline - that overflowed int8_t), so
    // the cursor y is the digit top. getTextBounds() handles either
    // convention, so the placement maths below does not care.
    int16_t bx, by, cxb, cyb;
    uint16_t bw, bh, cw, chh;

    gfx->setTextSize(1);
    gfx->setFont(&TempBig);
    gfx->getTextBounds(t, 0, 0, &bx, &by, &bw, &bh);
    gfx->setFont(&TempSmall);
    gfx->getTextBounds("C", 0, 0, &cxb, &cyb, &cw, &chh);

    int blockW = (int)bw + TEMP_C_GAP + (int)cw;
    int left   = TEMP_CX - blockW / 2;      // digits + gap + C, centred
    int top    = CLOCK_CY - (int)bh / 2;    // level with the middle of the dial
    if (left < 0) left = 0;
    if (left + blockW > SCREEN_W) left = SCREEN_W - blockW;

    gfx->setTextColor(0x37E9);              // no bg: the dial is redrawn anyway
    gfx->setFont(&TempBig);
    gfx->setCursor(left - bx, top - by);
    gfx->print(t);

    // C to the RIGHT of the digits, sharing their bottom line
    gfx->setFont(&TempSmall);
    gfx->setCursor(left + (int)bw + TEMP_C_GAP - cxb,
                   top + (int)bh - (int)chh - cyb);
    gfx->print("C");

    gfx->setFont(NULL);                     // back to the built-in font
    gfx->setTextSize(1);
  }
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
  lv_style_set_line_color(&stHour, lv_color_hex(0xA2712A));  // TEST 043: ~70%
  lv_style_set_line_rounded(&stHour, true);
  lv_style_init(&stMin); lv_style_set_line_width(&stMin, 11);
  lv_style_set_line_color(&stMin, lv_color_hex(0x86A7B2));   // TEST 043: ~70%
  lv_style_set_line_rounded(&stMin, true);
  lv_style_init(&stSec); lv_style_set_line_width(&stSec, 4);
  lv_style_set_line_color(&stSec, lv_color_hex(0xA8433B));   // TEST 043: ~70%
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
  if (!anaHour || !anaMin || !anaSec) return;      // TEST 044 guard
  long s = nowSecOfDay();
  if (s < 0) s = 0;
  int hh = (s/3600) % 12, mm = (s/60)%60, ss = s%60;
  const float D = 3.14159265f / 180.0f;
  setHand(anaHour, hourPts, (hh*30 + mm*0.5f) * D, ANA_R - 115);
  setHand(anaMin,  minPts,  (mm*6 + ss*0.1f)  * D, ANA_R - 45);
  setHand(anaSec,  secPts,  (ss*6)            * D, ANA_R - 30);
  if (tempOK) { char t[16]; snprintf(t, sizeof(t), "%d C", (int)lroundf(gTemp)); lv_label_set_text(anaTemp, t); }
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
  if (!clkMain || !clkGlow) return;               // TEST 044 guard
  long s = nowSecOfDay();
  char buf[16];
  if (s >= 0) {                                   // TEST 042: 12-hour + AM/PM
    int h24 = (int)(s/3600), mm = (int)((s/60)%60);
    int h12 = h24 % 12; if (h12 == 0) h12 = 12;
    snprintf(buf, sizeof(buf), "%d:%02d %s", h12, mm, (h24 >= 12) ? "PM" : "AM");
  } else snprintf(buf, sizeof(buf), "--:--");
  lv_label_set_text(clkMain, buf);
  lv_label_set_text(clkGlow, buf);
  if (tempOK) { char t[16]; snprintf(t, sizeof(t), "%d C", (int)lroundf(gTemp)); lv_label_set_text(clkTemp, t); }
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
  // TEST 047: Clock 0, Display 1, Massage 2, Files 3, About 4
  lv_obj_add_event_cb(tv, evSettingsTab, LV_EVENT_VALUE_CHANGED, NULL);

  // ---- Clock ----

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

  // ---- TEST 042: +/- fields.  [-] H [+]  :  [-] MM [+]   [AM/PM] ----
  // helper-free on purpose: three explicit blocks, easy to read and tweak.
  lv_obj_t *bHd = lv_btn_create(row); lv_obj_set_size(bHd, 64, 64);
  lv_obj_add_event_cb(bHd, evHourDn, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lHd = lv_label_create(bHd); lv_label_set_text(lHd, "-");
  lv_obj_set_style_text_font(lHd, &lv_font_montserrat_28, 0); lv_obj_center(lHd);

  lblSetH = lv_label_create(row); lv_label_set_text(lblSetH, "12");
  lv_obj_set_style_text_font(lblSetH, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(lblSetH, lv_color_hex(SET_TEXT_COL), 0);

  lv_obj_t *bHu = lv_btn_create(row); lv_obj_set_size(bHu, 64, 64);
  lv_obj_add_event_cb(bHu, evHourUp, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lHu = lv_label_create(bHu); lv_label_set_text(lHu, "+");
  lv_obj_set_style_text_font(lHu, &lv_font_montserrat_28, 0); lv_obj_center(lHu);

  lv_obj_t *cln = lv_label_create(row); lv_label_set_text(cln, ":");
  lv_obj_set_style_text_font(cln, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(cln, lv_color_hex(SET_TEXT_COL), 0);

  lv_obj_t *bMd = lv_btn_create(row); lv_obj_set_size(bMd, 64, 64);
  lv_obj_add_event_cb(bMd, evMinDn, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lMd = lv_label_create(bMd); lv_label_set_text(lMd, "-");
  lv_obj_set_style_text_font(lMd, &lv_font_montserrat_28, 0); lv_obj_center(lMd);

  lblSetM = lv_label_create(row); lv_label_set_text(lblSetM, "00");
  lv_obj_set_style_text_font(lblSetM, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(lblSetM, lv_color_hex(SET_TEXT_COL), 0);

  lv_obj_t *bMu = lv_btn_create(row); lv_obj_set_size(bMu, 64, 64);
  lv_obj_add_event_cb(bMu, evMinUp, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lMu = lv_label_create(bMu); lv_label_set_text(lMu, "+");
  lv_obj_set_style_text_font(lMu, &lv_font_montserrat_28, 0); lv_obj_center(lMu);

  lv_obj_t *bAP = lv_btn_create(row); lv_obj_set_size(bAP, 92, 64);
  lv_obj_set_style_bg_color(bAP, lv_color_hex(0x244055), 0);
  lv_obj_add_event_cb(bAP, evAmPm, LV_EVENT_CLICKED, NULL);
  lblSetAP = lv_label_create(bAP); lv_label_set_text(lblSetAP, "AM");
  lv_obj_set_style_text_font(lblSetAP, &lv_font_montserrat_28, 0); lv_obj_center(lblSetAP);

  // seed the fields from the clock we are already keeping
  { long s = nowSecOfDay();
    if (s >= 0) {
      int h24 = (int)(s/3600), mm = (int)((s/60)%60);
      setPM     = (h24 >= 12);
      setHour12 = h24 % 12; if (setHour12 == 0) setHour12 = 12;
      setMinute = mm;
    }
  }
  refreshSetterLabels();

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
  lv_obj_set_size(fileList, 760, 290);
  lv_obj_align(fileList, LV_ALIGN_TOP_MID, 0, 0);
  // TEST 047: NOT populated here. Building ~42 list buttons at boot is what
  // exhausted LVGL's heap and crashed. evSettingsTab fills it on demand.
  lv_list_add_text(fileList, "Open this tab to list the card");

  // TEST 041: slideshow of every photo in /photos, 2 s each, touch to stop
  lv_obj_t *slb = lv_btn_create(tFiles);
  lv_obj_set_size(slb, 320, 56);
  lv_obj_align(slb, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_obj_set_style_bg_color(slb, lv_color_hex(0x246080), 0);
  lv_obj_add_event_cb(slb, evSlideshow, LV_EVENT_CLICKED, NULL);
  lv_obj_t *sll = lv_label_create(slb);
  lv_label_set_text(sll, LV_SYMBOL_IMAGE "  Slideshow (all photos)");
  lv_obj_set_style_text_font(sll, &lv_font_montserrat_20, 0);
  lv_obj_center(sll);

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

void buildRadio() { buildPlaceholder(&scrRadio, "RADIO"); }
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
  if (!lblSensors) return;                        // TEST 044 guard
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
  bool pm = false;
  if (s >= 0) {                                    // TEST 042: 12-hour
    int h24 = (int)(s/3600);
    pm = (h24 >= 12);
    hh = h24 % 12; if (hh == 0) hh = 12;
    mm = (s/60)%60;
  }
  int d[4];
  if (s >= 0) { d[0]=hh/10; d[1]=hh%10; d[2]=mm/10; d[3]=mm%10; }
  int x = x0;
  for (int i=0;i<4;i++){
    uint8_t segs = (s>=0) ? SEGMAP[d[i]] : 0x40;   // '-' if unset
    if (s>=0 && i==0 && d[0]==0) segs = 0x00;      // blank the leading zero
    drawSeg(x, y0, DG_W, DG_H, DG_T, segs, CLK_ON, CLK_OFF);
    x += DG_W + DG_GAP;
    if (i==1) x += COLON_W;
  }
}
void drawClockAmPm(){                              // TEST 042
  long s = nowSecOfDay();
  if (s < 0) return;
  int totalW = 4*DG_W + 3*DG_GAP + COLON_W;
  int x0 = (SCREEN_W - totalW)/2;
  int y0 = (SCREEN_H - DG_H)/2 + 10;
  gfx->setTextColor(CLK_ON); gfx->setTextSize(4);
  gfx->setCursor(x0 + totalW + 16, y0 + DG_H - 40);
  gfx->print(((s/3600) >= 12) ? "PM" : "AM");
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
    char v[16]; snprintf(v, sizeof(v), "%dC", (int)lroundf(gTemp));
    gfx->setTextColor(0x7BEF); gfx->setTextSize(8);   // ~0.75cm digits
    gfx->setCursor(SCREEN_W-250, 8); gfx->print(v);
  }
}
void drawClockScreen(){
  gfx->fillScreen(BLACK);
  drawClockDigits();
  drawColon(true);
  drawClockAmPm();
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
    // TEST 059: CR *or* LF ends a line. Only '\n' was accepted before,
    // and the PlatformIO monitor sends CR - which is why `ref` never ran.
    if (c=='\n' || c=='\r'){
      conBuf.trim();
      if (conBuf.length()==0){ /* bare CRLF, nothing typed */ }
      else if (conBuf.startsWith("time ")){
        int hh, mm;
        if (sscanf(conBuf.c_str()+5, "%d:%d", &hh, &mm)==2 && hh>=0 && hh<24 && mm>=0 && mm<60){
          baseSecOfDay = (long)hh*3600 + (long)mm*60;
          baseMillis = millis();
          timeSet = true;
          if (haveDS3231) { ds3231SetHM(hh, mm); Serial.printf("DS3231 + clock set to %02d:%02d\n", hh, mm); }
          else Serial.printf("clock set to %02d:%02d (no DS3231; will reset on power-off)\n", hh, mm);
          if (state == ST_SAVER) drawDiverClock(true);
        } else Serial.println("usage: time HH:MM");
      }
      else if (conBuf == "dump") uttDump();
      else if (conBuf.startsWith("th ")) {
        float v = atof(conBuf.c_str() + 3);
        if (v > 0.5f && v < 100.0f) {
          wakeThreshold = v;
          Serial.printf("threshold set to %.2f\n", wakeThreshold);
        } else Serial.println("usage: th 10.5");
      }
      else if (conBuf == "ref")      learnReference();
      else if (conBuf == "refclear") clearReferences();
      else if (conBuf == "dsp") {
        Serial.printf("dsp: %s  last word %d frames, %d/%d references (",
                      dspReady ? "ready" : "not ready", curFrames, refCount, REF_SLOTS);
        for (int i = 0; i < REF_SLOTS; i++) Serial.printf(" %d", refFrames[i]);
        for (int i = 0; i < REF_SLOTS; i++) Serial.printf(" %ums", (unsigned)refMs[i]);
        Serial.printf(" ), last dtw %.3f, threshold %.2f, wakes %u\n",
                      lastDist, wakeThreshold, (unsigned)wakeCount);
      }
      else if (conBuf == "mic") {
        Serial.printf("mic: rms %.4f  peak %.4f  floor %.4f  start %.4f  words %u\n",
                      micRms, micPeak, micFloor,
                      fmaxf(micFloor * UTT_START_MULT, UTT_ABS_FLOOR),
                      (unsigned)uttCount);
        // TEST 057: spread of the recent words - the number that decides
        // whether template matching is worth trying
        uint8_t n = (uttHistN < UTT_HISTORY) ? uttHistN : UTT_HISTORY;
        if (n >= 2) {
          uint32_t lo = 0xFFFFFFFF, hi = 0, sum = 0;
          for (uint8_t i = 0; i < n; i++) {
            uint32_t v = uttHist[i];
            if (v < lo) lo = v;
            if (v > hi) hi = v;
            sum += v;
          }
          Serial.printf("     last %u words: min %u  max %u  mean %u ms  spread %.2fx\n",
                        n, (unsigned)lo, (unsigned)hi, (unsigned)(sum / n),
                        lo ? (float)hi / (float)lo : 0.0f);
        }
      }
      else Serial.printf("unknown command '%s' - try: "
                         "time HH:MM, mic, dsp, ref, refclear, dump\n",
                         conBuf.c_str());
      conBuf = "";
    } else conBuf += c;
  }
}

// ============================================================
//  Screensaver enter/exit around LVGL
// ============================================================
void enterSaver() {
  state = ST_SAVER;
  lv_scr_load(scrBlank);      // hide LVGL widgets
  lv_refr_now(NULL);
  drawDiverClock(true);       // TEST 055: full paint on entry
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

  bool sd = false;
#if ENABLE_SD
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  sd = SD.begin(SD_CS, SPI);
  if (sd && showPhoto(WELCOME_FILE)) delay(WELCOME_MS);
  if (sd) scanPhotos();
  if (sd) { File df=SD.open(DIAL_FILE); haveDial = (bool)df; if(df) df.close(); }
  Serial.printf("dial: haveDial=%s\n", haveDial ? "yes" : "NO");
#else
  // TEST 053: SPI is never started. GPIO 11/12/13 stay free for the
  // microphone, and the SD card is never selected.
  Serial.println("SD: disabled - GPIO 11/12/13 belong to the I2S microphone");
  (void)sd;
#endif
  cacheDial();                       // TEST 046: decode once, reuse forever
  micBegin();                        // TEST 054: microphone

  dspInit();                         // TEST 058: MFCC tables + buffers

  // TEST 056: utterance buffer, 2 s at 16 kHz
  uttBuf = (int16_t *)ps_malloc((size_t)UTT_BUF_SAMPLES * sizeof(int16_t));
  preBuf = (int16_t *)ps_malloc((size_t)PRE_ROLL_SAMPLES * sizeof(int16_t));
  Serial.printf("utt buffer: %s (%u bytes)  pre-roll: %s (%u bytes)\n",
                uttBuf ? "ok" : "FAILED",
                (unsigned)((size_t)UTT_BUF_SAMPLES * sizeof(int16_t)),
                preBuf ? "ok" : "FAILED",
                (unsigned)((size_t)PRE_ROLL_SAMPLES * sizeof(int16_t)));

  // TEST 055: off-screen compose buffer for a flicker-free screensaver
  clockBuf = (uint16_t *)ps_malloc((size_t)CLK_REG_W * CLK_REG_H * sizeof(uint16_t));
  Serial.printf("clock buffer: %s (%d bytes)\n",
                clockBuf ? "ok" : "FAILED - falling back to full repaint",
                (int)((size_t)CLK_REG_W * CLK_REG_H * sizeof(uint16_t)));

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

  // TEST 061: the microphone runs in EVERY state. It used to live inside
  // the ST_UI branch, so it stopped the moment the screensaver came on.
  if (micRead()) uttProcess();

  if (state == ST_UI) {
    lv_timer_handler();

    if (millis() - lastSensorMs > 1000) {
      lastSensorMs = millis();
      if (haveBH1750) luxOK = bh1750Read(gLux);
      if (haveBME280) tempOK = bme280Read(gTemp, gHum, gPress);
      computeTargetFromLux();
      updateSensorLabel();
      // TEST 054: one line a second, so a silent mic is obvious
      if (micReady)
        Serial.printf("mic: rms %.4f  peak %.4f  reads %u\n",
                      micRms, micPeak, (unsigned)micReads);
    }

    // hide the wake banner after a moment
    if (wakeShownMs && (millis() - wakeShownMs) > WAKE_SHOW_MS) {
      wakeShownMs = 0;
      if (wakeBanner) lv_obj_add_flag(wakeBanner, LV_OBJ_FLAG_HIDDEN);
    }

    if (micBar) {
      int pct = (int)(micRms * 400.0f);        // x4 so speech is visible
      if (pct > 100) pct = 100;
      lv_bar_set_value(micBar, pct, LV_ANIM_OFF);
      if (micLbl)
        lv_label_set_text_fmt(micLbl, "mic %d%%", pct);
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
      slideOn = false;                     // TEST 041: a touch stops the show
      state = ST_UI;
      gfx->fillScreen(BLACK);              // wipe the photo
      lv_scr_load(scrSettings);            // back to Settings
      lv_refr_now(NULL);                   // force a complete repaint now
      delay(200);                          // debounce the release
    } else if (slideOn && millis() >= slideNextMs) {
      // TEST 041: advance the slideshow. showPhoto() is size-guarded and
      // frees its buffer every time, so a long run cannot exhaust memory.
      if (slideIdx >= photoCount) slideIdx = 0;
      if (!showPhoto(photoList[slideIdx])) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(0x7BEF); gfx->setTextSize(2);
        gfx->setCursor(200, 230); gfx->print("skipped: ");
        gfx->print(photoList[slideIdx]);
      }
      slideIdx++;
      slideNextMs = millis() + SLIDE_MS;
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
 *                        TEST  061   (end of file)
 * ============================================================
 *  Panel - Stage 3 - rebuild of the lost 041-044 on top of 040
 *
 *   041  memory-safe photos (600 KB guard, PSRAM then heap,
 *        always freed) + Files-tab slideshow, 2 s per photo
 *   042  +/- time setter replacing the broken rollers,
 *        12-hour AM/PM on the settings, LVGL and 7-segment faces
 *   043  clock hands dimmed to about 70 percent
 *   044  null guards on updateSensorLabel, updateClockFace and
 *        updateAnalog, all display pointers initialised to NULL
 *
 *  platformio.ini v4 - LV_CONF_SKIP, no lv_conf.h needed.
 *  NOT hardware-tested. Watch the serial monitor on first boot.
 *  Next: bed box TEST 0014 on UART, then end-to-end motor test.
 * ============================================================ */
