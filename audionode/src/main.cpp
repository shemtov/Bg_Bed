/* ============================================================
 *                        TEST  011
 * ============================================================
 *
 *  AUDIO NODE  -  internet radio + phone control
 *
 *  Board : ESP32-S3-WROOM-1  N16R8   (16MB flash, 8MB psram, confirmed)
 *  Net   : W5500 over SPI            (confirmed, TEST 006)
 *  Out   : PCM5102A I2S DAC          (confirmed, TEST 009 tone)
 *  Radio : plays through the soundbar (confirmed, TEST 010)
 *
 *  WHAT IS NEW SINCE TEST 010
 *    The node serves a web page. Open its address in any browser
 *    on the same network - phone, tablet, PC:
 *
 *        http://192.168.1.195
 *
 *    From there: pick a station, set the volume, stop, or paste
 *    any stream url. The phone only sends a short command - the
 *    music comes from the internet straight into this wired node.
 *    Put the phone down, walk out of the room, music keeps playing.
 *
 *    Also new: presets. Anything you paste can be saved into one
 *    of 6 preset slots, kept in flash, surviving a power cut.
 *    That is how the four wanted stations - whiskey blues,
 *    waterfalls, meditation, pink floyd radio - get captured once
 *    you find urls that work.
 *
 *  SERIAL COMMANDS still all work
 *    l   list      i  info      s  stop      +  -  volume
 *    0..9  play by number       u <url>  play any url
 *
 *  NOT TESTED ON HARDWARE. Build to confirm.
 * ============================================================ */

#define TEST_NUMBER 11

#include <Arduino.h>
#include <SPI.h>
#include <ETH.h>
#include <WebServer.h>
#include <Preferences.h>
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
#define STATUS_EVERY_MS   30000
#define VOLUME_START      12
#define VOLUME_MAX        21

#define AUTOPLAY_STATION  0
#define AUTOPLAY_DELAY_MS 3000

#define PRESET_COUNT      6

/* ---------- built in stations ---------- */

struct Station {
  const char *name;
  const char *url;
};

static const Station STATIONS[] = {
  { "Galgalatz",      "http://glzwizzlv.bynetcdn.com/glglz_mp3" },
  { "Kan 88FM",       "https://playerservices.streamtheworld.com/api/livestream-redirect/KAN_88.mp3" },
  { "Groove Salad",   "https://ice5.somafm.com/groovesalad-128-mp3" },
  { "Drone Zone",     "https://ice5.somafm.com/dronezone-128-mp3" },
  { "Deep Space One", "https://ice5.somafm.com/deepspaceone-128-mp3" },
  { "Lush",           "https://ice5.somafm.com/lush-128-mp3" },
};

static const int STATION_COUNT = sizeof(STATIONS) / sizeof(STATIONS[0]);

/* ---------- state ---------- */

Audio       audio;
WebServer   server(80);
Preferences prefs;

static bool   g_linkUp       = false;
static bool   g_haveAddress  = false;
static int    g_volume       = VOLUME_START;
static int    g_station      = -1;
static String g_nowName      = "nothing";
static String g_nowTitle     = "";
static unsigned long g_lastStatus = 0;
static unsigned long g_bootMillis = 0;
static unsigned long g_autoplayAt = 0;
static bool   g_autoplayDone = false;
static String g_line;

static String g_presetName[PRESET_COUNT];
static String g_presetUrl[PRESET_COUNT];
static String g_lastUrl;

/* ---------- presets in flash ---------- */

static void loadPresets()
{
  prefs.begin("audionode", true);
  for (int i = 0; i < PRESET_COUNT; i++) {
    g_presetName[i] = prefs.getString(("pn" + String(i)).c_str(), "");
    g_presetUrl[i]  = prefs.getString(("pu" + String(i)).c_str(), "");
  }
  prefs.end();

  int used = 0;
  for (int i = 0; i < PRESET_COUNT; i++) if (g_presetUrl[i].length()) used++;
  Serial.printf("presets .... %d of %d slots in use\n", used, PRESET_COUNT);
}

static void savePreset(int slot, const String &name, const String &url)
{
  if (slot < 0 || slot >= PRESET_COUNT) return;

  g_presetName[slot] = name;
  g_presetUrl[slot]  = url;

  prefs.begin("audionode", false);
  prefs.putString(("pn" + String(slot)).c_str(), name);
  prefs.putString(("pu" + String(slot)).c_str(), url);
  prefs.end();

  Serial.printf("presets .... slot %d saved: %s\n", slot, name.c_str());
}

/* ---------- playing ---------- */

