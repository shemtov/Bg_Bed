/* ============================================================
 *                        TEST  010
 * ============================================================
 *
 *  AUDIO NODE  -  internet radio over wired Ethernet
 *
 *  Board : ESP32-S3-WROOM-1  N16R8   (16MB flash, 8MB psram, confirmed)
 *  Net   : W5500 over SPI            (confirmed working, TEST 006)
 *  Out   : PCM5102A I2S DAC          (confirmed working, TEST 009 tone)
 *
 *  WHAT TEST 009 SETTLED
 *    The tone played. So the DAC, the solder bridges, the three
 *    signal wires, the optical cable and the soundbar are all good.
 *    The faulty part was the analog-to-digital converter box.
 *    It has been replaced. Nothing in the firmware was ever wrong.
 *
 *  WHAT THIS BUILD DOES
 *    - brings up the wired network
 *    - starts playing by itself once it has an address (AUTOPLAY)
 *    - a station table, played by number
 *    - serial commands so you can try ANY url without rebuilding
 *
 *  SERIAL COMMANDS  (type into the monitor, press enter)
 *    l              list the stations
 *    0 1 2 3 ...    play that station number
 *    +   -          volume up / down
 *    s              stop
 *    i              info: heap, psram, link, volume
 *    u <url>        play any url you paste
 *
 *  NOT TESTED ON HARDWARE. Build to confirm.
 * ============================================================ */

#define TEST_NUMBER 10

#include <Arduino.h>
#include <SPI.h>
#include <ETH.h>
#include "Audio.h"

/* ---------- pin map ---------- */

#define PIN_ETH_SCK   12
#define PIN_ETH_MOSI  11
#define PIN_ETH_MISO  10
#define PIN_ETH_CS     9
#define PIN_ETH_INT    8
#define PIN_ETH_RST    7

#define PIN_I2S_BCLK  14
#define PIN_I2S_LRC    2
#define PIN_I2S_DOUT  13

/* ---------- settings ---------- */

#define ETH_PHY_ADDRESS   1
#define ETH_SPI_MHZ       12
#define STATUS_EVERY_MS   10000
#define VOLUME_START      12
#define VOLUME_MAX        21

/* play this station by itself, a few seconds after getting an address.
 * set AUTOPLAY_STATION to -1 to disable and go back to manual only. */
#define AUTOPLAY_STATION  0
#define AUTOPLAY_DELAY_MS 3000

/* ---------- stations ----------
 *
 *  VERIFIED urls, found in public listings:
 *    Galgalatz  - plain http mp3, the friendliest kind for this chip
 *    Kan 88     - https with a redirect, more work for the chip
 *    SomaFM     - somafm say the ice5 links can move; if one dies the
 *                 alt server is ice3 with the same path
 *
 *  STILL WANTED - use the u command to hunt for these:
 *    whiskey blues, waterfalls / nature, meditation, pink floyd radio
 *  When one works, tell me and it goes into this table permanently.
 * ------------------------------------------------------------ */

struct Station {
  const char *name;
  const char *url;
};

static const Station STATIONS[] = {
  { "Galgalatz",         "http://glzwizzlv.bynetcdn.com/glglz_mp3" },
  { "Kan 88FM",          "https://playerservices.streamtheworld.com/api/livestream-redirect/KAN_88.mp3" },
  { "Groove Salad",      "https://ice5.somafm.com/groovesalad-128-mp3" },
  { "Drone Zone",        "https://ice5.somafm.com/dronezone-128-mp3" },
  { "Deep Space One",    "https://ice5.somafm.com/deepspaceone-128-mp3" },
  { "Lush",              "https://ice5.somafm.com/lush-128-mp3" },
};

static const int STATION_COUNT = sizeof(STATIONS) / sizeof(STATIONS[0]);

/* ---------- state ---------- */

Audio audio;

static bool  g_linkUp        = false;
static bool  g_haveAddress   = false;
static int   g_volume        = VOLUME_START;
static int   g_station       = -1;
static unsigned long g_lastStatus  = 0;
static unsigned long g_bootMillis  = 0;
static unsigned long g_autoplayAt  = 0;
static bool  g_autoplayDone  = false;
static String g_line;

/* ---------- helpers ---------- */

static void banner()
{
  Serial.println();
  Serial.println(F("============================================================"));
  Serial.printf ("  AUDIO NODE   -   TEST %03d   -   radio over ethernet\n", TEST_NUMBER);
  Serial.println(F("============================================================"));
}

static void printResetReason()
{
  esp_reset_reason_t r = esp_reset_reason();
  const char *name = "unknown";

  switch (r) {
    case ESP_RST_POWERON:  name = "power on";              break;
    case ESP_RST_EXT:      name = "external reset pin";    break;
    case ESP_RST_SW:       name = "software restart";      break;
    case ESP_RST_PANIC:    name = "PANIC or exception";    break;
    case ESP_RST_INT_WDT:  name = "interrupt watchdog";    break;
    case ESP_RST_TASK_WDT: name = "task watchdog";         break;
    case ESP_RST_WDT:      name = "other watchdog";        break;
    case ESP_RST_BROWNOUT: name = "BROWNOUT - power dip";  break;
    default: break;
  }

  Serial.printf("reset ...... %s\n", name);

  if (r == ESP_RST_BROWNOUT) {
    Serial.println(F("             ^^ the supply sagged. audio draws more than idle."));
  }
  if (r == ESP_RST_PANIC || r == ESP_RST_TASK_WDT || r == ESP_RST_INT_WDT) {
    Serial.println(F("             ^^ the previous run crashed."));
  }
}

