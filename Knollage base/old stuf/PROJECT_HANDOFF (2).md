# PROJECT HANDOFF — Adjustable Bed Massage Retrofit
Paste this document at the start of any new chat to continue the project
without re-explaining. Last updated at: TEST 1 (architecture rev 3 —
FULLY WIRED, RF-FREE PANELS. Rev-3 code not yet written; Test 2 will be
built on this foundation).

## 1. What the project is
A DIY massage system for TWO adjacent beds (user שם טוב + wife Ira),
each bed with its OWN touchscreen panel on the headboard (two identical
panels — Ira requested per-side screens instead of one shared middle
panel). The panels also front a SHARED internet radio playing through
stereo speakers on the two bedside tables, show a big clock with
temperature/humidity when idle, and are planned to gain AC infrared
control later. Each panel defaults to controlling its own bed.

- Per bed: 10 small vibration motors (ERM type, eccentric weight, ONE
  direction only) embedded in the mattress. Motors are 12V (originally 24V,
  changed to 12V), ~0.2–0.5A each.
- Driver: L298N modules, 2 motors per module, 5 modules per bed.
  (DRV8833 was rejected: its 10.8V max VM is below 12V.)
- L298N wiring rule: 12V into VS, KEEP the onboard 5V-regulator jumper
  (12V input is OK), hardwire IN1/IN3 HIGH and IN2/IN4 LOW (single
  direction), only ENA/ENB run to the controller = 10 PWM lines per bed,
  20 kHz PWM (silent).
- Power: one 12V 6–7A (~80W) supply per bed + a master switch per bed.
  A 12V→5V buck (2–3A) feeds each bed's ESP32; a separate 12V→5V 3A buck
  feeds the panel. COMMON GROUND everywhere per power domain.

## 2. Control architecture (final)
- Each bed has a control box: 1 classic ESP32 (WROOM-32) + 5x L298N.
  Firmware: `main-board/` project. BED_ID 1 and BED_ID 2.
- The headboard panel is a Waveshare ESP32-S3-Touch-LCD-4.3 (user already
  BOUGHT this exact board — plain 4.3, NOT the 4.3B/4.3C): 800x480 RGB IPS,
  GT911 capacitive touch, ESP32-S3-WROOM-1 N16R8 (16MB flash, 8MB PSRAM),
  mains-powered (no battery in final design).
- Link: ESP-NOW broadcast, 4-byte messages, bedId filtering. No pairing.
  Protocol: {bedId, cmd, target, value}
  cmd: 0 OFF | 1 ALL(value=0..100) | 2 ZONE(target=0..4) |
       3 MOTOR(target=0..9) | 4 PRESET(target: 0 Wave,1 Pulse,2 Ripple) |
       5 TIMER(value=minutes 5..120)
- CRITICAL RULE: when the panel's WiFi is enabled, ESP-NOW rides the
  router's channel — both bed boxes' RF_CHANNEL must be set to the router's
  channel (panel prints it on serial after connecting). With WiFi off, all
  use ESPNOW_FIXED_CHANNEL (default 1).
- Bed boxes remember last massage + timer in flash, RESUME it instantly on
  power-on (user switches bed power on -> massage starts, no screen needed),
  and auto-stop motors after the session timer (setting is kept).
- Bed selection on the panel: taps on Bed 1/Bed 2 pills, AND (planned)
  sensing the two beds' LED-strip light buttons — pressing a bed's light
  button flips the panel to that bed ("last press wins"). Lights are
  12/24V LED strips; sense via voltage divider or dual-pole switch second
  contact into panel GPIOs 15/16.

## 2b. REV 3 — FULLY WIRED, RF-FREE PANELS (current architecture)
CORE DESIGN RULE: the user is highly sensitive to EMF/RF near the head.
The panels at the two headboards must emit NO radio, ever. All
inter-device communication is WIRED. ESP-NOW IS DROPPED ENTIRELY.

- TWO identical Waveshare S3 4.3 panels, one per bedside (second unit to
  buy). PANEL_ID 1/2, each defaults to its own bed. WiFi and BT stay OFF
  in panel firmware at all times (WIFI_OFF at boot).
- WIRED LINKS (UART 115200, 3 wires TX/RX/GND, twisted/shielded pair
  riding the existing DC cable runs):
    Panel1 <-> Panel2            (headboard run, ~2 m: messages, alarms,
                                  radio commands + now-playing sync)
    Panel1 <-> Bed box 1         (massage commands down the power cable)
    Panel2 <-> Bed box 2
    Cross-bed control hops: Panel1 -> Panel2 -> Bed box 2.
  The 4-byte command protocol {bedId, cmd, target, value} survives, now
  framed over serial instead of ESP-NOW.
- AUDIO/INTERNET MODULE: the only WiFi device, placed AWAY from both
  heads (foot of bed / inside a bed box). A third ESP32(-S3) that does
  WiFi + radio streaming + PCM5102A + TPA3116 + the two speakers, and
  takes commands over a UART wire from the panels. Neither panel has
  any active radio at any time. NTP time reaches the panels over the
  wire from this module.
