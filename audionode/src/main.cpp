/* ============================================================
 *                        TEST  004
 * ============================================================
 *  AUDIO NODE - internet radio, with a web control page
 *
 *  TEST_NUMBER 4 - printed at boot on serial.
 *
 *  WHAT CHANGED IN 004
 *    1. A WEB PAGE. Open the board's IP in any browser on the
 *       same network - stations, volume, now playing, all from
 *       a phone. This matters because the board browns out on
 *       PC USB and only runs properly on the battery pack,
 *       where there is no serial console at all.
 *       Flash on USB, unplug, run on battery, control by web.
 *
 *    2. TOOLS FOR THE RIGHT-CHANNEL-ONLY PROBLEM. Sound came
 *       out of one side only and reseating LCK did not help.
 *       Three things to try, all from the web page or serial:
 *         fmt      toggles I2S_COMM_FORMAT LSB/MSB. A format
 *                  mismatch between the ESP32 and the DAC is
 *                  the usual cause of one dead channel.
 *         mono     forces stereo down to mono - if BOTH sides
 *                  then play, the wiring is fine and it is a
 *                  channel-mapping problem, not a dead wire.
 *         bal N    balance, -16 left .. 0 centre .. 16 right.
 *
 *    3. Volume defaults to 21 of 21. The PCM5102A puts out
 *       LINE level, meant to feed an amplifier, so headphones
 *       plugged straight in will always sound quiet. That is
 *       expected and is not a fault.
 *
 *  POWER - the hard-won lesson
 *    On a PC USB port this board browns out once WiFi and I2S
 *    run together: the USB device drops off the bus mid-stream
 *    and it crash-loops. On a battery pack or phone charger it
 *    runs and plays. Flash on USB, listen on battery.
 *
 *  WIFI CREDENTIALS - IMPORTANT
 *    This repository is PUBLIC. Anything committed here can be
 *    read by anyone. Consider putting the real values in a
 *    separate file that git ignores, or change the password
 *    once bring-up is done. They are left here as defines for
 *    now because bring-up needs them, but do not forget.
 *
 *  HARDWARE
 *      PCM5102A        ESP32-CAM
 *      --------        ---------
 *      VIN     ....... 5V
 *      GND     ....... GND
 *      SCK     ....... GND    <-- must be grounded
 *      BCK     ....... GPIO14
 *      LCK     ....... GPIO2
 *      DIN     ....... GPIO13
 *
 *  SERIAL AND WEB COMMANDS
 *    list          show the station table
 *    play N        play station N
 *    url <URL>     play any stream URL
 *    vol N         volume 0..21
 *    stop          stop playback
 *    info          state, IP, memory
 *    fmt           toggle the I2S comm format  (channel fix)
 *    mono          toggle forced mono          (channel test)
 *    bal N         balance -16..16             (channel test)
 *
 *  NOT TESTED ON HARDWARE.
 * ============================================================ */

#define TEST_NUMBER 4

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Audio.h"

// ---- fill these in ----
#define WIFI_SSID   "YOUR_WIFI_NAME"
#define WIFI_PASS   "YOUR_WIFI_PASSWORD"

// ---- I2S pins to the PCM5102A ----
#define I2S_BCLK    14
#define I2S_LRC      2
#define I2S_DOUT    13

#define DEFAULT_VOLUME 21          // line level into headphones is quiet

Audio audio;
WebServer server(80);

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

// TEST 004: channel diagnostics
bool  fmtLsb   = false;    // I2S comm format
bool  monoOn   = false;    // forced mono
int   balance  = 0;        // -16 left .. 16 right

// ------------------------------------------------------------
//  Library callbacks - 3.4.0 style, plain functions
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
  if (ok) { curStation = n; playStartMs = millis(); }
  else Serial.println("connecttohost refused the URL");
  return ok;
}

bool playUrl(const char *url) {
  nowTitle = "";
  nowStation = "";
  Serial.printf("connecting to %s\n", url);
  bool ok = audio.connecttohost(url);
  if (ok) { curStation = -1; playStartMs = millis(); }
  else Serial.println("connecttohost refused the URL");
  return ok;
}

void applyFormat() {
  audio.setI2SCommFMT_LSB(fmtLsb);
  Serial.printf("i2s ........ comm format is now %s\n", fmtLsb ? "LSB" : "MSB (default)");
}

