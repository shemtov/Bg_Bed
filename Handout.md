# Adjustable Bed Massage Retrofit — Project Handout

**Revision 9 — 2 August 2026**
Supersedes Rev 8 (1 August). Self-contained: this document alone should let a
future session pick up without re-learning the project.

---

## 1. Goal

A DIY smart bedside system for two adjustable beds shared by Shemtov (Shemi)
and Ira, in Petah Tikva, Israel. One central touch panel on the headboard
controls vibration massage on both beds, plus a clock face with environmental
sensors, voice control, and internet radio through a soundbar.

**Governing rule: zero RF at the heads.** No WiFi, Bluetooth or ESP-NOW near
the sleeping positions. All inter-device communication is wired UART. Only the
audio node has a radio, and it lives away from both heads.

---

## 2. Source control

**Repository:** `github.com/shemtov/Bg_Bed` (public, branch `main`)
**Local working copy:** `C:\projects\Bg_Bed`

```
Bg_Bed/
├── panel/
│   ├── platformio.ini
│   ├── include/
│   │   ├── temp_font.h      screensaver temperature font
│   │   └── dial_image.h     the dial, embedded in flash
│   └── src/main.cpp         TEST 062
├── bedbox/     platformio.ini, src/main.cpp (TEST 007 — NOT the latest)
├── audionode/  platformio.ini, src/main.cpp (TEST 004)
├── .gitignore
└── Handout.md
```

Separate, deliberately outside the repo:
`C:\projects\blinktest\` — a minimal LED test used to prove a board runs at
all. Kept out of `Bg_Bed` because it was accidentally written over the audio
node once already.

**⚠ THE WIFI PASSWORD IS IN A PUBLIC REPO.** `audionode/src/main.cpp` holds
`WIFI_SSID` and `WIFI_PASS` as plain defines. Anyone can read them once
pushed. Change the WiFi password, and consider moving credentials to a file
listed in `.gitignore`.

**Conventions**
- Filenames never carry version numbers. The code file is always `src/main.cpp`.
- `#define TEST_NUMBER n` inside the file. **The test number appears in the
  first three lines AND the last three lines of every file.** Bumped on every
  change, no exceptions.
- Every deliverable is the **whole file**, ready to copy and paste. Never a
  diff, never "change this one line".
- Version numbers go in commit messages, not filenames.
- Never put decorative equal-sign border lines inside code or `.ini` files.
- `.pio/` is never committed.
- Each project folder is opened separately in VS Code — never the repo root.

---

## 3. Panel — WORKING

**Hardware:** Guition ESP32-8048S043 — ESP32-S3-WROOM-1 N16R8, 16MB flash,
8MB octal PSRAM, 4.3" 800×480 IPS parallel RGB (ST7262), GT911 touch.

### The pin budget — read before adding any peripheral

Seven exposed GPIOs total: IO11, IO12, IO13, IO17, IO18, IO19, IO20. Every one
is committed. IO35/36/37 go to the octal PSRAM. The GT911 touch controller is
hard-wired to IO19/IO20 and cannot move. **There is no free pin.**

| Function | Pins / address |
|---|---|
| I2C | SDA = IO19, SCL = IO20 |
| BH1750 ambient light | 0x23 |
| **SHT31 temperature + humidity** | **0x44 — NEW, the temperature source** |
| AT24C32 EEPROM (on the RTC board) | 0x57 — present, unused |
| GT911 touch | 0x5D |
| **DS3231 real-time clock** | **0x68 — NEW, battery-backed** |
| BMP280 temperature + pressure | 0x76 — now used for PRESSURE only |
| INMP441 microphone (I2S0) | DIN=IO11, BCK=IO12, WS=IO13 |
| SD card | **DISABLED** — shares the microphone's pins |
| Backlight | GPIO2, LEDC ch7, 5 kHz, 8-bit |
| UART1 → bed box | TX=IO17, RX=IO18 @ 115200 (P3 header) |

All five I2C devices confirmed responding in the boot scan. No new pins were
needed — that is the point of I2C, and it matters on a board with none free.

**The DS3231 module** is the compact type with a CR1220 already fitted, not the
ZS-042 that tries to charge a non-rechargeable cell. No modification needed.
It carries an AT24C32 EEPROM at 0x57, which is harmless and could be useful
storage later.

**The SHT31** is written with raw `Wire` calls in the same style as the BMP280
driver, so no new library was added.

**Mount the SHT31 away from the display board.** The panel electronics
self-heat a degree or two; a sensor glued behind the screen reads the panel,
not the room.

### platformio.ini — do not change casually