- Bed boxes: unchanged internally (classic ESP32 + 5x L298N), but their
  radio is also removed — they listen on UART from their panel instead
  of ESP-NOW. Memory/resume/session-timer logic unchanged.
- NIGHT QUIET MODE: when a panel's screen goes dark (stillness), that
  panel does nothing but watch its PIR (purely passive sensor) and its
  UART line. With rev 3 there is no RF to silence — night quiet is now
  mainly about screen-dark + delivering stored messages on PIR wake.

## 2c. MESSAGES & ALARMS (agreed feature, planned ~Test 4)
- "Note on the pillow": compose on one panel -> delivered over the wire
  to the other -> stored; shown in big letters when the recipient's PIR
  sees movement (or immediately if awake). "Got it" button acknowledges;
  sender sees delivered/acknowledged status. Messages persist in flash.
- Preset message tiles cover most use (user+Ira to supply real list;
  Hebrew OK in presets from day one). Free-text via LVGL keyboard in
  English first; full Hebrew keyboard/RTL is a later dedicated test.
- Alarms: set on own panel or on the other person's ("wake Ira at 7").
  Alarm = clock face + radio ramping up gently via the audio module.
  Big Snooze/Off buttons.
- PIR cannot distinguish wake-up from walk-past: accepted — message
  shows on any motion, dismissible.

## 2d. PER-PANEL SENSORS & SCREEN STATE MACHINE (unchanged from rev 2)
- Each panel has its own: LDR (Sensor header GPIO6), PIR HC-SR501
  (motion, one GPIO), AHT20 or SHT31 temp+humidity on the panel I2C bus
  (mount away from the warm display board - short cable to enclosure
  edge).
- State machine: power-on WELCOME PHOTO (user+Ira; placeholder until
  photo provided) -> HOME screen (tiles: Massage / Radio / AC-coming-
  soon) -> idle ~2 min -> CLOCK FACE (huge digits + temp + humidity,
  brightness follows ambient light) -> no PIR motion ~10 min -> SCREEN
  FULLY DARK -> PIR motion -> clock lights ~60 s; TOUCH -> home. All
  timeouts are config.h constants.
- AC control: DEFERRED stage. Home screen reserves an AC tile; will be
  an IR LED + IRremoteESP8266 capture session (AC brand not yet known).

## 3. Decision history (why things are the way they are)
- Started as phone web app (ESP32-hosted page) -> user rejected phone at
  bedside (RF near head at night) -> battery touchscreen remote (CYD 2.8")
  -> Arduino Mega considered and rejected (ESP8266 sidecar complexity) ->
  ESP-NOW chosen over WiFi joins for remote (instant, low power) ->
  Guition JC4880P443C rejected (ESP32-P4+C6, immature ESP-NOW, MIPI-DSI) ->
  Waveshare S3 4.3 chosen and PURCHASED -> battery + latching button plans
  superseded by FIXED mains-powered headboard panel (light-button idea led
  to this) -> "auto-off" moved to the BED side as the session timer.
- Audio: user wanted YouTube — impossible on ESP32 (explained, accepted).
  Settled on internet radio. True stereo required: PCM5102A I2S DAC ->
  TPA3116 (or TPA3110) class-D amp (runs on the existing 12V) -> passive
  hi-fi speakers on the two bedside tables. (MAX98357A pair was the
  alternative for raw small drivers; PCM5102A+amp chosen for real
  speakers.) 3W is enough for bedside volume but the TPA3116 gives clean
  headroom.
- Stations: Radio Browser directory (all.api.radio-browser.info) — presets
  (tap), genre browse tiles (tap-through, no keyboard), on-panel keyboard
  only as last resort, plus (planned) a panel-served web page for
  comfortable searching from a PC. Genres with assigned colors:
  Greek #2c58a8, Classical #7a5bb5, Rock #b5483a, Blues #1d7a94,
  Jazz #b07818, Israeli #2e8a5c, News #5f6673, Chill #a04f7e.
  Presets inherit their genre's color (left edge stripe).
- Brightness: board has a Sensor header on GPIO6 (ADC) — LDR plugs in
  there. CH422G IO expander controls backlight ON/OFF ONLY (EXIO2 also
  gates DISP — PWM-ing it kills the display). True dimming needs a
  one-wire mod to the MP3302 backlight chip control point (~1 kHz PWM to
  avoid whine); until then firmware dims CONTENT via a black LVGL overlay.
  Time-of-day (NTP) caps brightness at night (21:00–07:00 Israel time).

## 4. UI design (mockups approved by user)
Dark theme (bg #14161f, cards #1b1e2a), big touch targets, LVGL 8.4.
- Top: Bed 1 / Bed 2 pills (left), tabs Massage | Radio.
- TEST NUMBER badge ALWAYS visible top-right (e.g. "T1").
- Massage tab: mode row [All | Zones | Motors | Presets]; content area
  changes per mode (one big slider / 5 zone sliders / 10 motor sliders /
  3 preset cards); bottom row: timer slider 5–60 min (5-min steps,
  TIMER_MAX 120 possible) + red STOP always in the corner.
  Zones: Shoulders, Upper back, Lower back, Thighs, Calves (2 motors each,
  motors 0-1, 2-3, 4-5, 6-7, 8-9).
- Radio tab: now-playing bar + Stop, 2x4 colored genre grid, preset row
  (genre-colored edges, long-press a browse result to save), volume slider
  (0..21), "Sleep 30" fade-out button.

## 5. Code (all C++ / VS Code / PlatformIO)
Projects delivered in bed_system_test1.zip:
- `panel/` — the headboard panel. pioarduino platform (Arduino core 3.x,
  needed for RGB panel), qio_opi PSRAM, 16MB flash. Libraries: lvgl@^8.4,
  GFX Library for Arduino (Arduino_ESP32RGBPanel — pin map verified from
  community config, in display.cpp), bb_captouch (GT911),
  ESP32-audioI2S (radio), ArduinoJson.
  Files: include/config.h (ALL switches/pins/credentials — the control
  panel), include/lv_conf.h, src/{main,display,espnow_link,massage_ui,
  radio,comfort}.cpp/.h.
- `main-board/` — per-bed classic ESP32 (stock espressif32 platform,
  2.x-compatible ledcSetup/ledcAttachPin API). Motor pins:
  {13,14,18,19,21,22,23,25,26,27} = ENA/ENB of the 5 modules in order.
- `remote/` — legacy Arduino_GFX battery-remote project, superseded by
  panel/ but kept for reference.
Panel peripheral pins (config.h): I2S BCLK 19 / LRCK 20 / DOUT 11;
LDR GPIO6; light buttons GPIO15/16; touch I2C SDA 8 / SCL 9;
backlight-mod PWM pin suggestion GPIO4.

Feature switches in config.h (1/0):
ENABLE_MASSAGE_UI, ENABLE_ESPNOW, ENABLE_RADIO, ENABLE_WIFI,
ENABLE_LIGHT_BUTTONS, ENABLE_AUTOBRIGHT, ENABLE_BACKLIGHT_PWM.
Disabled modules compile but only serial-print what they WOULD do.
Bring-up stages: 1) MASSAGE+ESPNOW, 2) +WIFI+RADIO (fix bed channels!),
3) +LIGHT_BUTTONS+AUTOBRIGHT(+BACKLIGHT_PWM after mod).

