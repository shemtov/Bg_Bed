/* ============================================================
 *                        TEST  016
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
 *  TEST016 SERIAL RECOVERY:
 *    Forces ESP32-S3 native USB CDC serial on boot via platformio.ini.
 *    Prints an unmistakable USB SERIAL ALIVE heartbeat every 1 second.
 *    The heartbeat runs regardless of Ethernet, SD, RS485 or Audio state.
 *    This isolates terminal/USB from the rest of the AudioNode.
 *
 *  PURPOSE:
 *    One firmware to prove that Ethernet + internal network,
 *    SD card, RS485 receive/transmit path, 7 RGBW pixels and
 *    PCM5102A audio can coexist on the AudioNode.
 *
 *  LED diagnostics:
 *    startup: RED -> GREEN -> BLUE -> WHITE
 *    normal:  one GREEN pixel scans
 *    fault:   one RED pixel scans
 *    RS485 RX valid A5 frame: BLUE flash
 *    RS485 TX diagnostic frame: YELLOW flash
 *    bad A5 CRC: all RED flash
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
 *    TEST016 does NOT execute audio commands from RS485 yet.
 *    It only proves transport and reports what it sees.
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

#define TEST_NUMBER 16

#define USE_WIFI 0

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ETH.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include "Audio.h"

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

static const char *TEST_STREAM =
  "https://ice5.somafm.com/groovesalad-128-mp3";

/* ============================================================
 * OBJECTS
 * ============================================================ */

Audio audio;
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

static bool g_ethLink = false;
static bool g_haveIP = false;
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

enum LedEvent {
  LED_EVENT_NONE,
  LED_EVENT_RX,
  LED_EVENT_TX,
  LED_EVENT_BADCRC
};

static LedEvent g_ledEvent = LED_EVENT_NONE;
static unsigned long g_ledEventUntil = 0;
static int g_scanPos = 0;
static int g_scanDir = 1;
static unsigned long g_lastLedStep = 0;

/* ============================================================
 * LOG
 * ============================================================ */