`platform = espressif32 @ 6.9.0` is mandatory (unpinned gives Arduino core
3.2.0, which removed `ledcSetup`, and GFX 1.3.8 will not build against ESP-IDF
5.4). Both `board_build.*` and `board_upload.*` flash-size lines are mandatory.
`-DLV_CONF_SKIP` means **no `lv_conf.h` anywhere**. `-DLV_MEM_SIZE=65536` is
required or the Files tab crashes at boot. Libraries: GFX 1.3.8, JPEGDEC,
LVGL ^8.3.11.

### Firmware — TEST 062, runs on hardware

Voice recognition works, tuned on measured data:

- **Utterance detection** with an adaptive noise floor and a 250 ms pre-roll
  ring buffer, so the start of the word is not lost.
- **MFCC + DTW** — 12 cepstral coefficients, cepstral mean normalisation,
  dynamic time warping normalised by path length.
- **Three reference slots**, learned from a button on the home screen.
- **Measured separation:** 10 repeats of the wake word scored 5.91–8.36; six
  other words scored 13.02–23.86. No overlap. `WAKE_THRESHOLD` is 10.5.
  Replayed through the decision logic: 10/10 true accepted, 0/6 false.
- A duration gate rejects anything outside 0.5×–1.7× of the learned lengths
  before the DTW even runs.
- The microphone runs in **every** state, so the wake word works while the
  screensaver is on, and a detection wakes the screen.

**In practice it is intermittent** — it recognises sometimes and not others,
notably in the morning. That is DTW behaving as designed: templates are
speaker- and condition-specific. See §7 for the plan.

Screensaver is flicker-free: the dial is decoded once into PSRAM, a 424×424
region is composed off-screen with the hands drawn into it, and blitted once.
PSRAM total ≈1.3 MB of 8 MB.

**New in TEST 062**

- **Humidity at last.** The panel never showed it because the BMP280 does not
  measure humidity. The SHT31 now supplies temperature and humidity; the
  BMP280 is kept for pressure and its trend.
- **The pressure arrow now works.** It never appeared, and that was a design
  fault rather than a bug: `pressTrend` only changed inside a block gated on
  30 minutes of uptime, so every reboot reset it to a dash and half an hour
  was needed before it could move at all — and it then wanted a 0.3 hPa swing,
  which is a real weather change. Now the first reference is taken on the
  first reading, the window is 10 minutes, the threshold 0.1 hPa, and the
  **pressure value itself is displayed**, so the reading says something even
  when the trend is steady.
- **The clock survives power cuts.** The DS3231 code was already written in
  TEST 061 in anticipation; the chip simply had not arrived. `time HH:MM` now
  writes to the RTC as well as the running clock.
- Two new serial commands: `sensors` prints every reading and which chips
  answered, `trend` forces a pressure recalculation instead of waiting.

---

## 4. Bed box — UNCHANGED, STILL THE OLDEST OPEN ITEM

**Hardware:** QuinLED-ESP32 (WROOM-32E), 8× 12V ERM motors via 4× L298N.
Motor PWM pins 13,14,18,19,21,22,23,25. Four zones × 2 motors: Head,
Shoulders, Back, Legs. `MIN_DUTY_BIG = 165`, `MIN_DUTY_SMALL = 60`. PWM 20 kHz
8-bit, with a 60 ms full-duty kick-start. UART2 RX=GPIO16 TX=GPIO17 @115200,
crossed to the panel, ground shared, **5V never connected**.

**Protocol, 4 bytes:** `{bedId, cmd, target, value}` — bedId 0 both / 1 Shemi /
2 Ira; cmd 0 OFF, 1 ALL, 2 ZONE, 3 MOTOR, 4 PRESET, 5 TIMER.

**What is in the repo is not the latest.** `bedbox/src/main.cpp` is TEST 007, a
standalone motor demo that does not listen to the panel — which is why massage
taps still do nothing. **TEST 0014**, the UART receiver, was written 22 July
and never committed. It is in the chat "Bed door project",
`claude.ai/chat/c5d478a6-4aec-4178-89f9-9e7918bb4c8c`. Chat attachments expire.
If lost, rebuilding is ~20 lines on top of TEST 007's existing
`handleMessage()`.

---

## 5. Audio node — firmware proven, hardware is a dead end

### Design intent

WT32-ETH01 (wired Ethernet, no radio near the heads) + PCM5102A I2S DAC →
3.5mm → analog-to-TOSLINK converter → optical → Klipsch FLEXUS CORE 100.
The WT32-ETH01 and the TOSLINK converter are both still in transit.

### What was actually built

Bring-up was attempted on a spare **ESP32-CAM (AI-Thinker)** with its USB
motherboard, because it was the only spare board on hand.