static void playUrlNamed(const String &url, const String &name)
{
  if (!g_haveAddress) {
    Serial.println(F("play ....... no network address yet"));
    return;
  }

  g_lastUrl  = url;
  g_nowName  = name;
  g_nowTitle = "";

  Serial.println();
  Serial.printf("play ....... %s\n", name.c_str());
  Serial.printf("url ........ %s\n", url.c_str());

  if (!audio.connecttohost(url.c_str())) {
    Serial.println(F("play ....... connecttohost returned FALSE"));
    g_nowName = "failed to open";
  }
}

static void playStation(int index)
{
  if (index < 0 || index >= STATION_COUNT) {
    Serial.println(F("cmd ........ no such station"));
    return;
  }
  g_station = index;
  playUrlNamed(STATIONS[index].url, STATIONS[index].name);
}

static void stopPlaying()
{
  audio.stopSong();
  g_station  = -1;
  g_nowName  = "stopped";
  g_nowTitle = "";
  Serial.println(F("cmd ........ stopped"));
}

static void setVolume(int v)
{
  if (v < 0) v = 0;
  if (v > VOLUME_MAX) v = VOLUME_MAX;
  g_volume = v;
  audio.setVolume(g_volume);
  Serial.printf("vol ........ %d of %d\n", g_volume, VOLUME_MAX);
}

/* ---------- the web page ---------- */

static String htmlEscape(const String &in)
{
  String out;
  for (unsigned i = 0; i < in.length(); i++) {
    char c = in.charAt(i);
    if      (c == '&')  out += "&amp;";
    else if (c == '<')  out += "&lt;";
    else if (c == '>')  out += "&gt;";
    else if (c == '"')  out += "&quot;";
    else                out += c;
  }
  return out;
}

static String pageHead()
{
  String h;
  h += F("<!DOCTYPE html><html><head><meta charset='utf-8'>");
  h += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  h += F("<title>Bedroom radio</title><style>");
  h += F("body{background:#14161f;color:#e8e8ea;font-family:system-ui,-apple-system,"
         "Segoe UI,Roboto,sans-serif;margin:0;padding:16px;}"
         "h1{font-size:20px;margin:0 0 4px 0;font-weight:600;}"
         "h2{font-size:14px;margin:24px 0 8px 0;color:#9a9aa2;font-weight:500;"
         "text-transform:uppercase;letter-spacing:.5px;}"
         ".now{background:#1b1e2a;border-radius:12px;padding:14px;margin:12px 0;}"
         ".now .n{font-size:18px;font-weight:600;}"
         ".now .t{font-size:14px;color:#9a9aa2;margin-top:4px;min-height:18px;}"
         "a.btn,button{display:block;width:100%;box-sizing:border-box;"
         "background:#242838;color:#e8e8ea;border:0;border-radius:12px;"
         "padding:16px;margin:8px 0;font-size:17px;text-align:left;"
         "text-decoration:none;cursor:pointer;}"
         "a.btn:active,button:active{background:#2f3448;}"
         ".row{display:flex;gap:8px;}"
         ".row a.btn,.row button{text-align:center;}"
         ".stop{background:#5a2230;}"
         ".vol{background:#1b1e2a;border-radius:12px;padding:8px 14px 14px;}"
         "input[type=text]{width:100%;box-sizing:border-box;background:#242838;"
         "color:#e8e8ea;border:0;border-radius:12px;padding:14px;font-size:16px;}"
         "select{width:100%;box-sizing:border-box;background:#242838;color:#e8e8ea;"
         "border:0;border-radius:12px;padding:14px;font-size:16px;margin:8px 0;}"
         ".dim{color:#6e6e78;font-size:13px;}"
         "</style></head><body>");
  return h;
}

