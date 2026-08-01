/* ============================================================
 *                        TEST  002
 * ============================================================
 *  AUDIO NODE - internet radio
 *
 *  TEST_NUMBER 2 - printed at boot on serial.
 *
 *  WHY 002 AND NOT 001
 *    TEST 001 was written against the callback API of library
 *    version 3.4.7 and would not compile, because the .ini pins
 *    the library to 3.4.0 to dodge an esp-dsp mismatch. 3.4.0
 *    uses FREE FUNCTIONS - audio_info(), audio_showstation(),
 *    audio_showstreamtitle() - not the Audio::msg_t struct.
 *    That is corrected here. Also adds the two Kan stations.
 *
 *  HARDWARE (prototype stage)
 *    ESP32-CAM (AI-Thinker) on its USB motherboard, plus a
 *    PCM5102A I2S DAC. The camera is NOT initialised, which is
 *    what frees the microSD pins for I2S.
 *
 *      PCM5102A        ESP32-CAM
 *      --------        ---------
 *      VIN     ....... 3.3V   (5V also works)
 *      GND     ....... GND
 *      SCK     ....... GND    <-- MUST be grounded. The ESP32
 *                              sends no master clock; the board
 *                              makes its own only if SCK is low.
 *                              Floating SCK = silence or noise.
 *      BCK     ....... GPIO14
 *      LCK     ....... GPIO15
 *      DIN     ....... GPIO13
 *
 *      Headphones or powered speakers into the 3.5mm jack.
 *
 *    Pins avoided and why:
 *      GPIO0     strapping + camera XCLK, breaks uploads
 *      GPIO1/3   UART0, needed for this serial console
 *      GPIO4     the very bright flash LED
 *      GPIO16    PSRAM chip select - the audio library needs PSRAM
 *      GPIO12    strapping, selects flash voltage, must be low at boot
 *
 *  SERIAL COMMANDS (115200)
 *    list          show the station table
 *    play N        play station N from the table
 *    url <URL>     play any stream URL without recompiling
 *    vol N         volume 0..21
 *    stop          stop playback
 *    info          what is playing, WiFi state, free memory
 *
 *  BEFORE YOU BUILD
 *    Fill in WIFI_SSID and WIFI_PASS below.
 *
 *  ON THE STATIONS
 *    Stations 0-2 are SomaFM, publicly documented and stable -
 *    use them to prove the DAC and wiring first.
 *    Stations 3-4 are Kan 88 and Kan Gimel, taken from the
 *    data-player-hls-src attribute in kan.org.il page source.
 *    They are HLS; this library handles .m3u8 natively.
 *    None of these have been played on this hardware yet.
 *
 *  NOT TESTED ON HARDWARE.
 * ============================================================ */

#define TEST_NUMBER 2

#include <Arduino.h>
#include <WiFi.h>
#include "Audio.h"

// ---- fill these in ----
#define WIFI_SSID   "YOUR_WIFI_NAME"
#define WIFI_PASS   "YOUR_WIFI_PASSWORD"

// ---- I2S pins to the PCM5102A ----
#define I2S_BCLK    14
#define I2S_LRC     15
#define I2S_DOUT    13

#define DEFAULT_VOLUME 12          // 0..21, matches the panel's slider

Audio audio;

struct Station {
  const char *name;
  const char *url;
};

const Station stations[] = {
  { "SomaFM Groove Salad", "http://ice1.somafm.com/groovesalad-128-mp3" },
  { "SomaFM Drone Zone",   "http://ice1.somafm.com/dronezone-128-mp3"   },
  { "SomaFM Deep Space",   "http://ice1.somafm.com/deepspaceone-128-mp3"},
  { "Kan 88",
    "https://kancdn.medonecdn.net/livehls/oil/kancdn-live/live/radio/kan_88/live.livx/playlist.m3u8" },
  { "Kan Gimel",
    "https://kancdn.medonecdn.net/livehls/oil/kancdn-live/live/radio/kan_gimel/live.livx/playlist.m3u8" },
};
const int NUM_STATIONS = sizeof(stations) / sizeof(stations[0]);

int      curStation  = -1;
int      curVolume   = DEFAULT_VOLUME;
String   conBuf      = "";
String   nowTitle    = "";
String   nowStation  = "";
uint32_t playStartMs = 0;

// ------------------------------------------------------------
//  Library callbacks - version 3.4.0 style. These are plain
//  functions with these exact names; the library calls them if
//  they exist. Do not wrap them in a class.
// ------------------------------------------------------------
void audio_info(const char *info) {
  Serial.printf("info ....... %s\n", info);
}

void audio_showstation(const char *info) {
  nowStation = info;
  Serial.printf("station .... %s\n", info);
}

void audio_showstreamtitle(const char *info) {
  nowTitle = info;
  Serial.printf("now playing  %s\n", info);
}

void audio_bitrate(const char *info) {
  Serial.printf("bitrate .... %s\n", info);
}

void audio_lasthost(const char *info) {
  Serial.printf("last host .. %s\n", info);
}

void audio_eof_mp3(const char *info) {
  Serial.printf("end of file  %s\n", info);
}

// ------------------------------------------------------------

void listStations() {
  Serial.println("stations:");
  for (int i = 0; i < NUM_STATIONS; i++) {
    Serial.printf("  %d %c %s\n", i, (i == curStation) ? '*' : ' ',
                  stations[i].name);
  }
  Serial.println("  (use 'url <address>' to try one that is not listed)");
}