static void say(const String &s)
{
  Serial.println(s);
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

static void startupPixelTest()
{
  pixelsAll(40, 0, 0, 0);
  delay(250);

  pixelsAll(0, 40, 0, 0);
  delay(250);

  pixelsAll(0, 0, 40, 0);
  delay(250);

  // RGBW proof: WHITE channel only.
  pixelsAll(0, 0, 0, 35);
  delay(350);

  pixelsOff();
}

static void triggerLedEvent(LedEvent ev)
{
  g_ledEvent = ev;
  g_ledEventUntil = millis() + LED_EVENT_MS;
}

static bool coreHealthy()
{
  // RS485 is passive in this test; no received packet is not a fault.
  return g_sdOK && g_sdWriteOK && g_ethLink && g_haveIP && g_audioConfigured;
}

static void updatePixels()
{
  unsigned long now = millis();

  if (g_ledEvent != LED_EVENT_NONE && now < g_ledEventUntil) {
    if (g_ledEvent == LED_EVENT_RX) {
      pixelsAll(0, 0, 55, 0);                 // blue
    } else if (g_ledEvent == LED_EVENT_TX) {
      pixelsAll(45, 35, 0, 0);                // yellow
    } else {
      pixelsAll(65, 0, 0, 0);                 // red
    }
    return;
  }

  if (g_ledEvent != LED_EVENT_NONE && now >= g_ledEventUntil) {
    g_ledEvent = LED_EVENT_NONE;
    pixelsOff();
  }

  if (now - g_lastLedStep < LED_STEP_MS) return;
  g_lastLedStep = now;

  pixels.clear();

  if (coreHealthy()) {
    pixels.setPixelColor(g_scanPos, pixels.Color(0, 40, 0, 0));
  } else {
    pixels.setPixelColor(g_scanPos, pixels.Color(45, 0, 0, 0));
  }

  pixels.show();

  g_scanPos += g_scanDir;
  if (g_scanPos >= PIXEL_COUNT - 1) {
    g_scanPos = PIXEL_COUNT - 1;
    g_scanDir = -1;
  } else if (g_scanPos <= 0) {
    g_scanPos = 0;
    g_scanDir = 1;
  }
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

  f.printf("BG BED AudioNode TEST016 boot=%lu ms\r\n", millis());
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

  sayf("rs485 ...... A5 OK src=%u dst=%u type=0x%02X seq=%u total=%u",
       g_lastSrc, g_lastDst, g_lastType, g_lastSeq,
       (unsigned)g_rs485A5Good);

  triggerLedEvent(LED_EVENT_RX);
}

static void pollRS485()
{
  while (Serial2.available()) {
    uint8_t b = (uint8_t)Serial2.read();
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
  p.reserve(3500);

  p += F("<!doctype html><html><head><meta charset='utf-8'>");
  p += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  p += F("<title>BG Bed AudioNode TEST016</title>");
  p += F("<style>"
         "body{font-family:system-ui;background:#111722;color:#e8eef7;padding:18px}"
         ".c{background:#1c2635;border-radius:14px;padding:14px;margin:10px 0}"
         ".ok{color:#65e68a}.bad{color:#ff6b6b}"
         "button{padding:13px 18px;margin:5px;border:0;border-radius:10px;font-size:16px}"
         "code{color:#9bd4ff}"
         "</style></head><body>");

  p += F("<h2>BG BED AudioNode TEST016</h2>");

  p += F("<div class='c'>Ethernet: <b class='");
  p += (g_ethLink && g_haveIP) ? "ok'>OK" : "bad'>FAIL";
  p += F("</b><br>IP: <code>");
  p += g_haveIP ? localIPString() : "none";
  p += F("</code></div>");

  p += F("<div class='c'>SD mount: <b class='");
  p += g_sdOK ? "ok'>OK" : "bad'>FAIL";
  p += F("</b><br>SD write/read: <b class='");
  p += g_sdWriteOK ? "ok'>OK" : "bad'>FAIL";
  p += F("</b><br>Size: ");
  p += String((unsigned long long)g_sdSizeMB);
  p += F(" MB</div>");

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
  p += F("<br>Audio configured: <b class='");
  p += g_audioConfigured ? "ok'>OK" : "bad'>FAIL";
  p += F("</b><br>Audio running: ");
  p += audio.isRunning() ? "YES" : "NO";
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

static void startWeb()
{
  if (g_webStarted) return;

  server.on("/", []() {
    server.send(200, "text/html", page());
  });

  server.on("/play", []() {
    if (g_haveIP) {
      say(F("audio ...... web PLAY test stream"));
      audio.connecttohost(TEST_STREAM);
    }
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.on("/stop", []() {
    audio.stopSong();
    say(F("audio ...... web STOP"));
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.on("/rs485tx", []() {
    sendRS485Diagnostic();
    server.sendHeader("Location", "/");
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
  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.setVolume(VOLUME_START);
  g_audioConfigured = true;

  sayf("audio ...... PCM5102A configured BCLK=%d LRC=%d DOUT=%d volume=%d",
       PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT, VOLUME_START);
}

void audio_info(const char *info)
{
  if (info) sayf("audio ...... %s", info);
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
  if (info) sayf("audio ...... stream ended: %s", info);
}

/* ============================================================
 * SERIAL COMMANDS
 * ============================================================ */

static void printDiagnostics()
{
  say(F(""));
  say(F("================ TEST016 DIAGNOSTICS ================"));
  sayf("ethernet .... link=%s ip=%s",
       g_ethLink ? "UP" : "DOWN",
       g_haveIP ? localIPString().c_str() : "none");

  sayf("sd .......... mount=%s write/read=%s size=%llu MB",
       g_sdOK ? "OK" : "FAIL",
       g_sdWriteOK ? "OK" : "FAIL",
       (unsigned long long)g_sdSizeMB);

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
  sayf("audio ....... configured=%s running=%s",
       g_audioConfigured ? "YES" : "NO",
       audio.isRunning() ? "YES" : "NO");

  sayf("heap ........ %u", (unsigned)ESP.getFreeHeap());
  sayf("psram ....... %u", (unsigned)ESP.getFreePsram());
  sayf("overall ..... %s", coreHealthy() ? "CORE OK" : "CHECK FAILURE");
  say(F("======================================================"));
  say(F(""));
}

static void pollUSB()
{
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == 'i' || c == 'I') {
      printDiagnostics();
    } else if (c == 'x' || c == 'X') {
      sendRS485Diagnostic();
    } else if (c == 'p' || c == 'P') {
      if (!g_haveIP) {
        say(F("audio ...... cannot play, no IP"));
      } else {
        say(F("audio ...... PLAY test stream"));
        audio.connecttohost(TEST_STREAM);
      }
    } else if (c == 's' || c == 'S') {
      audio.stopSong();
      say(F("audio ...... STOP"));
    }
  }
}

/* ============================================================
 * SETUP
 * ============================================================ */

void setup()
{
  Serial.begin(115200);

  // TEST016: native USB CDC may enumerate a moment after boot.
  // Do not block the firmware waiting for a terminal. Instead keep
  // printing a visible boot pulse while USB is coming up.
  delay(250);
  for (int i = 0; i < 6; ++i) {
    Serial.printf("\r\nUSB SERIAL ALIVE - AUDIO NODE TEST %03d - boot pulse %d/6\r\n",
                  TEST_NUMBER, i + 1);
    Serial.flush();
    delay(250);
  }

  g_bootMs = millis();

  Serial.println();
  Serial.println(F("============================================================"));
  Serial.println(F("  BG BED AUDIO NODE - TEST 016 - USB SERIAL + FULL INTEGRATION TEST"));
  Serial.println(F("============================================================"));
  Serial.println(F("  NOT YET HARDWARE PROVEN"));
  Serial.println();

  sayf("flash ....... %u bytes", (unsigned)ESP.getFlashChipSize());
  sayf("psram ....... %u bytes", (unsigned)ESP.getPsramSize());

  pixels.begin();
  pixels.setBrightness(120);
  startupPixelTest();
  say(F("pixels ...... startup RGBW sequence finished"));

  startRS485();
  startSD();
  startAudio();
  startEthernet();

  printDiagnostics();

  say(F("commands .... i=info  x=RS485 TX  p=play radio  s=stop"));
}

/* ============================================================
 * LOOP
 * ============================================================ */

void loop()
{
  audio.loop();
  pollUSB();
  pollRS485();

  if (g_haveIP && g_webStarted) {
    server.handleClient();
  }

  updatePixels();

  unsigned long now = millis();

  // TEST016: independent USB terminal heartbeat.
  // If this appears, USB Serial itself is alive even if every
  // peripheral below is disconnected.
  if (now - g_lastSerialHeartbeat >= 1000) {
    g_lastSerialHeartbeat = now;
    Serial.printf("USB SERIAL ALIVE | TEST %03d | uptime=%lus | ETH=%s | IP=%s | SD=%s | RS485bytes=%u\r\n",
                  TEST_NUMBER,
                  (now - g_bootMs) / 1000UL,
                  g_ethLink ? "UP" : "DOWN",
                  g_haveIP ? "YES" : "NO",
                  (g_sdOK && g_sdWriteOK) ? "OK" : "FAIL",
                  (unsigned)g_rs485Bytes);
    Serial.flush();
  }

  if (now - g_lastStatus >= STATUS_EVERY_MS) {
    g_lastStatus = now;

    sayf("[%5lus] ETH=%s IP=%s SD=%s RS485_A5=%u CRCbad=%u AUDIO=%s",
         (now - g_bootMs) / 1000UL,
         g_ethLink ? "UP" : "DOWN",
         g_haveIP ? "YES" : "NO",
         (g_sdOK && g_sdWriteOK) ? "OK" : "FAIL",
         (unsigned)g_rs485A5Good,
         (unsigned)g_rs485A5Bad,
         audio.isRunning() ? "PLAY" : "IDLE");
  }
}

/* ============================================================
 *                        TEST  016
 *   AUDIO NODE - Ethernet + SD + RS485 + 7 RGBW + PCM5102A
 *   NOT YET TESTED ON HARDWARE.
 * ============================================================ */