static void handleRoot()
{
  String p = pageHead();

  p += F("<h1>Bedroom radio</h1>");
  p += F("<div class='dim'>audio node &middot; TEST ");
  p += String(TEST_NUMBER);
  p += F("</div>");

  p += F("<div class='now'><div class='n'>");
  p += htmlEscape(g_nowName);
  p += F("</div><div class='t'>");
  p += htmlEscape(g_nowTitle);
  p += F("</div></div>");

  p += F("<div class='row'>");
  p += F("<a class='btn' href='/vol?d=-1'>&minus; quieter</a>");
  p += F("<a class='btn' href='/vol?d=1'>louder &plus;</a>");
  p += F("</div>");
  p += F("<div class='vol dim'>volume ");
  p += String(g_volume);
  p += F(" of ");
  p += String(VOLUME_MAX);
  p += F("</div>");

  p += F("<a class='btn stop' href='/stop'>Stop</a>");

  p += F("<h2>Stations</h2>");
  for (int i = 0; i < STATION_COUNT; i++) {
    p += F("<a class='btn' href='/play?s=");
    p += String(i);
    p += F("'>");
    p += htmlEscape(STATIONS[i].name);
    p += F("</a>");
  }

  bool anyPreset = false;
  for (int i = 0; i < PRESET_COUNT; i++) if (g_presetUrl[i].length()) anyPreset = true;

  if (anyPreset) {
    p += F("<h2>Your presets</h2>");
    for (int i = 0; i < PRESET_COUNT; i++) {
      if (!g_presetUrl[i].length()) continue;
      p += F("<a class='btn' href='/play?p=");
      p += String(i);
      p += F("'>");
      p += htmlEscape(g_presetName[i]);
      p += F("</a>");
    }
  }

  p += F("<h2>Play any url</h2>");
  p += F("<form action='/url' method='get'>");
  p += F("<input type='text' name='u' placeholder='http://...' autocomplete='off'>");
  p += F("<button type='submit'>Play it</button></form>");

  p += F("<h2>Save the last url as a preset</h2>");
  p += F("<div class='dim' style='margin-bottom:8px'>");
  if (g_lastUrl.length()) p += htmlEscape(g_lastUrl);
  else                    p += F("nothing played yet");
  p += F("</div>");
  p += F("<form action='/save' method='get'>");
  p += F("<input type='text' name='n' placeholder='name it, e.g. Whiskey Blues' autocomplete='off'>");
  p += F("<select name='slot'>");
  for (int i = 0; i < PRESET_COUNT; i++) {
    p += F("<option value='");
    p += String(i);
    p += F("'>slot ");
    p += String(i + 1);
    if (g_presetName[i].length()) {
      p += F(" - now: ");
      p += htmlEscape(g_presetName[i]);
    } else {
      p += F(" - empty");
    }
    p += F("</option>");
  }
  p += F("</select>");
  p += F("<button type='submit'>Save</button></form>");

  p += F("<h2 class='dim'>Find more</h2>");
  p += F("<div class='dim'>Search a station directory in another tab, "
         "copy the stream url, paste it above, then save it here.</div>");

  p += F("<div style='height:40px'></div></body></html>");

  server.send(200, "text/html", p);
}