bool playStation(int n) {
  if (n < 0 || n >= NUM_STATIONS) {
    Serial.printf("no station %d - there are %d\n", n, NUM_STATIONS);
    return false;
  }
  nowTitle = "";
  nowStation = "";
  Serial.printf("connecting to %s\n", stations[n].name);
  bool ok = audio.connecttohost(stations[n].url);
  if (ok) {
    curStation = n;
    playStartMs = millis();
  } else {
    Serial.println("connecttohost refused the URL");
  }
  return ok;
}

bool playUrl(const char *url) {
  nowTitle = "";
  nowStation = "";
  Serial.printf("connecting to %s\n", url);
  bool ok = audio.connecttohost(url);
  if (ok) {
    curStation = -1;
    playStartMs = millis();
  } else {
    Serial.println("connecttohost refused the URL");
  }
  return ok;
}

void showInfo() {
  Serial.printf("TEST %03d  audio node\n", TEST_NUMBER);
  Serial.printf("  wifi ...... %s",
                WiFi.status() == WL_CONNECTED ? "connected" : "NOT connected");
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("  ip %s  rssi %d dBm",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  Serial.println();
  Serial.printf("  station ... %s\n",
                curStation >= 0 ? stations[curStation].name :
                (audio.isRunning() ? "(direct URL)" : "none"));
  if (nowStation.length()) Serial.printf("  icy name .. %s\n", nowStation.c_str());
  if (nowTitle.length())   Serial.printf("  title ..... %s\n", nowTitle.c_str());
  Serial.printf("  playing ... %s", audio.isRunning() ? "yes" : "no");
  if (audio.isRunning()) Serial.printf("  for %lu s", (millis() - playStartMs) / 1000);
  Serial.println();
  Serial.printf("  volume .... %d of 21\n", curVolume);
  Serial.printf("  heap ...... %u free, psram %u free\n",
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
}

void pollSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    // Accept CR or LF. Only accepting '\n' is how the panel spent a
    // whole session ignoring every command that was typed.
    if (c == '\n' || c == '\r') {
      conBuf.trim();
      if (conBuf.length() == 0) { /* bare terminator */ }
      else if (conBuf == "list") listStations();
      else if (conBuf == "stop") {
        audio.stopSong();
        curStation = -1;
        Serial.println("stopped");
      }
      else if (conBuf == "info") showInfo();
      else if (conBuf.startsWith("play ")) {
        playStation(conBuf.substring(5).toInt());
      }
      else if (conBuf.startsWith("url ")) {
        String u = conBuf.substring(4);
        u.trim();
        if (u.length() > 7) playUrl(u.c_str());
        else Serial.println("usage: url http://...");
      }
      else if (conBuf.startsWith("vol ")) {
        int v = conBuf.substring(4).toInt();
        if (v >= 0 && v <= 21) {
          curVolume = v;
          audio.setVolume(v);
          Serial.printf("volume %d\n", v);
        } else Serial.println("usage: vol 0..21");
      }
      else Serial.printf("unknown command '%s' - try: "
                         "list, play N, url <URL>, vol N, stop, info\n",
                         conBuf.c_str());
      conBuf = "";
    } else conBuf += c;
  }
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.printf("\n=== AUDIO NODE - TEST %03d - internet radio ===\n", TEST_NUMBER);

  // The library requires PSRAM. Say so plainly rather than failing oddly.
  if (!psramFound()) {
    Serial.println("WARNING: no PSRAM found. ESP32-audioI2S needs it; expect trouble.");
  } else {
    Serial.printf("psram ...... %u bytes\n", (unsigned)ESP.getPsramSize());
  }

  Serial.printf("wifi ....... connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("wifi ....... ok, ip %s, rssi %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  else
    Serial.println("wifi ....... FAILED - check WIFI_SSID and WIFI_PASS "
                   "at the top of this file");

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(curVolume);
  Serial.printf("i2s ........ BCK=%d LCK=%d DIN=%d, volume %d\n",
                I2S_BCLK, I2S_LRC, I2S_DOUT, curVolume);
  Serial.println("REMINDER: the DAC's SCK pin must be wired to GND.");

  listStations();
  Serial.println("type 'play 0' to start, or 'url http://...' to try any stream");

  if (WiFi.status() == WL_CONNECTED) playStation(0);
}

void loop() {
  audio.loop();
  pollSerial();
  vTaskDelay(1);
}

/* ============================================================
 *                        TEST  002   (end of file)
 * ============================================================
 *  Audio node - stage 1 of the audio chain.
 *
 *  Next steps, in order:
 *    003  UART link to the panel - accept RADIO_PLAY / STOP /
 *         VOL / SLEEP (commands 6-9, node address 9) and send
 *         the now-playing text back
 *    004  move to the WT32-ETH01 and wired Ethernet. Only the
 *         network setup changes; everything above stays.
 *    005+ Spotify Connect via cspot, under ESP-IDF, once the
 *         S3 has taught us that framework.
 *
 *  The DAC output currently goes to headphones. The final chain
 *  is: PCM5102A 3.5mm -> analog-to-TOSLINK converter -> optical
 *  cable -> Klipsch FLEXUS CORE 100.
 *
 *  Kan brand colours, for the panel's radio tiles later:
 *    Kan 88     #8c24ff   Kan Gimel  #ff931e
 * ============================================================ */
