/* ============================================================
 *                        TEST 055
 * ============================================================
 *
 *  BG BED - AUDIO NODE INTEGRATION SELF TEST
 *
 *  Board : ESP32-S3-WROOM-1 N16R8
 *
 *  Existing / already used from TEST014:
 *    W5500 Ethernet
 *      SCK  = GPIO12
 *      MOSI = GPIO11
 *      MISO = GPIO10
 *      CS   = GPIO9
 *      INT  = GPIO8
 *      RST  = GPIO7
 *
 *    PCM5102A I2S
 *      BCLK = GPIO14
 *      LRC  = GPIO2
 *      DOUT = GPIO13
 *
 *  Added in TEST016:
 *    RS485 auto-direction module
 *      ESP32 TX GPIO17 -> module RXD
 *      ESP32 RX GPIO18 <- module TXD
 *      GND common
 *
 *    7 RGBW NeoPixels
 *      DATA = GPIO21
 *      exactly 7 pixels
 *      NEO_GRBW + NEO_KHZ800
 *
 *    microSD module (3.3V)
 *      CLK  = GPIO35
 *      MISO = GPIO36
 *      MOSI = GPIO37
 *      CS   = GPIO39
 *      VCC  = 3.3V
 *      GND  = GND
 *
 *  Retained ambient bench hardware (user confirmed 11 September 2026):
 *    GPIO15 -> DATA IN left, 12 GRBW pixels; GPIO16 -> DATA IN right, 12.
 *    GPIO21 diagnostic strip stays separate. Starts OFF, cap 64/255.
 *    Panels use A5 0x24 requests / 0x25 ACKs in existing granted slots.
 *    Web tablet control: /lights. Music level observes real PCM after gain.
 *
 *  TEST055: verify 2048-byte blocks over a 1 Mbps link, retry damaged blocks,
 *  and measure transfer time before full CRC verification and playback.
 *  TEST054: ACK and verify each 512-byte block, retry damaged blocks, then
 *  compare end-to-end CRC before enabling the DAC.
 *  TEST053: receive entire WAV PCM in PSRAM, compare end-to-end CRC, then
 *  enable the DAC. 't' plays a direct 440 Hz DAC tone without the Listener.
 *  TEST053 WIRED PLAYBACK DIAGNOSTICS:
 *  PCM5102A three-wire PLL needs 64 BCK per frame at 16 kHz.  Convert
 *  Listener's 16-bit stereo PCM to left-aligned 32-bit I2S slots.
 *  TEST051 DREAM DAC CLOCK:
 *  TEST050 SERIAL RECOVERY:
 *    Echoes the application console on native USB CDC and UART0 (CP2102).
 *    Accepts commands through either connector. No credentials are logged.
 *    Prints an unmistakable serial heartbeat every 1 second.
 *    The heartbeat runs regardless of Ethernet, SD, RS485 or Audio state.
 *    This isolates terminal/USB from the rest of the AudioNode.
 *
 *  PURPOSE:
 *    One firmware to prove that Ethernet + internal network,
 *    SD card, RS485 receive/transmit path, 7 RGBW pixels and
 *    PCM5102A audio can coexist on the AudioNode.
 *
 *  LED diagnostics:
 *    Fixed LED positions from DATA IN: heartbeat, Ethernet, IP, SD, RX, TX, audio.
 *    Heartbeat 2s on / 2s off; RX/TX held 1.5s; RX red CRC error held 5s.
 *    Brightness 30/255; no scanning or full-chain flashes.
 *
 *  RS485:
 *    listens for the same 14-byte A5 frame used by the BG Bed bus:
 *      0  0xA5
 *      1  SRC
 *      2  DST
 *      3  TYPE
 *      4  SEQ low
 *      5  SEQ high
 *      6..12 payload/time fields
 *      13 XOR CRC of bytes 0..12
 *
 *    Two RGBW mood/music outputs on user-verified free GPIO15/16.
 *    Existing radio/catalog/partition correction retained; SD stays disabled.
 *    Shared internet search, 64 persistent presets, remembered station/volume.
 *    TEST043 adds token-protected local Listener WAV playback on AudioNode.
 *    The latest completed recording is streamed over the local network;
 *    radio state remains separate and radio commands can preempt the dream.
 *    SD bypass from working TEST032 is retained. NOT YET HARDWARE PROVEN.
 *    SD is intentionally disabled for this internet-radio diagnostic.
 *    This test returns from the TEST030 slideshow experiment to the main
 *    AudioNode integration line, using TEST016 transport/audio as its base.
 *    It reacts to a small manual Shemi-panel test:
 *      TYPE 0x20 = audio command to AudioNode node 3
 *      TYPE 0x21 = matching acknowledgement
 *      payload[0] command: 1 ensure PLAY / update volume, 2 STOP, 4 STATUS
 *      payload[1] station: 0..7, see include/radio_stations.h
 *      payload[2] target volume: 0..21
 *      payload[3] fade time in 100 ms units
 *    AudioNode owns the non-blocking fade from silence.
 *
 *  Serial commands:
 *    i = print complete diagnostics
 *    x = transmit one harmless diagnostic RS485 frame
 *    p = play test internet radio stream
 *    s = stop audio
 *
 *  Web:
 *    open http://<AudioNode-IP>/
 *    shows Ethernet / SD / RS485 / LEDs / Audio diagnostics.
 *
 *  NOT YET TESTED ON HARDWARE.
 * ============================================================ */

#define TEST_NUMBER 55

#define USE_WIFI 0

#include <Arduino.h>
#include <atomic>
#include <ctype.h>
#include <SPI.h>
#include <SD.h>
#include <ETH.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include "Audio.h"
#include "wired_dream.h"
#include "ambient_lighting.h"

// The boot ROM is visible on UART0 even when Arduino's Serial is native USB.
// A COM port that shows only ROM output cannot operate the old Serial command
// parser. Keep both interfaces alive so the existing monitor connector works.
class DualConsole : public Print {
 public:
  void begin(uint32_t baud) {
    Serial.begin(baud);
    Serial0.begin(baud);
  }
  size_t write(uint8_t byte) override {
    const size_t usb = Serial.write(byte);
    const size_t uart = Serial0.write(byte);
    return usb > uart ? usb : uart;
  }
  size_t write(const uint8_t *data, size_t length) override {
    const size_t usb = Serial.write(data, length);
    const size_t uart = Serial0.write(data, length);
    return usb > uart ? usb : uart;
  }
  int available() { return Serial.available() + Serial0.available(); }
  int read() {
    if (Serial.available()) return Serial.read();
    return Serial0.available() ? Serial0.read() : -1;
  }
  void flush() {
    Serial.flush();
    Serial0.flush();
  }
};
static DualConsole console;

void ambientCapturePCM(const int32_t* samples, int16_t words) {
  ambient::capture(samples, words);
}

/* ============================================================
 * PIN MAP
 * ============================================================ */

// W5500 - preserve TEST014 wiring
#define PIN_ETH_SCK   12
#define PIN_ETH_MOSI  11
#define PIN_ETH_MISO  10
#define PIN_ETH_CS     9
#define PIN_ETH_INT    8
#define PIN_ETH_RST    7

// PCM5102A - preserve TEST014 wiring
#define PIN_I2S_BCLK  14
#define PIN_I2S_LRC    2
#define PIN_I2S_DOUT  13
// The Audio library owns I2S0 for radio. Its setPinout() cannot be called a
// second time with this library version (ESP_ERR_INVALID_STATE). Release the
// radio object before Dream playback, and construct a new one afterward.