```
PCM5102A        ESP32-CAM
VIN     ....... 5V
GND     ....... GND
SCK     ....... GND      <-- MUST be grounded, or silence
BCK     ....... GPIO14
LCK     ....... GPIO2    (was 15 — a strapping pin)
DIN     ....... GPIO13
```

**TEST 004** — internet radio with a web control page. Stations, volume,
now-playing, plus channel diagnostics (I2S format toggle, forced mono,
balance). Web page exists because the board only runs on battery, where there
is no serial console.

### It worked, briefly, and that is the important part

On battery power the full chain ran:

```
connect to: "ice1.somafm.com" on port 80
Connection has been established in 921 ms
MP3Decoder has been initialized
Channels: 2   SampleRate: 44100   BitsPerSample: 16
now playing  Fascinating Earthbound Objects - Charm
```

Sound came out of the headphones. **The firmware, the library, the streaming
and the DAC are all proven.**

### Why the ESP32-CAM is being abandoned

1. **It browns out.** With WiFi and I2S both running the supply collapses —
   on PC USB the device drops off the bus mid-stream. The DAC's power LED goes
   dark at the same instant, confirming the whole rail sags, and the board
   crash-loops inside I2S init.
2. **Every free pin is compromised.** Only 10 GPIOs are exposed and all are
   strapping pins or SD pins.
3. **Serial fights the monitor** — see §9.
4. **Sound came out of one channel only**, and reseating LCK did not fix it.
   TEST 004 has the tools to chase it (format/mono/balance) but the board
   never stayed up long enough to work through them.

**Conclusion: stop. Move to the WT32-ETH01 when it arrives.** The same code
transfers; only the network initialisation changes.

### Station URLs — found and recorded

Extracted from the `data-player-hls-src` attribute in kan.org.il page source
(view-source, Ctrl+F for `m3u8` — the URL is not visible in the rendered page):

| Station | ID | Colour | Stream |
|---|---|---|---|
| Kan 88 | 4504 | `#8c24ff` | `https://kancdn.medonecdn.net/livehls/oil/kancdn-live/live/radio/kan_88/live.livx/playlist.m3u8` |
| Kan Gimel | 4490 | `#ff931e` | `https://kancdn.medonecdn.net/livehls/oil/kancdn-live/live/radio/kan_gimel/live.livx/playlist.m3u8` |

Other Kan stations follow the same pattern with a different name segment.
Reshet Bet is 4483, Kol HaMusica 4518. These are HLS; ESP32-audioI2S handles
`.m3u8` natively.

### audionode/platformio.ini — three non-obvious lines

```ini
platform = espressif32                     ; unpinned ON PURPOSE, see below
board = esp32cam
monitor_rts = 0                            ; or the monitor kills the board
monitor_dtr = 0
lib_deps =
  https://github.com/schreibfaul1/ESP32-audioI2S.git#3.4.0
```

- **Unpinned platform**: the panel's `@ 6.9.0` pin exists for GFX Library and
  does not apply here. ESP32-audioI2S 3.x wants Arduino core 3.x.
- **Library pinned to 3.4.0**: unpinned fetches 3.4.7, which calls
  `dsps_biquad_sf32()` from a newer esp-dsp than the platform ships and fails
  to build. Versions 3.1.0–3.4.0 do not call it.
- **3.4.0 uses the OLD callback style** — free functions `audio_info()`,
  `audio_showstation()`, `audio_showstreamtitle()` — not the `Audio::msg_t`
  struct of 3.4.7. Pinning the library without matching the callbacks costs a
  build cycle.
- The library **requires PSRAM** and a multi-core chip.

---

## 6. Voice recognition — the plan

DTW template matching works but is speaker- and condition-dependent, which is
why recognition is intermittent. Switching the wake word to **English** opens
the door to pre-trained models that Hebrew never had.

**Decision: ESP-SR on a dedicated ESP32-S3.**

- **English MultiNet does not exist for the classic ESP32** — the table in the
  esp-sr repository has an empty cell. It requires ESP32-S3 (or P4). This rules
  out the WT32-ETH01 for speech.
- Free pre-trained English wake words include Computer, Jarvis, Sophia,
  Astrolabe, Hey Wand, Hi Joy, Mycroft, Hey Willow. Trained on thousands of
  speakers, so they work for Shemi, Ira **and a guest** — which DTW cannot.
- **MultiNet supports up to 300 English commands** with custom commands added
  without retraining. That is the real goal: "volume up", "stop", "shoulders
  harder", spoken naturally.
- ESP-SR is an **ESP-IDF component**, not Arduino. That is the learning curve,
  and it is accepted deliberately.

**Hardware to buy: ESP32-S3-DevKitC-1 N16R8** (16MB flash, 8MB PSRAM), ~$8–12
on AliExpress. Must be N16R8 — N8R2 has only 2MB PSRAM, N4/N8 have none.
Plus a second INMP441.