## 6. WORKING CONVENTIONS (user's standing rules — follow strictly)
1. TEST_NUMBER: `#define TEST_NUMBER n` is the FIRST line of code in
   panel/include/config.h and main-board/src/main.cpp. It is bumped on
   EVERY change, no exceptions. Shown top-right on screen (LVGL top
   layer) and printed first on serial at boot on both boards.
2. Every change: present the modified files individually (so the user can
   open/copy/download each) AND a fresh zip named bed_system_testN.zip.
3. The user codes in VS Code + PlatformIO, wants C++ (.cpp), and wants to
   learn the code — explain changes, keep modules small and readable.
4. Be honest about untested code: nothing has run on real hardware yet.
   Known first-boot tuning points: PSRAM setting (qio_opi) or blank
   screen; touch possibly mirrored (fix in display.cpp touchCb); library
   version drift.

## 7. Current status & open items
- STATUS: Test 1. Hardware not yet arrived (screen in transit). Nothing
  compiled/flashed on real hardware. User has: L298N modules, 12V motors,
  an Arduino Mega+WiFi (unused), and the purchased Waveshare panel.
- OPEN: (a) speakers not chosen yet (passive pair vs powered — chain
  assumes passive + TPA3116); (b) backlight hardware mod — optional,
  later; (c) user's real radio station favorites for presets; (d) exact
  zone labels/motor placement to confirm to the mattress layout;
  (e) WiFi credentials to be filled in config.h; (f) SECOND Waveshare
  panel + 2x PIR + 2x AHT20/SHT31 to purchase; (g) welcome photo of
  user+Ira to be provided and converted; (h) AC brand for the IR stage;
  (i) which side is the audio master.
- TEST 2 (next code work, not yet written): panel state machine
  (welcome/home/clock/dark/PIR wake), clock face with big digits +
  temp/humidity, sensors module (PIR + AHT20), PANEL_ID and own-bed
  default, audio master/client radio command protocol between panels.
- NEXT STEPS agreed: user studies Test 1 code, tries a build of
  main-board/ in PlatformIO; on any compile error, paste it in chat.
  When the screen arrives: Stage-1 bring-up (massage + ESP-NOW only).

## 8. User context (helpful to know)
- Comfortable with electronics and soldering; hobbyist programmer keen to
  learn; communicates by voice dictation (transcripts sometimes garbled —
  ask when unclear, e.g. "Waveshare" appears as "wavesetter").
- Cares about avoiding RF/screens near the head at night — the reason for
  the fixed panel + mains power + auto-dim design. Located in Israel
  (Israeli radio stations relevant, TZ Israel).