// RS485 auto-direction module
#define PIN_RS485_TX  17
#define PIN_RS485_RX  18

// exactly 7 RGBW NeoPixels
#define PIN_PIXELS    21
#define PIXEL_COUNT    7

// microSD - separate SPI bus
#define PIN_SD_SCK    35
#define PIN_SD_MISO   36
#define PIN_SD_MOSI   37
#define PIN_SD_CS     39

/* ============================================================
 * SETTINGS
 * ============================================================ */

#define ETH_PHY_ADDRESS   1
#define ETH_SPI_MHZ       12
#define RS485_BAUD        115200
#define STATUS_EVERY_MS   10000
#define LED_STEP_MS       110
#define LED_EVENT_MS      220

#define VOLUME_START      10
#define VOLUME_MAX        21

#define NODE_SHEMI         1
#define NODE_IRA           2
#define NODE_AUDIO         3
#define TYPE_AUDIO_CMD  0x20
#define TYPE_AUDIO_ACK  0x21
#define AUDIO_PLAY         1
#define AUDIO_STOP         2
#define AUDIO_STATUS       4
#define AUDIO_DREAM_PLAY   8
#define AUDIO_DREAM_STOP   9
#define AUDIO_DREAM_STATUS 10
#define AUDIO_DREAM_SELECTED 11
#define AUDIO_DREAM_PAUSE 12
#define AUDIO_DREAM_RESUME 13
#define AUDIO_DREAM_SEEK 14
#define AUDIO_DREAM_VOLUME 15
#define AUDIO_DREAM_SEEK_TO 16
#include "radio_stations.h"

static const char *TEST_STREAM =
  "https://ice5.somafm.com/groovesalad-128-mp3";

/* ============================================================
 * OBJECTS
 * ============================================================ */

static Audio *audio = nullptr;
WebServer server(80);

// W5500 uses global SPI.
// SD gets the second hardware SPI controller.
SPIClass sdSPI(HSPI);

Adafruit_NeoPixel pixels(
  PIXEL_COUNT,
  PIN_PIXELS,
  NEO_GRBW + NEO_KHZ800
);

/* ============================================================
 * STATE
 * ============================================================ */

static std::atomic<bool> g_ethLink{false};
static std::atomic<bool> g_haveIP{false};
static bool g_sdOK = false;
static bool g_sdWriteOK = false;
static bool g_audioConfigured = false;
static bool g_webStarted = false;

static uint64_t g_sdSizeMB = 0;

static uint32_t g_rs485Bytes = 0;
static uint32_t g_rs485A5Good = 0;
static uint32_t g_rs485A5Bad = 0;
static uint32_t g_rs485TxFrames = 0;
static uint32_t g_rs485OtherBytes = 0;

static uint8_t g_lastSrc = 0;
static uint8_t g_lastDst = 0;
static uint8_t g_lastType = 0;
static uint16_t g_lastSeq = 0;

static unsigned long g_bootMs = 0;
static unsigned long g_lastStatus = 0;
static unsigned long g_lastSerialHeartbeat = 0;

static bool g_fadeActive = false;
static bool g_pendingPlay = false;
static bool g_waitForAudio = false;
static bool g_playFailed = false;
enum class PlaybackSource : uint8_t { None, Radio, Dream };
static PlaybackSource g_playbackSource = PlaybackSource::None;
static uint8_t g_station = 0; // 255 = bounded custom URL, never an array index
static char g_customName[49] = {}, g_customURL[321] = {}, g_customUuid[37] = {};
static void catalogAudioMutation();
static const char *currentRadioName() {
  return g_station < RADIO_STATION_COUNT ? RADIO_STATIONS[g_station].name : g_customName;
}
static const char *currentRadioURL() {
  return g_station < RADIO_STATION_COUNT ? RADIO_STATIONS[g_station].url : g_customURL;
}
static uint32_t g_connectStarted = 0;
static uint8_t g_fadeTarget = 11;
static uint8_t g_radioOwner = 0, g_radioRevision = 0;
static uint16_t g_fadeDurationMs = 1500;
static unsigned long g_fadeStartMs = 0;
static int g_lastAppliedVolume = -1;
static String g_dreamSerialLine;
static uint8_t g_dreamRank = 255, g_dreamVolume = 11;
static uint16_t g_dreamKey = 0;
static uint32_t g_dreamOffset = 0, g_dreamTotal = 0;
static bool g_dreamPaused = false;
// Acknowledge the RS485 request before the potentially slow audio/I2S switch.
static bool g_dreamStartPending = false, g_dreamStartFailed = false;
static bool g_dreamTransferLogged = false;
static bool g_dreamPendingSelected = false;
static uint8_t g_dreamPendingRank = 255;
static uint16_t g_dreamPendingKey = 0;

enum LedEvent {
  LED_EVENT_NONE,
  LED_EVENT_RX,
  LED_EVENT_TX,
  LED_EVENT_BADCRC
};

static unsigned long g_lastRxMs = 0, g_lastTxMs = 0, g_lastCrcMs = 0;
static bool g_seenRx = false, g_seenTx = false, g_seenCrc = false;
static uint8_t playbackState();
static uint8_t dreamPlaybackState();
static unsigned long g_lastLedStep = 0;

/* ============================================================
 * LOG
 * ============================================================ */

static void say(const String &s)
{
  console.println(s);
}

static void sayf(const char *fmt, ...)
{
  char b[220];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof(b), fmt, ap);
  va_end(ap);
  say(String(b));
}

/* ============================================================
 * PIXELS
 * ============================================================ */

static void pixelsOff()
{
  pixels.clear();
  pixels.show();
}

static void pixelsAll(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0)
{
  for (int i = 0; i < PIXEL_COUNT; ++i) {
    pixels.setPixelColor(i, pixels.Color(r, g, b, w));
  }
  pixels.show();
}

static void startupPixelTest() { pixelsOff(); }

static void triggerLedEvent(LedEvent ev) {
  if (ev == LED_EVENT_RX) { g_lastRxMs = millis(); g_seenRx = true; }
  if (ev == LED_EVENT_TX) { g_lastTxMs = millis(); g_seenTx = true; }
  if (ev == LED_EVENT_BADCRC) { g_lastCrcMs = millis(); g_seenCrc = true; }
}

static bool coreHealthy()
{
  // RS485 is passive in this test; no received packet is not a fault.
  return g_ethLink && g_haveIP && g_audioConfigured; // SD intentionally disabled
}

static void updatePixels() {
  const unsigned long now = millis();
  if (now - g_lastLedStep < 100) return;
  g_lastLedStep = now;
  pixels.clear();
  const uint32_t green = pixels.Color(0, 80, 0, 0);
  const uint32_t red = pixels.Color(80, 0, 0, 0);
  const uint32_t amber = pixels.Color(80, 45, 0, 0);
  const uint32_t blue = pixels.Color(0, 0, 80, 0);
  pixels.setPixelColor(0, (now / 2000UL) % 2 == 0 ? green : 0);
  pixels.setPixelColor(1, g_ethLink ? green : red);
  pixels.setPixelColor(2, g_haveIP ? green : amber);
  // LED 4 intentionally off: SD is disabled.
  if (g_seenCrc && now - g_lastCrcMs < 5000UL) pixels.setPixelColor(4, red);
  else if (g_seenRx && now - g_lastRxMs < 1500UL) pixels.setPixelColor(4, blue);
  if (g_seenTx && now - g_lastTxMs < 1500UL) pixels.setPixelColor(5, amber);
  const uint8_t state = g_playbackSource == PlaybackSource::Dream ? dreamPlaybackState() : playbackState();
  pixels.setPixelColor(6, state == 3 ? red : state == 2 ? green : state == 1 ? amber : 0);
  pixels.show();
}