static void printBoardFacts()
{
  Serial.printf("chip ....... %s  rev %d  %d core(s)  %d MHz\n",
                ESP.getChipModel(), ESP.getChipRevision(),
                ESP.getChipCores(), getCpuFrequencyMhz());

  Serial.printf("flash ...... %u bytes\n", (unsigned) ESP.getFlashChipSize());

  size_t psram = ESP.getPsramSize();
  if (psram == 0) {
    Serial.println(F("psram ...... NOT FOUND  <-- streaming will stutter without it"));
  } else {
    Serial.printf("psram ...... %u bytes free %u\n",
                  (unsigned) psram, (unsigned) ESP.getFreePsram());
  }

  Serial.printf("heap ....... %u bytes free\n", (unsigned) ESP.getFreeHeap());
}

static void listStations()
{
  Serial.println();
  Serial.println(F("stations:"));
  for (int i = 0; i < STATION_COUNT; i++) {
    Serial.printf("   %d  %s\n", i, STATIONS[i].name);
  }
  Serial.println(F("   type a number to play, u <url> to try any url"));
  Serial.println();
}

static void playStation(int index)
{
  if (index < 0 || index >= STATION_COUNT) {
    Serial.println(F("cmd ........ no such station"));
    return;
  }
  if (!g_haveAddress) {
    Serial.println(F("cmd ........ no network address yet, wait"));
    return;
  }

  g_station = index;

  Serial.println();
  Serial.printf("play ....... %s\n", STATIONS[index].name);
  Serial.printf("url ........ %s\n", STATIONS[index].url);

  if (!audio.connecttohost(STATIONS[index].url)) {
    Serial.println(F("play ....... connecttohost returned FALSE"));
    Serial.println(F("             the url did not open. wrong url, or a format"));
    Serial.println(F("             this library cannot read."));
  }
}

static void playUrl(const String &url)
{
  if (!g_haveAddress) {
    Serial.println(F("cmd ........ no network address yet, wait"));
    return;
  }

  g_station = -1;

  Serial.println();
  Serial.println(F("play ....... custom url"));
  Serial.printf ("url ........ %s\n", url.c_str());

  if (!audio.connecttohost(url.c_str())) {
    Serial.println(F("play ....... connecttohost returned FALSE"));
  }
}

static void setVolume(int v)
{
  if (v < 0) v = 0;
  if (v > VOLUME_MAX) v = VOLUME_MAX;
  g_volume = v;
  audio.setVolume(g_volume);
  Serial.printf("vol ........ %d of %d\n", g_volume, VOLUME_MAX);
}

static void printInfo()
{
  Serial.println();
  Serial.printf("link ....... %s\n", g_linkUp ? "UP" : "DOWN");
  Serial.print (F("address .... "));
  Serial.println(g_haveAddress ? ETH.localIP().toString() : String("none"));
  Serial.printf("volume ..... %d of %d\n", g_volume, VOLUME_MAX);
  Serial.printf("station .... %s\n",
                g_station >= 0 ? STATIONS[g_station].name : "custom or stopped");
  Serial.printf("running .... %s\n", audio.isRunning() ? "yes" : "no");
  Serial.printf("heap ....... %u free\n", (unsigned) ESP.getFreeHeap());
  Serial.printf("psram ...... %u free\n", (unsigned) ESP.getFreePsram());
  Serial.println();
}

/* ---------- serial command handling ---------- */

static void handleLine(String line)
{
  line.trim();
  if (line.length() == 0) return;

  char c = line.charAt(0);

  if (c == 'l' || c == 'L') { listStations(); return; }
  if (c == 'i' || c == 'I') { printInfo();    return; }
  if (c == '+')             { setVolume(g_volume + 1); return; }
  if (c == '-')             { setVolume(g_volume - 1); return; }

  if (c == 's' || c == 'S') {
    audio.stopSong();
    g_station = -1;
    Serial.println(F("cmd ........ stopped"));
    return;
  }

  if (c == 'u' || c == 'U') {
    int sp = line.indexOf(' ');
    if (sp < 0) {
      Serial.println(F("cmd ........ usage: u http://something"));
      return;
    }
    playUrl(line.substring(sp + 1));
    return;
  }

  if (c >= '0' && c <= '9') {
    playStation(line.toInt());
    return;
  }

  Serial.println(F("cmd ........ unknown. try l, i, s, +, -, a number, or u <url>"));
}