static void redirectHome()
{
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

static void handlePlay()
{
  if (server.hasArg("s")) {
    playStation(server.arg("s").toInt());
  } else if (server.hasArg("p")) {
    int slot = server.arg("p").toInt();
    if (slot >= 0 && slot < PRESET_COUNT && g_presetUrl[slot].length()) {
      g_station = -1;
      playUrlNamed(g_presetUrl[slot], g_presetName[slot]);
    }
  }
  redirectHome();
}

static void handleUrl()
{
  String u = server.arg("u");
  u.trim();
  if (u.length()) {
    g_station = -1;
    playUrlNamed(u, "custom url");
  }
  redirectHome();
}

static void handleSave()
{
  if (!g_lastUrl.length()) { redirectHome(); return; }

  int slot = server.arg("slot").toInt();
  String name = server.arg("n");
  name.trim();
  if (!name.length()) name = "preset " + String(slot + 1);

  savePreset(slot, name, g_lastUrl);
  redirectHome();
}

static void handleVol()
{
  setVolume(g_volume + server.arg("d").toInt());
  redirectHome();
}

static void handleStop()
{
  stopPlaying();
  redirectHome();
}

static void handleNotFound()
{
  server.send(404, "text/plain", "not here");
}

static void startWebServer()
{
  server.on("/",      handleRoot);
  server.on("/play",  handlePlay);
  server.on("/url",   handleUrl);
  server.on("/save",  handleSave);
  server.on("/vol",   handleVol);
  server.on("/stop",  handleStop);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println();
  Serial.println(F("------------------------------------------------------------"));
  Serial.print  (F("  OPEN THIS ON YOUR PHONE:   http://"));
  Serial.println(ETH.localIP());
  Serial.println(F("  same wifi as this router. leave the phone anywhere."));
  Serial.println(F("------------------------------------------------------------"));
  Serial.println();
}

/* ---------- serial commands ---------- */

static void listStations()
{
  Serial.println();
  Serial.println(F("stations:"));
  for (int i = 0; i < STATION_COUNT; i++) {
    Serial.printf("   %d  %s\n", i, STATIONS[i].name);
  }
  for (int i = 0; i < PRESET_COUNT; i++) {
    if (g_presetUrl[i].length()) {
      Serial.printf("   preset %d  %s\n", i + 1, g_presetName[i].c_str());
    }
  }
  Serial.println();
}

static void printInfo()
{
  Serial.println();
  Serial.printf("link ....... %s\n", g_linkUp ? "UP" : "DOWN");
  Serial.print (F("address .... "));
  Serial.println(g_haveAddress ? ETH.localIP().toString() : String("none"));
  Serial.printf("volume ..... %d of %d\n", g_volume, VOLUME_MAX);
  Serial.printf("now ........ %s\n", g_nowName.c_str());
  Serial.printf("running .... %s\n", audio.isRunning() ? "yes" : "no");
  Serial.printf("heap ....... %u free\n", (unsigned) ESP.getFreeHeap());
  Serial.printf("psram ...... %u free\n", (unsigned) ESP.getFreePsram());
  Serial.println();
}

static void handleLine(String line)
{
  line.trim();
  if (line.length() == 0) return;

  char c = line.charAt(0);

  if (c == 'l' || c == 'L') { listStations(); return; }
  if (c == 'i' || c == 'I') { printInfo();    return; }
  if (c == '+')             { setVolume(g_volume + 1); return; }
  if (c == '-')             { setVolume(g_volume - 1); return; }
  if (c == 's' || c == 'S') { stopPlaying();  return; }

  if (c == 'u' || c == 'U') {
    int sp = line.indexOf(' ');
    if (sp < 0) {
      Serial.println(F("cmd ........ usage: u http://something"));
      return;
    }
    g_station = -1;
    playUrlNamed(line.substring(sp + 1), "custom url");
    return;
  }

  if (c >= '0' && c <= '9') { playStation(line.toInt()); return; }

  Serial.println(F("cmd ........ unknown. try l, i, s, +, -, a number, or u <url>"));
}

static void pollSerial()
{
  while (Serial.available()) {
    char c = (char) Serial.read();
    if (c == '\n' || c == '\r') {
      if (g_line.length()) { handleLine(g_line); g_line = ""; }
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
      Serial.print  (F("  IP ADDRESS   "));  Serial.println(ETH.localIP());
      Serial.print  (F("  GATEWAY      "));  Serial.println(ETH.gatewayIP());
      Serial.print  (F("  MAC          "));  Serial.println(ETH.macAddress());
      startWebServer();
      listStations();
      if (AUTOPLAY_STATION >= 0 && !g_autoplayDone) {
        g_autoplayAt = millis() + AUTOPLAY_DELAY_MS;
        Serial.printf("autoplay ... station %d in %d ms\n",
                      AUTOPLAY_STATION, AUTOPLAY_DELAY_MS);
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

/* ---------- audio library callbacks ---------- */

void audio_info(const char *info)
{
  Serial.printf("audio ...... %s\n", info);
}

void audio_showstation(const char *info)
{
  Serial.printf("  STATION    %s\n", info);
  if (info && *info) g_nowName = String(info);
}

void audio_showstreamtitle(const char *info)
{
  Serial.printf("  NOW PLAYING  %s\n", info);
  g_nowTitle = String(info ? info : "");
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

  Serial.println();
  Serial.println(F("============================================================"));
  Serial.printf ("  AUDIO NODE   -   TEST %03d   -   radio + phone control\n", TEST_NUMBER);
  Serial.println(F("============================================================"));

  esp_reset_reason_t r = esp_reset_reason();
  Serial.printf("reset ...... %s\n",
                r == ESP_RST_POWERON  ? "power on" :
                r == ESP_RST_BROWNOUT ? "BROWNOUT - power dip" :
                r == ESP_RST_PANIC    ? "PANIC - previous run crashed" :
                r == ESP_RST_SW       ? "software restart" : "other");

  Serial.printf("flash ...... %u bytes\n", (unsigned) ESP.getFlashChipSize());
  Serial.printf("psram ...... %u bytes\n", (unsigned) ESP.getPsramSize());
  Serial.println(F("i2s pins ... bclk 14  lrc 2  dout 13"));

  loadPresets();
  Serial.println();

  Network.onEvent(onNetEvent);

  pinMode(PIN_ETH_RST, OUTPUT);
  digitalWrite(PIN_ETH_RST, LOW);
  delay(20);
  digitalWrite(PIN_ETH_RST, HIGH);
  delay(100);

  SPI.begin(PIN_ETH_SCK, PIN_ETH_MISO, PIN_ETH_MOSI);

  Serial.println(F("eth ........ calling begin"));

  if (!ETH.begin(ETH_PHY_W5500, ETH_PHY_ADDRESS,
                 PIN_ETH_CS, PIN_ETH_INT, PIN_ETH_RST,
                 SPI, ETH_SPI_MHZ)) {
    Serial.println(F("  !!! ETH.begin RETURNED FALSE - wiring or power !!!"));
  }

  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.setVolume(g_volume);
  Serial.printf("i2s ........ ready, volume %d of %d\n", g_volume, VOLUME_MAX);
  Serial.println();
}

/* ---------- loop ---------- */

void loop()
{
  audio.loop();
  pollSerial();

  if (g_haveAddress) server.handleClient();

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
 *                        TEST  011
 *        AUDIO NODE - radio over ethernet + web page control
 *   open http://<node address> on your phone. presets in flash.
 * ============================================================ */