void applyMono() {
  audio.forceMono(monoOn);
  Serial.printf("audio ...... forced mono %s\n", monoOn ? "ON" : "off");
}

void applyBalance() {
  audio.setBalance((int8_t)balance);
  Serial.printf("audio ...... balance %d  (-16 left, 0 centre, 16 right)\n", balance);
}

void showInfo() {
  Serial.printf("TEST %03d  audio node\n", TEST_NUMBER);
  Serial.printf("  wifi ...... %s",
                WiFi.status() == WL_CONNECTED ? "connected" : "NOT connected");
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("  ip %s  rssi %d dBm",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  Serial.println();
  Serial.printf("  i2s ....... BCK=%d LCK=%d DIN=%d  fmt=%s  mono=%s  bal=%d\n",
                I2S_BCLK, I2S_LRC, I2S_DOUT,
                fmtLsb ? "LSB" : "MSB", monoOn ? "on" : "off", balance);
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

// ------------------------------------------------------------
//  TEST 004: the web page
// ------------------------------------------------------------
String htmlPage() {
  String h;
  h.reserve(4000);
  h += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h += "<title>Audio Node</title><style>";
  h += "body{background:#14161f;color:#e8ecf4;font-family:system-ui,sans-serif;";
  h += "margin:0;padding:18px;} h1{font-size:20px;margin:0 0 4px;}";
  h += ".sub{color:#7b90a0;font-size:13px;margin-bottom:18px;}";
  h += ".card{background:#1b1e2a;border-radius:12px;padding:14px;margin-bottom:14px;}";
  h += "a.btn,button{display:inline-block;background:#2a3346;color:#e8ecf4;";
  h += "border:0;border-radius:10px;padding:12px 16px;margin:4px 4px 4px 0;";
  h += "font-size:15px;text-decoration:none;cursor:pointer;}";
  h += "a.on{background:#2e6f4e;} .stop{background:#8a3a3a;}";
  h += ".np{font-size:16px;color:#8fe0a0;min-height:22px;}";
  h += "input[type=range]{width:100%;}";
  h += "</style></head><body>";

  h += "<h1>Audio Node</h1><div class='sub'>TEST ";
  h += String(TEST_NUMBER);
  h += " &middot; ";
  h += WiFi.localIP().toString();
  h += "</div>";

  // now playing
  h += "<div class='card'><div class='sub'>Now playing</div><div class='np'>";
  if (audio.isRunning()) {
    h += nowTitle.length() ? nowTitle : String("(no title)");
    h += "<br><span class='sub'>";
    h += nowStation.length() ? nowStation : String("");
    h += "</span>";
  } else {
    h += "stopped";
  }
  h += "</div></div>";

  // stations
  h += "<div class='card'><div class='sub'>Stations</div>";
  for (int i = 0; i < NUM_STATIONS; i++) {
    h += "<a class='btn";
    if (i == curStation) h += " on";
    h += "' href='/play?n=";
    h += String(i);
    h += "'>";
    h += stations[i].name;
    h += "</a>";
  }
  h += "<br><a class='btn stop' href='/stop'>Stop</a></div>";

  // volume
  h += "<div class='card'><div class='sub'>Volume ";
  h += String(curVolume);
  h += " of 21</div>";
  h += "<form action='/vol' method='get'>";
  h += "<input type='range' name='v' min='0' max='21' value='";
  h += String(curVolume);
  h += "' onchange='this.form.submit()'></form></div>";

  // channel diagnostics
  h += "<div class='card'><div class='sub'>Channel tests</div>";
  h += "<a class='btn";
  if (fmtLsb) h += " on";
  h += "' href='/fmt'>Format: ";
  h += (fmtLsb ? "LSB" : "MSB");
  h += "</a>";
  h += "<a class='btn";
  if (monoOn) h += " on";
  h += "' href='/mono'>Mono: ";
  h += (monoOn ? "ON" : "off");
  h += "</a>";
  h += "<br><div class='sub'>Balance ";
  h += String(balance);
  h += " (-16 left, 0 centre, 16 right)</div>";
  h += "<form action='/bal' method='get'>";
  h += "<input type='range' name='b' min='-16' max='16' value='";
  h += String(balance);
  h += "' onchange='this.form.submit()'></form></div>";

  h += "</body></html>";
  return h;
}

void handleRoot()  { server.send(200, "text/html", htmlPage()); }
void redirectHome(){ server.sendHeader("Location", "/"); server.send(303); }

void handlePlay() {
  if (server.hasArg("n")) playStation(server.arg("n").toInt());
  redirectHome();
}

void handleStop() {
  audio.stopSong();
  curStation = -1;
  redirectHome();
}

void handleVol() {
  if (server.hasArg("v")) {
    int v = server.arg("v").toInt();
    if (v >= 0 && v <= 21) { curVolume = v; audio.setVolume(v); }
  }
  redirectHome();
}

void handleFmt()  { fmtLsb = !fmtLsb; applyFormat(); redirectHome(); }
void handleMono() { monoOn = !monoOn; applyMono();   redirectHome(); }

void handleBal() {
  if (server.hasArg("b")) {
    int b = server.arg("b").toInt();
    if (b >= -16 && b <= 16) { balance = b; applyBalance(); }
  }
  redirectHome();
}

// ------------------------------------------------------------

void pollSerial() {
  while (Serial.available()) {
    char c = Serial.read();
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
      else if (conBuf == "fmt")  { fmtLsb = !fmtLsb; applyFormat(); }
      else if (conBuf == "mono") { monoOn = !monoOn; applyMono(); }
      else if (conBuf.startsWith("bal ")) {
        int b = conBuf.substring(4).toInt();
        if (b >= -16 && b <= 16) { balance = b; applyBalance(); }
        else Serial.println("usage: bal -16..16");
      }
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
      else Serial.printf("unknown command '%s' - try: list, play N, url <URL>, "
                         "vol N, stop, info, fmt, mono, bal N\n",
                         conBuf.c_str());
      conBuf = "";
    } else conBuf += c;
  }
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.printf("\n=== AUDIO NODE - TEST %03d - radio + web ===\n", TEST_NUMBER);
  Serial.flush();

  if (!psramFound())
    Serial.println("WARNING: no PSRAM found. ESP32-audioI2S needs it.");
  else
    Serial.printf("psram ...... %u bytes\n", (unsigned)ESP.getPsramSize());
  Serial.flush();

  Serial.printf("i2s ........ about to set pins BCK=%d LCK=%d DIN=%d\n",
                I2S_BCLK, I2S_LRC, I2S_DOUT);
  Serial.flush();
  delay(50);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  Serial.println("i2s ........ setPinout survived");
  Serial.flush();

  audio.setVolume(curVolume);
  applyFormat();
  applyMono();
  applyBalance();
  Serial.flush();

  Serial.printf("wifi ....... connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("wifi ....... ok, ip %s, rssi %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    server.on("/",     handleRoot);
    server.on("/play", handlePlay);
    server.on("/stop", handleStop);
    server.on("/vol",  handleVol);
    server.on("/fmt",  handleFmt);
    server.on("/mono", handleMono);
    server.on("/bal",  handleBal);
    server.begin();
    Serial.printf("web ........ open http://%s in a browser\n",
                  WiFi.localIP().toString().c_str());
  } else {
    Serial.println("wifi ....... FAILED - check WIFI_SSID and WIFI_PASS");
  }
  Serial.flush();

  listStations();
  if (WiFi.status() == WL_CONNECTED) playStation(0);
}

void loop() {
  audio.loop();
  server.handleClient();
  pollSerial();
  vTaskDelay(1);
}

/* ============================================================
 *  Audio node - stage 1 of the audio chain.
 *
 *  TRYING TO FIX THE ONE-SIDED SOUND
 *    Open the web page and work down this list, listening
 *    after each one:
 *      1. Press "Format: MSB" so it reads LSB. An I2S format
 *         mismatch is the most common cause of a dead channel.
 *      2. Press "Mono: off" so it reads ON. If BOTH sides now
 *         play, nothing is broken in the wiring - the data is
 *         simply landing in one channel slot.
 *      3. Slide Balance fully left. If the LEFT side is silent
 *         at every setting, that channel truly is not driven.
 *
 *  Next steps, in order:
 *    005  UART link to the panel - RADIO_PLAY / STOP / VOL /
 *         SLEEP (commands 6-9, node address 9)
 *    006  move to the WT32-ETH01 and wired Ethernet
 *    007+ Spotify Connect via cspot, under ESP-IDF
 *
 *  Kan brand colours, for the panel's radio tiles later:
 *    Kan 88     #8c24ff   Kan Gimel  #ff931e
 * ============================================================
 *                  TEST  004   (end of file)
 * ============================================================ */