**Steps:** 1 blank IDF project printing hello → 2 I2S mic under IDF →
3 wake word only → 4 MultiNet commands → 5 UART to the panel → 6 real
vocabulary → 7 give the panel its SD card back.

Boards already ruled out: the ESP32-CAM (soldered to a camera in another
project, and unsuitable anyway) and the spare Waveshare ESP32-S3-Touch-LCD-4.3
(its headers are dedicated RS-485/CAN/I2C/AD ports, not free GPIO — a 4.3"
RGB panel consumes almost every pin).

---

## 7. Next steps

**Immediate**
1. **Commit and push everything.** Panel TEST 061, `temp_font.h`,
   `dial_image.h`, audionode TEST 004, this handout. Several near-misses this
   session came from files existing in only one place.
2. **Change the WiFi password** and keep credentials out of the public repo.
3. Recover bed box TEST 0014 before the attachment expires.
4. Order the ESP32-S3-DevKitC-1 N16R8.

**When hardware arrives**
5. Audio node onto the WT32-ETH01 — port TEST 004, change only the network
   setup, then chase the one-channel problem on a board that stays up.
6. ESP-SR speech on the S3, steps 1–7 above.

**Soon**
7. Supply `welcome_800x480.jpg` so it can be embedded in flash like the dial.
8. Decide on the SD photos and repo visibility.
9. Second bed box: QuinLED, 4× L298N, 12V 5A PSU.
10. A spare BH1750 arrived with the sensors. It has an ADDR pin: floating or
    low gives 0x23, tied high gives 0x5C. The panel already has one at 0x23,
    so a second would need its address moved. Purpose not yet decided.

**Later**
11. Panel radio UI wired to the audio node over UART (commands 6–9, node
    address 9).
12. Spotify Connect via cspot — ESP-IDF only, Premium required, unofficial.
    Becomes much easier once IDF is familiar from the ESP-SR work.
13. AC control via IR or Switcher Breeze.
14. Hebrew UI text — needs a custom LVGL font.

---

## 8. Hard-won lessons

**New this session**

- **A feature that never fires may be a design fault, not a bug.** The pressure
  arrow was correct code that could not run: its only update path sat behind a
  30-minute gate, and development reboots reset it every time. Check that a
  feature's trigger conditions can actually occur in normal use.
- **A serial monitor can stop an ESP32 dead.** Opening the monitor asserts DTR
  and RTS; on ESP32 boards those lines drive EN and GPIO0 for auto-flashing,
  and a board without the protecting circuit drops straight into the
  bootloader and goes silent. `monitor_rts = 0` and `monitor_dtr = 0` fix it.
  This cost hours and looked exactly like a dead board. **Shemi found it** by
  noticing the LED stopped blinking the moment the monitor connected.
- **Prove the board runs before debugging anything else.** A blink test with no
  libraries, no WiFi and no serial answers "is this board alive" in one minute.
- **`Serial.flush()` after every line during bring-up.** A board that resets
  mid-print truncates the log exactly where the useful information was.
- **Brownout looks like a code bug.** `rst:0x1 (POWERON_RESET)` with no panic
  message, at the moment a peripheral starts, is a power problem — not a
  crash. A peripheral's own power LED going dark at the same instant confirms
  it.
- **Pin a library version AND check its API.** 3.4.0 and 3.4.7 of the same
  library use different callback styles.
- **Windows renumbers COM ports on replug.** A silent monitor may simply be
  listening on the wrong one. Check the `Terminal on COMxx` line.
- **The stream URL is often not in the rendered page.** View-source and search
  for `m3u8` found what DevTools filtering did not.
- **Keep scratch projects outside the repo.** A blink test written into
  `audionode/src/main.cpp` destroyed the audio node firmware.

**Standing**

- Known-good beats deduced. Read the working file rather than reasoning from
  the code.
- Chat attachments expire. Anything delivered in a chat gets committed the
  same day.
- Never let a failure be silent. Print the reason, and put it on screen if you
  can.
- A black screen with no serial output means the failure is before `setup()`.
- `board_build.*` and `board_upload.*` are different things and both matter.
- LVGL's default heap is 32 KB under `LV_CONF_SKIP`; build lists on demand.
- `GFXglyph` offsets are `int8_t` — reference a large font from the top of the
  digits, not the baseline.
- JPEGDEC decodes **baseline** JPEG only; progressive files fail silently.
- Full-screen repaints flicker on an RGB panel. Compose off-screen, blit once.
- The INMP441 is a 24-bit mic in 32-bit slots; asking for 16-bit gives near
  silence that looks like a wiring fault.
- Keep project paths free of Hebrew characters.

---

*End of handout — Revision 9.*