/* ============================================================
 * SD CARD
 * ============================================================ */

static bool testSDWrite()
{
  const char *path = "/bgbed_test015.txt";

  File f = SD.open(path, FILE_APPEND);
  if (!f) {
    say(F("sd ......... WRITE OPEN FAILED"));
    return false;
  }

  f.printf("BG BED AudioNode TEST055 boot=%lu ms\r\n", millis());
  f.close();

  File r = SD.open(path, FILE_READ);
  if (!r) {
    say(F("sd ......... READBACK OPEN FAILED"));
    return false;
  }

  size_t sz = r.size();
  r.close();

  sayf("sd ......... write/read test OK, file size %u bytes", (unsigned)sz);
  return true;
}

static void startSD()
{
  sayf("sd pins .... SCK %d MISO %d MOSI %d CS %d",
       PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

  sdSPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

  if (!SD.begin(PIN_SD_CS, sdSPI, 10000000)) {
    g_sdOK = false;
    g_sdWriteOK = false;
    say(F("sd ......... MOUNT FAILED"));
    return;
  }

  uint8_t type = SD.cardType();
  if (type == CARD_NONE) {
    g_sdOK = false;
    g_sdWriteOK = false;
    say(F("sd ......... NO CARD"));
    return;
  }

  g_sdOK = true;
  g_sdSizeMB = SD.cardSize() / (1024ULL * 1024ULL);

  const char *tn =
    type == CARD_MMC  ? "MMC" :
    type == CARD_SD   ? "SDSC" :
    type == CARD_SDHC ? "SDHC" : "UNKNOWN";

  sayf("sd ......... OK type=%s size=%llu MB",
       tn, (unsigned long long)g_sdSizeMB);

  g_sdWriteOK = testSDWrite();
}

/* ============================================================
 * RS485
 * ============================================================ */

static uint8_t xorCRC(const uint8_t *p, size_t n)
{
  uint8_t c = 0;
  while (n--) c ^= *p++;
  return c;
}

static uint8_t a5buf[14];
static uint8_t a5pos = 0;
static bool inA5 = false;
static uint32_t a5lastByteMs = 0;

static void sendAudioAck(uint8_t dst, uint16_t seq, uint8_t command,
                         uint8_t result, uint8_t state, uint8_t volume)
{
  uint8_t f[14] = {0};
  f[0] = 0xA5;
  f[1] = NODE_AUDIO;
  f[2] = dst;
  f[3] = TYPE_AUDIO_ACK;
  f[4] = (uint8_t)(seq & 0xFF);
  f[5] = (uint8_t)(seq >> 8);
  f[6] = command;
  f[7] = result;
  f[8] = state;
  f[9] = volume;
  if (command >= AUDIO_DREAM_PLAY && command <= AUDIO_DREAM_SEEK_TO) {
    const uint32_t position = g_dreamPaused ? g_dreamOffset :
        wiredDream::startByte + wiredDream::readAt.load();
    const uint16_t elapsed = uint16_t(position / 64000U > 4095U ? 4095U : position / 64000U);
    const uint32_t totalBytes = g_dreamTotal ? g_dreamTotal : wiredDream::totalBytes;
    const uint16_t duration = uint16_t(totalBytes / 64000U > 4095U ? 4095U : totalBytes / 64000U);
    f[10] = uint8_t(elapsed);
    f[11] = uint8_t((elapsed >> 8) | ((duration & 15U) << 4));
    f[12] = uint8_t(duration >> 4);
  } else {
    f[10] = g_station; f[11] = g_radioOwner; f[12] = g_radioRevision;
  }
  f[13] = xorCRC(f, 13);
  Serial2.write(f, sizeof(f));
  Serial2.flush();
  g_rs485TxFrames++;
  triggerLedEvent(LED_EVENT_TX);
  if (g_playbackSource != PlaybackSource::Dream ||
      (command != AUDIO_STATUS && command != AUDIO_DREAM_STATUS))
    sayf("audio cmd .. ACK seq=%u cmd=%u result=%u state=%u volume=%u",
         seq, command, result, state, volume);
}

// Ambient uses the existing Shemi-granted bus slots, independent of radio.
static void processAmbientCommand() {
  const uint8_t src = a5buf[1], cmd = a5buf[6], value = a5buf[7];
  if ((src != NODE_SHEMI && src != NODE_IRA) || a5buf[2] != NODE_AUDIO) return;
  const uint16_t seq = (uint16_t)a5buf[4] | ((uint16_t)a5buf[5] << 8);
  struct Last { bool valid; uint16_t seq; uint8_t cmd, value, result; uint32_t at; };
  static Last last[2] = {};
  Last& prev = last[src - 1];
  bool reserved = false;
  for (unsigned i = 8; i <= 12; ++i) if (a5buf[i]) reserved = true;
  uint8_t result;
  // Bound duplicate window so a rebooted panel can reuse its initial sequence.
  if (prev.valid && prev.seq == seq && prev.cmd == cmd && prev.value == value &&
      millis() - prev.at < 5000 && !reserved) result = prev.result;
  else {
    result = reserved ? 2 : ambient::apply(cmd, value);
    prev = {true, seq, cmd, value, result, millis()};
  }
  uint8_t f[14] = {0xA5, NODE_AUDIO, src, 0x25, (uint8_t)seq, (uint8_t)(seq >> 8),
    cmd, result, ambient::flags(), ambient::percent(), ambient::color, ambient::sides, ambient::revision, 0};
  f[13] = xorCRC(f, 13);
  Serial2.write(f, sizeof(f)); Serial2.flush();
  ++g_rs485TxFrames; triggerLedEvent(LED_EVENT_TX);
}

static uint8_t playbackState() {
  if (g_playFailed) return 3;
  if (g_pendingPlay || g_waitForAudio) return 1;
  return audio && audio->isRunning() ? 2 : 0;
}

static uint8_t radioPlaybackState() {
  return g_playbackSource == PlaybackSource::Radio ? playbackState() : 0;
}

static uint8_t dreamPlaybackState() {
  if (g_dreamStartPending) return 1; // accepted, about to start
  if (g_dreamStartFailed) return 3;
  if (g_playbackSource != PlaybackSource::Dream) return 0;
  return g_playFailed ? 3 : g_dreamPaused ? 4 : wiredDream::status.load();
}

static void queueDreamStart(uint8_t rank = 255, uint16_t key = 0,
                            bool selected = false) {
  g_dreamPendingRank = rank;
  g_dreamPendingKey = key;
  g_dreamPendingSelected = selected;
  g_dreamStartFailed = false;
  g_dreamStartPending = true;
}

static void startAudio();
static bool stopDreamPlayback();
static bool startDreamPlayback(uint8_t rank = 0, uint16_t key = 0, bool selected = false) {
  g_dreamTransferLogged = false;
  if (g_playbackSource == PlaybackSource::Dream && !stopDreamPlayback()) return false;
  g_playbackSource = PlaybackSource::None;
  if (audio) {
    audio->stopSong();
    delete audio; // releases the library's I2S0 channel and DAC pins
    audio = nullptr;
  }
  say(F("dream ...... radio I2S released; wired player owns DAC"));
  g_pendingPlay = g_waitForAudio = g_fadeActive = false;
  g_lastAppliedVolume = 0;
  g_dreamRank = selected ? rank : 255;
  g_dreamKey = selected ? key : 0;
  g_dreamOffset = g_dreamTotal = 0;
  g_dreamPaused = false;
  if (!wiredDream::start(g_dreamRank, g_dreamKey, g_dreamVolume)) {
    startAudio();
    g_playFailed = true;
    say(F("dream ...... wired I2S/UART link could not start"));
    return false;
  }
  g_playbackSource = PlaybackSource::Dream;
  g_playFailed = false;
  sayf("dream ...... requesting %s recording over dedicated cable",
       selected ? "selected" : "latest");
  return true;
}

static void startRadioFade(uint8_t target, uint16_t fadeMs, uint8_t station = 0)
{
  if (g_playbackSource == PlaybackSource::Dream && !stopDreamPlayback()) return;
  if (!audio) startAudio();
  if (!audio) { g_playFailed = true; return; }
  if (station >= RADIO_STATION_COUNT && (station != 255 || !g_customURL[0])) return;
  if (station < RADIO_STATION_COUNT) g_customUuid[0] = 0;
  if (target > VOLUME_MAX) target = VOLUME_MAX;
  if (fadeMs < 100) fadeMs = 100;
  // A volume adjustment on the current station must not reconnect the stream.
  if (g_playbackSource == PlaybackSource::Radio && station == g_station && !g_playFailed &&
      (g_pendingPlay || g_waitForAudio || audio->isRunning())) {
    g_fadeTarget = target;
    if (!g_waitForAudio && !g_pendingPlay) {
      g_fadeActive = false;
      audio->setVolume(target);
      g_lastAppliedVolume = target;
    }
    return;
  }
  g_playbackSource = PlaybackSource::None;
  audio->stopSong();
  audio->setVolume(0);
  g_lastAppliedVolume = 0;
  g_station = station;
  g_playbackSource = PlaybackSource::Radio;
  g_fadeTarget = target;
  g_fadeDurationMs = fadeMs;
  g_fadeActive = false;
  g_pendingPlay = true;
  g_waitForAudio = false;
  g_playFailed = false;
}

static void startCustomRadio(const char *name, const char *url, const char *uuid,
                             uint8_t target, uint16_t fadeMs) {
  const bool same = !strcmp(currentRadioURL(), url);
  // A different custom URL must reconnect; same URL volume changes keep decoding.
  if (!same) g_station = 254;
  strlcpy(g_customName, name, sizeof(g_customName));
  strlcpy(g_customURL, url, sizeof(g_customURL));
  strlcpy(g_customUuid, uuid, sizeof(g_customUuid));
  if (same && g_station < RADIO_STATION_COUNT) {
    startRadioFade(target, fadeMs, g_station);
  } else startRadioFade(target, fadeMs, 255);
}

static void stopRadioCommand()
{
  g_playbackSource = PlaybackSource::None;
  g_pendingPlay = g_waitForAudio = g_fadeActive = false;
  g_playFailed = false;
  if (audio) { audio->stopSong(); audio->setVolume(0); }
  g_lastAppliedVolume = 0;
  say(F("audio cmd .. STOP"));
}

static bool stopDreamPlayback() {
  if (g_playbackSource != PlaybackSource::Dream) return true;
  if (!wiredDream::stop()) {
    g_playFailed = true;
    say(F("dream ...... playback task did not stop; I2S kept assigned to Dream"));
    return false;
  }
  startAudio(); // fresh Audio object, first and only setPinout call
  if (!audio) {
    g_playFailed = true;
    say(F("dream ...... radio I2S could not be restored"));
    return false;
  }
  g_playbackSource = PlaybackSource::None;
  g_dreamPaused = false;
  g_dreamOffset = g_dreamTotal = 0;
  g_pendingPlay = g_waitForAudio = g_fadeActive = false;
  g_playFailed = false;
  if (audio) audio->setVolume(0);
  g_lastAppliedVolume = 0;
  say(F("dream ...... STOP wired playback"));
  return true;
}

static bool controlDreamPlayback(uint8_t command, uint8_t value) {
  if (command == AUDIO_DREAM_VOLUME) {
    if (value > VOLUME_MAX) return false;
    g_dreamVolume = value;
    wiredDream::volume = value;
    return true;
  }
  if (g_playbackSource != PlaybackSource::Dream || g_playFailed) return false;
  if (command == AUDIO_DREAM_PAUSE) {
    if (g_dreamPaused) return true;
    if (wiredDream::status.load() != 2) return false;
    g_dreamTotal = wiredDream::totalBytes;
    g_dreamOffset = (wiredDream::startByte + wiredDream::readAt.load()) & ~3U;
    if (g_dreamTotal >= 4 && g_dreamOffset >= g_dreamTotal)
      g_dreamOffset = (g_dreamTotal - 4) & ~3U;
    if (!wiredDream::stop()) { g_playFailed = true; return false; }
    g_dreamPaused = true;
    return true;
  }
  if (command == AUDIO_DREAM_RESUME) {
    if (!g_dreamPaused || !g_dreamTotal) return false;
    if (!wiredDream::start(g_dreamRank, g_dreamKey, g_dreamVolume, g_dreamOffset)) {
      g_playFailed = true; return false;
    }
    g_dreamPaused = false;
    return true;
  }
  if (command == AUDIO_DREAM_SEEK || command == AUDIO_DREAM_SEEK_TO) {
    if ((command == AUDIO_DREAM_SEEK && value > 1) ||
        (command == AUDIO_DREAM_SEEK_TO && value > 100)) return false;
    const bool paused = g_dreamPaused;
    if (!paused && wiredDream::status.load() != 2) return false;
    const uint32_t total = paused ? g_dreamTotal : wiredDream::totalBytes;
    if (total < 4) return false;
    const uint32_t now = paused ? g_dreamOffset :
        wiredDream::startByte + wiredDream::readAt.load();
    int64_t target = command == AUDIO_DREAM_SEEK_TO ?
        (int64_t(total) * value / 100) :
        int64_t(now) + (value ? 640000LL : -640000LL);
    if (target < 0) target = 0;
    if (target >= total) target = total - 4;
    g_dreamOffset = uint32_t(target) & ~3U;
    g_dreamTotal = total;
    if (paused) return true;
    if (!wiredDream::start(g_dreamRank, g_dreamKey, g_dreamVolume, g_dreamOffset)) {
      g_playFailed = true; return false;
    }
    return true;
  }
  return false;
}

static void processAudioCommand()
{
  if ((a5buf[1] != NODE_SHEMI && a5buf[1] != NODE_IRA) || a5buf[2] != NODE_AUDIO) return;
  const uint16_t seq = (uint16_t)a5buf[4] | ((uint16_t)a5buf[5] << 8);
  const uint8_t command = a5buf[6], station = a5buf[7], target = a5buf[8];
  const uint16_t fadeMs = (uint16_t)a5buf[9] * 100U;
  uint8_t result = 0;
  if (command == AUDIO_PLAY || command == 7) {
    if (!g_haveIP) result = 1;
    else if (station >= RADIO_STATION_COUNT || (command == AUDIO_PLAY && target > VOLUME_MAX)) result = 2;
    else startRadioFade(command == 7 ? g_fadeTarget : target, fadeMs, station);
  } else if (command == AUDIO_STOP) {
    if (g_playbackSource == PlaybackSource::Radio) stopRadioCommand();
  }
  else if (command == 3) {
    if (target > VOLUME_MAX) result = 2;
    else if (radioPlaybackState() == 1 || radioPlaybackState() == 2)
      startRadioFade(target, fadeMs, g_station);
    else g_fadeTarget = target;
  } else if (command == 5 || command == 6) {
    if (command == 6 && (radioPlaybackState() == 1 || radioPlaybackState() == 2)) stopRadioCommand();
    else if (!g_haveIP) result = 1;
    else startRadioFade(g_fadeTarget, fadeMs, g_station);
  } else if (command == AUDIO_DREAM_PLAY) {
    queueDreamStart();
  } else if (command == AUDIO_DREAM_SELECTED) {
    const uint16_t key = uint16_t(a5buf[8]) | (uint16_t(a5buf[9]) << 8);
    if (station >= 64) result = 2;
    else queueDreamStart(station, key, true);
  } else if (command == AUDIO_DREAM_STOP) {
    g_dreamStartPending = g_dreamStartFailed = false;
    if (!stopDreamPlayback()) result = 1;
  } else if ((command >= AUDIO_DREAM_PAUSE && command <= AUDIO_DREAM_VOLUME) ||
             command == AUDIO_DREAM_SEEK_TO) {
    if (!controlDreamPlayback(command, station)) result = 1;
  }
  else if (command != AUDIO_STATUS && command != AUDIO_DREAM_STATUS) result = 2;
  if (result == 0 && command != AUDIO_STATUS &&
      command != AUDIO_DREAM_PLAY && command != AUDIO_DREAM_STOP && command != AUDIO_DREAM_STATUS &&
      command != AUDIO_DREAM_SELECTED &&
      (command < AUDIO_DREAM_PAUSE || command > AUDIO_DREAM_SEEK_TO)) {
    g_radioOwner = a5buf[1]; ++g_radioRevision;
    catalogAudioMutation();
  }
  // Dream commands expose only Dream state; legacy radio status remains stopped
  // while a Dream WAV is playing. Running is reported only after isRunning().
  const bool dreamCommand = command >= AUDIO_DREAM_PLAY && command <= AUDIO_DREAM_SEEK_TO;
  sendAudioAck(a5buf[1], seq, command, result,
               dreamCommand ? dreamPlaybackState() : radioPlaybackState(),
               dreamCommand ? g_dreamVolume : g_fadeTarget);
}

static void serviceRadioConnection()
{
  if (g_playbackSource != PlaybackSource::Radio) return;
  if (g_pendingPlay) {
    g_pendingPlay = false;
    g_connectStarted = millis();
    g_waitForAudio = true;
    sayf("radio ...... CONNECT station=%u %s", g_station, currentRadioName());
    if (!audio->connecttohost(currentRadioURL())) {
      g_waitForAudio = false;
      g_playFailed = true;
      say(F("radio ...... CONNECTION FAILED"));
    }
  }
  if (g_waitForAudio) {
    if (audio->isRunning()) {
      g_waitForAudio = false;
      g_fadeStartMs = millis();
      g_fadeActive = true;
      sayf("radio ...... RUNNING station=%u target=%u", g_station, g_fadeTarget);
    } else if (millis() - g_connectStarted > 20000UL) {
      stopRadioCommand();
      g_playbackSource = PlaybackSource::Radio;
      g_playFailed = true;
      say(F("radio ...... START TIMEOUT"));
    }
  }
}

#include "radio_catalog.h"

static void processA5Frame()
{
  uint8_t wanted = xorCRC(a5buf, 13);
  uint8_t got = a5buf[13];

  if (wanted != got) {
    g_rs485A5Bad++;
    sayf("rs485 ...... BAD CRC want=%02X got=%02X", wanted, got);
    triggerLedEvent(LED_EVENT_BADCRC);
    return;
  }

  g_rs485A5Good++;

  g_lastSrc = a5buf[1];
  g_lastDst = a5buf[2];
  g_lastType = a5buf[3];
  g_lastSeq = (uint16_t)a5buf[4] | ((uint16_t)a5buf[5] << 8);

  if (g_playbackSource != PlaybackSource::Dream &&
      (g_lastType < 0x30 || g_lastType > 0x35)) sayf("rs485 ...... A5 OK src=%u dst=%u type=0x%02X seq=%u total=%u",
       g_lastSrc, g_lastDst, g_lastType, g_lastSeq,
       (unsigned)g_rs485A5Good);

  triggerLedEvent(LED_EVENT_RX);

  if (g_lastType == 0x24) processAmbientCommand();
  else if (g_lastType == TYPE_AUDIO_CMD) processAudioCommand();
  else catalogHandleFrame(a5buf);
}

static void pollRS485()
{
  // Discard a truncated frame before the next coordinated bus slot.
  if (inA5 && millis() - a5lastByteMs > 50) { inA5 = false; a5pos = 0; }
  while (Serial2.available()) {
    uint8_t b = (uint8_t)Serial2.read();
    a5lastByteMs = millis();
    g_rs485Bytes++;

    if (!inA5) {
      if (b == 0xA5) {
        inA5 = true;
        a5pos = 0;
        a5buf[a5pos++] = b;
      } else {
        g_rs485OtherBytes++;
      }
      continue;
    }

    a5buf[a5pos++] = b;

    if (a5pos == sizeof(a5buf)) {
      processA5Frame();
      inA5 = false;
      a5pos = 0;
    }
  }
}

static void sendRS485Diagnostic()
{
  // Harmless unknown diagnostic frame.
  // Other nodes should ignore TYPE 0x7E.
  static uint16_t seq = 0;

  uint8_t f[14] = {0};
  f[0] = 0xA5;
  f[1] = 3;       // temporary AudioNode test ID
  f[2] = 0xFF;    // broadcast / diagnostic
  f[3] = 0x7E;    // TEST016 diagnostic type
  f[4] = (uint8_t)(seq & 0xFF);
  f[5] = (uint8_t)(seq >> 8);

  uint32_t up = millis() / 1000UL;
  f[6] = (uint8_t)(up & 0xFF);
  f[7] = (uint8_t)((up >> 8) & 0xFF);
  f[8] = (uint8_t)((up >> 16) & 0xFF);
  f[9] = (uint8_t)((up >> 24) & 0xFF);

  f[13] = xorCRC(f, 13);

  Serial2.write(f, sizeof(f));
  Serial2.flush();

  seq++;
  g_rs485TxFrames++;

  sayf("rs485 ...... TX TEST frame seq=%u bytes=14 total=%u",
       (unsigned)(seq - 1), (unsigned)g_rs485TxFrames);

  triggerLedEvent(LED_EVENT_TX);
}

static void startRS485()
{
  Serial2.setRxBufferSize(1024); // absorb bounded catalogue bursts while servicing audio
  Serial2.begin(RS485_BAUD, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);

  sayf("rs485 ...... ready %d baud RX=%d TX=%d",
       RS485_BAUD, PIN_RS485_RX, PIN_RS485_TX);
}

/* ============================================================
 * WEB DIAGNOSTICS
 * ============================================================ */

static String yn(bool v)
{
  return v ? "OK" : "FAIL";
}

static String localIPString()
{
#if USE_WIFI
  return WiFi.localIP().toString();
#else
  return ETH.localIP().toString();
#endif
}

static String page()
{
  String p;
  p.reserve(5200);

  p += F("<!doctype html><html><head><meta charset='utf-8'>");
  p += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  p += F("<title>BG Bed AudioNode TEST055</title>");
  p += F("<style>"
         "body{font-family:system-ui;background:#111722;color:#e8eef7;padding:18px}"
         ".c{background:#1c2635;border-radius:14px;padding:14px;margin:10px 0}"
         ".ok{color:#65e68a}.bad{color:#ff6b6b}"
         "button{min-height:60px;min-width:120px;padding:14px 20px;margin:6px;border:0;border-radius:12px;font-size:22px;background:#d6e9ff;color:#10243a}"
         "code{color:#9bd4ff}"
         "</style></head><body>");

  p += F("<h2>BG BED AudioNode TEST055</h2><a href='/lights'>פתיחת שלט תאורת אווירה</a>");

  p += F("<div class='c'><h2>Ambient light</h2><p>");
  p += ambient::name();
  p += F(" | PCM blocks: "); p += String(ambient::pcmBlocks.load());
  if (!ambient::pcmBlocks.load()) p += F(" (waiting for real audio samples)");
  p += F(" | Brightness: "); p += String(ambient::brightness);
  p += F("/255 (cap 64)</p><form action='/ambient' method='post'>");
  p += F("<button name='cmd' value='0'>Off</button><button name='cmd' value='1'>Mood</button><button name='cmd' value='2'>Music</button><br>");
  p += F("<button name='cmd' value='3'>Left (15)</button><button name='cmd' value='4'>Right (16)</button><button name='cmd' value='5'>Both</button><br>");
  p += F("<button name='cmd' value='c'>Color</button><button name='cmd' value='-'>Dimmer</button><button name='cmd' value='+'>Brighter</button></form>");
  p += F("<p>Music needs real PCM; silence or a missing callback means dark. 12 RGBW LEDs per output; 24 total.</p></div>");


  p += F("<div class='c'>Ethernet: <b class='");
  p += (g_ethLink && g_haveIP) ? "ok'>OK" : "bad'>FAIL";
  p += F("</b><br>IP: <code>");
  p += g_haveIP ? localIPString() : "none";
  p += F("</code></div>");

  p += F("<div class='c'>AudioNode SD: DISABLED for TEST055; Listener owns WAV storage</div>");

  p += F("<div class='c'>RS485 bytes: ");
  p += String(g_rs485Bytes);
  p += F("<br>A5 good: ");
  p += String(g_rs485A5Good);
  p += F("<br>A5 bad CRC: ");
  p += String(g_rs485A5Bad);
  p += F("<br>other bytes: ");
  p += String(g_rs485OtherBytes);
  p += F("<br>TX test frames: ");
  p += String(g_rs485TxFrames);

  if (g_rs485A5Good) {
    p += F("<br>last: src=");
    p += String(g_lastSrc);
    p += F(" dst=");
    p += String(g_lastDst);
    p += F(" type=0x");
    char hexbuf[5];
    snprintf(hexbuf, sizeof(hexbuf), "%02X", g_lastType);
    p += hexbuf;
    p += F(" seq=");
    p += String(g_lastSeq);
  }
  p += F("</div>");

  p += F("<div class='c'>NeoPixels: <b class='ok'>initialized - 7 RGBW</b>");
  p += F("<br>From DATA IN: 1 heartbeat (2s on/2s off); 2 Ethernet; 3 IP; 4 SD (off); 5 RX (blue); 6 TX (amber); 7 audio.");
  p += F("<br>RX red = CRC error (5s). Audio: off=stopped, amber=connecting, green=player running, red=failed.");
  p += F("<br>Audio configured: <b class='");
  p += g_audioConfigured ? "ok'>OK" : "bad'>FAIL";
  p += F("</b><br>Audio running: ");
  p += audio && audio->isRunning() ? "YES" : "NO";
  p += F("<br>Source: ");
  p += g_playbackSource == PlaybackSource::Dream ? "Dream WAV" :
       g_playbackSource == PlaybackSource::Radio ? "Radio" : "None";
  p += F("<br>Listener: wired UART, RX6 TX4 READY5");
  p += F("</div>");

  p += F("<div class='c'>");
  p += F("<form action='/play'><button>Play test radio</button></form>");
  p += F("<form action='/stop'><button>Stop audio</button></form>");
  p += F("<form action='/rs485tx'><button>Send RS485 diagnostic frame</button></form>");
  p += F("</div>");

  p += F("<div class='c'>Overall: <b class='");
  p += coreHealthy() ? "ok'>CORE OK" : "bad'>CHECK RED ITEM";
  p += F("</b></div>");

  p += F("</body></html>");
  return p;
}

static String lightsPage() {
  String p; p.reserve(3500);
  p += F("<!doctype html><html lang='he' dir='rtl'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>תאורת אווירה TEST055</title><style>body{font:24px system-ui;background:#111722;color:#eef5ff;margin:20px;max-width:850px}button{font:inherit;min-height:70px;min-width:140px;padding:14px;margin:8px;border:0;border-radius:16px;background:#c7e4ff;color:#10243a}h1{font-size:30px}.status{background:#263c55;padding:18px;border-radius:14px}a{color:#88cfff}</style></head><body><h1>תאורת אווירה · TEST055</h1><div class='status'>מצב: ");
  p += ambient::mode == ambient::Off ? "כבוי" : ambient::mode == ambient::Mood ? "אווירה" : "לפי המוזיקה";
  p += F(" · צד: "); p += ambient::sides == 1 ? "שמאל" : ambient::sides == 2 ? "ימין" : "שניהם";
  static const char* colors[] = {"חם", "ורוד", "סגול", "כחול", "ירוק", "לבן"};
  p += F("<br>צבע: "); p += colors[ambient::color];
  p += F(" · עוצמה: "); p += String((unsigned)ambient::brightness * 100 / 64);
  p += F("% מתקרת הבדיקה</div><form action='/ambient' method='post'><button name='cmd' value='0'>כבוי</button><button name='cmd' value='1'>אווירה</button><button name='cmd' value='2'>לפי המוזיקה</button><br><button name='cmd' value='c'>החלפת צבע</button><button name='cmd' value='-'>פחות אור −</button><button name='cmd' value='+'>יותר אור +</button><br><button name='cmd' value='3'>שמאל · 15</button><button name='cmd' value='5'>שניהם</button><button name='cmd' value='4'>ימין · 16</button></form><p>12 נורות בכל צד. התאורה אינה מפעילה או עוצרת את הרדיו.</p><p>דגימות מוזיקה שנקלטו (בלוקים): ");
  p += String(ambient::pcmBlocks.load());
  if (!ambient::pcmBlocks.load()) p += F(" — עדיין ממתין לדגימות שמע. במצב מוזיקה האור נשאר כבוי עד לקבלתן.");
  p += F("</p><a href='/lights'>רענון מצב</a> · <a href='/'>אבחון ורדיו</a></body></html>");
  return p;
}

static void startWeb()
{
  if (g_webStarted) return;

  server.on("/lights", []() { server.send(200, "text/html; charset=utf-8", lightsPage()); });

  server.on("/", []() {
    server.send(200, "text/html", page());
  });

  server.on("/play", []() {
    if (g_haveIP) {
      say(F("audio ...... web PLAY test stream"));
      startRadioFade(VOLUME_START, 1500);
      catalogAudioMutation();
    }
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.on("/stop", []() {
    if (g_playbackSource == PlaybackSource::Dream) stopDreamPlayback();
    else { stopRadioCommand(); catalogAudioMutation(); }
    say(F("audio ...... web STOP"));
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.on("/rs485tx", []() {
    sendRS485Diagnostic();
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.on("/ambient", HTTP_POST, []() {
    const String cmd = server.arg("cmd");
    if (cmd.length() != 1 || !ambient::command(cmd[0])) {
      server.send(400, "text/plain", "Invalid ambient command"); return;
    }
    ambient::diagnostics();
    server.sendHeader("Location", "/lights");
    server.send(303, "text/plain", "");
  });

  server.begin();
  g_webStarted = true;

  sayf("web ........ http://%s/", localIPString().c_str());
}

/* ============================================================
 * NETWORK
 * ============================================================ */

static void onNetEvent(arduino_event_id_t event)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname("audionode");
      say(F("eth ........ driver started"));
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      g_ethLink = true;
      say(F("eth ........ LINK UP"));
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      g_ethLink = true;
      g_haveIP = true;
      sayf("eth ........ IP %s", ETH.localIP().toString().c_str());
      startWeb();
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      g_ethLink = false;
      g_haveIP = false;
      say(F("eth ........ LINK DOWN"));
      break;

    case ARDUINO_EVENT_ETH_STOP:
      g_ethLink = false;
      g_haveIP = false;
      say(F("eth ........ STOPPED"));
      break;

    default:
      break;
  }
}

static void startEthernet()
{
  Network.onEvent(onNetEvent);

  pinMode(PIN_ETH_RST, OUTPUT);
  digitalWrite(PIN_ETH_RST, LOW);
  delay(25);
  digitalWrite(PIN_ETH_RST, HIGH);
  delay(120);

  SPI.begin(PIN_ETH_SCK, PIN_ETH_MISO, PIN_ETH_MOSI);

  sayf("eth pins .... SCK %d MISO %d MOSI %d CS %d INT %d RST %d",
       PIN_ETH_SCK, PIN_ETH_MISO, PIN_ETH_MOSI,
       PIN_ETH_CS, PIN_ETH_INT, PIN_ETH_RST);

  say(F("eth ........ calling ETH.begin"));

  if (!ETH.begin(
        ETH_PHY_W5500,
        ETH_PHY_ADDRESS,
        PIN_ETH_CS,
        PIN_ETH_INT,
        PIN_ETH_RST,
        SPI,
        ETH_SPI_MHZ)) {
    say(F("eth ........ ETH.begin FAILED"));
  }
}

/* ============================================================
 * AUDIO
 * ============================================================ */

static void startAudio()
{
  if (audio) return;
  audio = new Audio();
  if (!audio->setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT)) {
    delete audio;
    audio = nullptr;
    g_audioConfigured = false;
    say(F("audio ...... I2S initialization failed"));
    return;
  }
  audio->setVolume(VOLUME_START);
  g_audioConfigured = true;

  sayf("audio ...... PCM5102A configured BCLK=%d LRC=%d DOUT=%d volume=%d",
       PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT, VOLUME_START);
}

static void updateAudioFade()
{
  if (!g_fadeActive || !audio) return;

  const unsigned long elapsed = millis() - g_fadeStartMs;
  int v;
  if (elapsed >= g_fadeDurationMs) {
    v = g_fadeTarget;
    g_fadeActive = false;
  } else {
    v = (int)((elapsed * g_fadeTarget) / g_fadeDurationMs);
  }

  if (v != g_lastAppliedVolume) {
    audio->setVolume(v);
    g_lastAppliedVolume = v;
    sayf("audio fade . volume=%d/%u", v, g_fadeTarget);
  }
}

void audio_info(const char *info)
{
  if (!info) return;
  sayf("audio ...... %s", info);
}

void audio_showstation(const char *info)
{
  if (info) sayf("station .... %s", info);
}

void audio_showstreamtitle(const char *info)
{
  if (info) sayf("playing .... %s", info);
}

void audio_bitrate(const char *info)
{
  if (info) sayf("bitrate .... %s", info);
}

void audio_eof_stream(const char *info)
{
  if (info) {
    sayf("audio ...... stream ended: %s", info);
  }
}

/* ============================================================
 * SERIAL COMMANDS
 * ============================================================ */

static void printDiagnostics()
{
  say(F(""));
  say(F("================ TEST055 DIAGNOSTICS ================"));
  sayf("ethernet .... link=%s ip=%s",
       g_ethLink ? "UP" : "DOWN",
       g_haveIP ? localIPString().c_str() : "none");

  say(F("sd .......... DISABLED for TEST055; Listener holds WAV"));

  sayf("rs485 ....... bytes=%u A5good=%u A5bad=%u other=%u tx=%u",
       (unsigned)g_rs485Bytes,
       (unsigned)g_rs485A5Good,
       (unsigned)g_rs485A5Bad,
       (unsigned)g_rs485OtherBytes,
       (unsigned)g_rs485TxFrames);

  if (g_rs485A5Good) {
    sayf("rs485 last .. src=%u dst=%u type=0x%02X seq=%u",
         g_lastSrc, g_lastDst, g_lastType, g_lastSeq);
  }

  sayf("pixels ...... %d RGBW on GPIO%d", PIXEL_COUNT, PIN_PIXELS);
  say(F("LED 1 heartbeat 2s on/off | 2 Ethernet green/red | 3 IP green/amber | 4 SD disabled"));
  say(F("LED 5 RX blue 1.5s / CRC red 5s | 6 TX amber 1.5s | 7 audio off/amber/green/red"));
  sayf("audio ....... configured=%s running=%s",
       g_audioConfigured ? "YES" : "NO",
       audio && audio->isRunning() ? "YES" : "NO");
  sayf("dream ....... WIRED %s RX6 TX4 READY5 state=%u received=%lu/%lu",
       wiredDream::lastPongMs && millis() - wiredDream::lastPongMs < 5000 ? "CONNECTED" : "NO LINK",
       dreamPlaybackState(), (unsigned long)wiredDream::received,
       (unsigned long)wiredDream::expected);

  ambient::diagnostics();
  sayf("heap ........ %u", (unsigned)ESP.getFreeHeap());
  sayf("psram ....... %u", (unsigned)ESP.getFreePsram());
  sayf("overall ..... %s", coreHealthy() ? "CORE OK" : "CHECK FAILURE");
  say(F("======================================================"));
  say(F(""));
}

static void handleDreamSerialLine(String line) {
  line.trim();
  if (line == "dreamconfig")
    sayf("dream ...... wired %s state=%u RX6 TX4 READY5 bytes=%lu/%lu",
         wiredDream::lastPongMs && millis() - wiredDream::lastPongMs < 5000 ? "CONNECTED" : "NO LINK",
         dreamPlaybackState(), (unsigned long)wiredDream::received,
         (unsigned long)wiredDream::expected);
  else say(F("dream ...... host/token settings retired; Listener Wi-Fi is OFF"));
}

static void pollUSB()
{
  while (console.available()) {
    char c = (char)console.read();
    // The legacy single-key menu remains immediate. Dream settings are lines
    // so an embedded letter cannot accidentally trigger ambient or STOP.
    if (g_dreamSerialLine.length() || c == 'd' || c == 'D') {
      if (c == '\n' || c == '\r') {
        if (g_dreamSerialLine.length()) handleDreamSerialLine(g_dreamSerialLine);
        g_dreamSerialLine = "";
      } else if (g_dreamSerialLine.length() < 100) g_dreamSerialLine += c;
      else { g_dreamSerialLine = ""; say(F("dream ...... command too long")); }
    } else if (c == '\r' || c == '\n') {
      continue;
    } else if (c == 't' || c == 'T') {
      if (g_playbackSource == PlaybackSource::Dream) {
        say(F("dream ...... stop playback before DAC tone test"));
      } else {
        stopRadioCommand();
        if (audio) { delete audio; audio = nullptr; }
        say(F("dream ...... direct DAC tone test, 440 Hz for 0.8 seconds"));
        const bool okay = wiredDream::toneTest();
        say(okay ? F("dream ...... tone I2S write OK") :
                   F("dream ...... tone I2S write FAILED"));
        startAudio();
      }
    } else if (ambient::command(c)) {
      ambient::diagnostics();
    } else if (c == 'i' || c == 'I') {
      printDiagnostics();
    } else if (c == 'x' || c == 'X') {
      sendRS485Diagnostic();
    } else if (c == 'p' || c == 'P') {
      if (!g_haveIP) {
        say(F("audio ...... cannot play, no IP"));
      } else {
        say(F("audio ...... PLAY test stream"));
        startRadioFade(VOLUME_START, 1500);
        catalogAudioMutation();
      }
    } else if (c == 's' || c == 'S') {
      if (g_playbackSource == PlaybackSource::Dream) stopDreamPlayback();
      else { stopRadioCommand(); catalogAudioMutation(); }
      say(F("audio ...... STOP"));
    }
  }
}

/* ============================================================
 * SETUP
 * ============================================================ */

void setup()
{
  console.begin(115200);

  // Native USB CDC may enumerate after boot. UART0 is available immediately.
  // Do not block waiting for either terminal; print six visible boot pulses.
  delay(250);
  for (int i = 0; i < 6; ++i) {
    console.printf("\r\nSERIAL ALIVE - AUDIO NODE TEST %03d - boot pulse %d/6\r\n",
                  TEST_NUMBER, i + 1);
    console.flush();
    delay(250);
  }

  g_bootMs = millis();

  console.println();
  console.println(F("============================================================"));
  console.println(F("  BG BED AUDIO NODE - TEST 055 - FAST VERIFIED DREAM PLAYER"));
  console.println(F("============================================================"));
  console.println(F("  NOT YET HARDWARE PROVEN"));
  console.println();

  sayf("flash ....... %u bytes", (unsigned)ESP.getFlashChipSize());
  sayf("psram ....... %u bytes", (unsigned)ESP.getPsramSize());

  ambient::begin();
  pixels.begin();
  pixels.setBrightness(30);
  startupPixelTest();
  say(F("LEDs from DATA IN: 1=heartbeat 2=Ethernet 3=IP 4=SD(off) 5=RX 6=TX 7=audio"));
  say(F("LEDs: heartbeat 2s on/2s off; RX blue, TX amber 1.5s; RX red=CRC error 5s"));
  say(F("Audio LED: off=stopped, amber=connecting, green=player running, red=failed"));

  startRS485();
  say(F("sd ......... DISABLED in TEST055 - WAV resides on Listener"));
  startAudio();
  catalogBegin();
  say(wiredDream::begin() ? F("dream ...... wired transport ready") :
      F("dream ...... PSRAM ring allocation FAILED"));
  startEthernet();

  printDiagnostics();

  say(F("ambient ..... 0=off 1=mood 2=music 3=GPIO15 only 4=GPIO16 only 5=both c=color +/-=brightness"));
  say(F("commands .... i=info  x=RS485 TX  p=play radio  s=stop  t=DAC tone"));
  say(F("dream ...... wired connector: TX4->RX15 RX6<-TX14 READY5->21 GND"));
}

/* ============================================================
 * LOOP
 * ============================================================ */

void loop()
{
  // Radio decoding is idle while its I2S pins are parked for Dream playback.
  if (audio && g_playbackSource != PlaybackSource::Dream) audio->loop();
  wiredDream::service();
  if (g_playbackSource == PlaybackSource::Dream && !g_dreamTransferLogged &&
      wiredDream::receiveDone.load()) {
    g_dreamTransferLogged = true;
    sayf("dream ...... verified %lu bytes in %lu ms, blocks=%lu bad=%lu",
         (unsigned long)wiredDream::received,
         (unsigned long)wiredDream::transferMs,
         (unsigned long)wiredDream::goodBlocks,
         (unsigned long)wiredDream::badBlocks);
  }
  if (g_playbackSource == PlaybackSource::Dream && !g_playFailed && !g_dreamPaused &&
      (wiredDream::status.load() == 0 || wiredDream::status.load() == 3)) {
    const bool failed = wiredDream::status.load() == 3;
    if (failed)
      sayf("dream ...... failed fault=%u received=%lu/%lu played=%lu CRC=%08lX blocks=%lu bad=%lu",
           (unsigned)wiredDream::fault.load(),
           (unsigned long)wiredDream::received,
           (unsigned long)wiredDream::expected,
           (unsigned long)wiredDream::readAt.load(),
           (unsigned long)(wiredDream::crc ^ 0xFFFFFFFFU),
           (unsigned long)wiredDream::goodBlocks,
           (unsigned long)wiredDream::badBlocks);
    if (wiredDream::stop(false)) {
      startAudio();
      g_playFailed = failed || !audio;
      if (!failed) g_playbackSource = PlaybackSource::None;
      say(failed ? F("dream ...... WIRED TRANSFER FAILED") :
                   F("dream ...... wired playback complete"));
    } else g_playFailed = true;
  }
  updateAudioFade();
  pollUSB();
  pollRS485();
  // sendAudioAck() has already flushed the response to the panel. Starting
  // the player here cannot consume the panel's 650 ms RS485 reply window.
  if (g_dreamStartPending) {
    const uint8_t rank = g_dreamPendingRank;
    const uint16_t key = g_dreamPendingKey;
    const bool selected = g_dreamPendingSelected;
    g_dreamStartPending = false;
    say(F("dream ...... RS485 ACK sent; starting wired playback"));
    if (!startDreamPlayback(rank, key, selected)) {
      g_dreamStartFailed = true;
      say(F("dream ...... start failed after ACK; status now reports ERROR"));
    }
  }
  catalogService();
  serviceRadioConnection();

  if (g_haveIP && g_webStarted) {
    server.handleClient();
  }

  updatePixels();
  ambient::tick(audio && audio->isRunning());

  unsigned long now = millis();

  // Independent dual-port terminal heartbeat.
  // If this appears, the app is alive even if every
  // peripheral below is disconnected.
  if (now - g_lastSerialHeartbeat >= 1000) {
    g_lastSerialHeartbeat = now;
    console.printf("SERIAL ALIVE | TEST %03d | uptime=%lus | ETH=%s | IP=%s | SD=%s | RS485bytes=%u\r\n",
                  TEST_NUMBER,
                  (now - g_bootMs) / 1000UL,
                  g_ethLink ? "UP" : "DOWN",
                  g_haveIP ? "YES" : "NO",
                  "DISABLED",
                  (unsigned)g_rs485Bytes);
    console.flush();
  }

  if (now - g_lastStatus >= STATUS_EVERY_MS) {
    g_lastStatus = now;

    sayf("[%5lus] ETH=%s IP=%s SD=%s RS485_A5=%u CRCbad=%u AUDIO=%s",
         (now - g_bootMs) / 1000UL,
         g_ethLink ? "UP" : "DOWN",
         g_haveIP ? "YES" : "NO",
         "DISABLED",
         (unsigned)g_rs485A5Good,
         (unsigned)g_rs485A5Bad,
         g_playbackSource == PlaybackSource::Dream && wiredDream::status.load() == 2 ?
             "DREAM" : audio && audio->isRunning() ? "PLAY" : "IDLE");
  }
}

/* ============================================================
 *            END BG BED AUDIO NODE - TEST 055
 * ============================================================ */