static void pollSerial()
{
  while (Serial.available()) {
    char c = (char) Serial.read();
    if (c == '\n' || c == '\r') {
      if (g_line.length()) {
        handleLine(g_line);
        g_line = "";
      }
    } else {
      if (g_line.length() < 300) g_line += c;
    }
  }
}

/* ---------- network events ---------- */

static void onNetEvent(arduino_event_id_t event)
{
  switch (event) {

    case ARDUINO_EVENT_ETH_START:
      Serial.println(F("eth ........ driver started"));
      ETH.setHostname("audionode");
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      g_linkUp = true;
      Serial.println(F("eth ........ LINK UP"));
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      g_haveAddress = true;
      Serial.println();
      Serial.println(F("------------------------------------------------------------"));
      Serial.print  (F("  IP ADDRESS   "));  Serial.println(ETH.localIP());
      Serial.print  (F("  GATEWAY      "));  Serial.println(ETH.gatewayIP());
      Serial.print  (F("  DNS          "));  Serial.println(ETH.dnsIP());
      Serial.print  (F("  MAC          "));  Serial.println(ETH.macAddress());
      Serial.printf (  "  SPEED        %d Mbps %s duplex\n",
                       ETH.linkSpeed(), ETH.fullDuplex() ? "full" : "half");
      Serial.println(F("------------------------------------------------------------"));
      listStations();
      if (AUTOPLAY_STATION >= 0 && !g_autoplayDone) {
        g_autoplayAt = millis() + AUTOPLAY_DELAY_MS;
        Serial.printf("autoplay ... will start station %d in %d ms\n",
                      AUTOPLAY_STATION, AUTOPLAY_DELAY_MS);
        Serial.println(F("             no computer needed. type s to stop it."));
      }
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      g_linkUp      = false;
      g_haveAddress = false;
      Serial.println(F("eth ........ LINK DOWN"));
      break;

    default:
      break;
  }
}

/* ---------- callbacks from the audio library ---------- */

void audio_info(const char *info)
{
  Serial.printf("audio ...... %s\n", info);
}

void audio_showstation(const char *info)
{
  Serial.println();
  Serial.printf("  STATION    %s\n", info);
}

void audio_showstreamtitle(const char *info)
{
  Serial.printf("  NOW PLAYING  %s\n", info);
}

void audio_bitrate(const char *info)
{
  Serial.printf("audio ...... bitrate %s\n", info);
}

void audio_eof_stream(const char *info)
{
  Serial.printf("audio ...... stream ended: %s\n", info);
}

/* ---------- setup ---------- */

void setup()
{
  Serial.begin(115200);

  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 2000) delay(10);
  delay(300);

  g_bootMillis = millis();

  banner();
  printResetReason();
  printBoardFacts();
  Serial.println(F("w5500 pins . sck 12  mosi 11  miso 10  cs 9  int 8  rst 7"));
  Serial.println(F("i2s pins ... bclk 14  lrc 2  dout 13"));
  Serial.println();

  Network.onEvent(onNetEvent);

  Serial.println(F("w5500 ...... resetting"));
  pinMode(PIN_ETH_RST, OUTPUT);
  digitalWrite(PIN_ETH_RST, LOW);
  delay(20);
  digitalWrite(PIN_ETH_RST, HIGH);
  delay(100);

  SPI.begin(PIN_ETH_SCK, PIN_ETH_MISO, PIN_ETH_MOSI);

  Serial.println(F("eth ........ calling begin"));

  bool ok = ETH.begin(ETH_PHY_W5500, ETH_PHY_ADDRESS,
                      PIN_ETH_CS, PIN_ETH_INT, PIN_ETH_RST,
                      SPI, ETH_SPI_MHZ);

  if (!ok) {
    Serial.println(F("  !!! ETH.begin RETURNED FALSE - wiring or power !!!"));
  }

  Serial.println(F("i2s ........ starting dac"));
  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.setVolume(g_volume);
  Serial.printf("i2s ........ ready, volume %d of %d\n", g_volume, VOLUME_MAX);

  Serial.println();
  Serial.println(F("waiting for an address, then it plays by itself."));
  Serial.println();
}

/* ---------- loop ---------- */

void loop()
{
  audio.loop();
  pollSerial();

  unsigned long now = millis();

  if (g_autoplayAt && !g_autoplayDone && now >= g_autoplayAt) {
    g_autoplayDone = true;
    g_autoplayAt   = 0;
    Serial.println(F("autoplay ... starting"));
    playStation(AUTOPLAY_STATION);
  }

  if (now - g_lastStatus >= STATUS_EVERY_MS) {
    g_lastStatus = now;

    Serial.printf("[%5lus] link %s  playing %s  heap %u  psram %u\n",
                  (now - g_bootMillis) / 1000,
                  g_linkUp ? "UP  " : "DOWN",
                  audio.isRunning() ? "yes" : "no ",
                  (unsigned) ESP.getFreeHeap(),
                  (unsigned) ESP.getFreePsram());
  }
}

/* ============================================================
 *                        TEST  010
 *          AUDIO NODE - internet radio over wired ethernet
 *      autoplay on boot. type l for stations, u <url> for any.
 * ============================================================ */