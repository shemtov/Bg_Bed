# Adjustable Bed Massage Retrofit — Project Handout

**Revision 10 — 5 August 2026**
Supersedes Rev 9 (2 August). Self-contained: this document alone should let a
future session pick up without re-learning the project.

---

## 1. Goal

A DIY smart bedside system for two adjustable beds shared by Shemtov (Shemi)
and Ira, in Petah Tikva, Israel. One central touch panel on the headboard
controls vibration massage on both beds, plus a clock face with environmental
sensors, voice control, internet radio, air conditioning, lighting and shades.

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
│   ├── include/          FIVE headers, all required
│   │   ├── dial_image.h    the screensaver dial
│   │   ├── font_hebrew.h   Hebrew + Latin glyphs, and the Hebrew strings
│   │   ├── lamp_img.h      the Morning Glory lamp, 200x200 with alpha
│   │   ├── temp_font.h     the screensaver temperature font
│   │   └── tile_img.h      bed, radio, AC and shutter tiles, 200x200
│   └── src/main.cpp      TEST 074
├── bedbox/     platformio.ini, src/main.cpp (TEST 007 — NOT the latest)
├── audionode/  platformio.ini, src/main.cpp (TEST 004)
├── .gitignore
└── Handout.md
```

Outside the repo on purpose:
`C:\projects\blinktest\` — a minimal LED test that proves a board runs at all.
Kept separate because it once overwrote the audio node firmware.

**⚠ THE WIFI PASSWORD IS IN A PUBLIC REPO.** `audionode/src/main.cpp` holds
`WIFI_SSID` and `WIFI_PASS` as plain defines. Change the WiFi password, and
move credentials to a file listed in `.gitignore`.

**Conventions**
- The code file is always `src/main.cpp`. Filenames never carry versions.
- **The test number appears in the FIRST three lines and the LAST three lines
  of every file**, and in `#define TEST_NUMBER n`. Bumped on every change.
- Every deliverable is the **whole file**, ready to copy and paste. Never a
  diff, never "change this one line".
- Version numbers go in commit messages, not filenames.
- Never put decorative equal-sign border lines inside code or `.ini` files.
- `.pio/` is never committed.
- Each project folder is opened separately in VS Code — never the repo root.
- **Headers go in `include/`, not the project root.** Only `include/` is on the
  compiler's search path. A header dropped beside `platformio.ini` is invisible
  and the old copy keeps being used — this has happened.

---

## 3. Panel — the main work of this session

**Hardware:** Guition ESP32-8048S043 — ESP32-S3-WROOM-1 N16R8, 16MB flash,
8MB octal PSRAM, 4.3" 800×480 IPS parallel RGB (ST7262), GT911 touch.

### The pin budget

Only seven GPIOs are exposed: IO11, IO12, IO13, IO17, IO18, IO19, IO20. All are
committed. IO35/36/37 go to the octal PSRAM. GT911 touch is hard-wired to
IO19/IO20 and cannot move. **There is no free pin.**

| Function | Pins / address |
|---|---|
| I2C | SDA = IO19, SCL = IO20 |
| BH1750 light | 0x23 |
| SHT31 temp + humidity | 0x44 — the temperature source |
| AT24C32 EEPROM (on the RTC board) | 0x57 — present, unused |
| GT911 touch | 0x5D |
| DS3231 real-time clock | 0x68 — battery-backed |
| BMP280 | 0x76 — pressure only |
| INMP441 microphone (I2S0) | DIN=IO11, BCK=IO12, WS=IO13 |
| SD card | **DISABLED** — shares the microphone's pins |
| Backlight | GPIO2, LEDC ch7 |
| UART1 → bed box | TX=IO17, RX=IO18 @ 115200 |

### platformio.ini — do not change casually

`platform = espressif32 @ 6.9.0` is mandatory. Both `board_build.*` and
`board_upload.*` flash-size lines are mandatory. `-DLV_CONF_SKIP` means **no
`lv_conf.h` anywhere**. `-DLV_MEM_SIZE=65536` is required or the Files tab
crashes at boot. Libraries: GFX 1.3.8, JPEGDEC, LVGL ^8.3.11.

### Firmware history, TEST 062 → 074

| Test | Change |
|---|---|
| 062 | SHT31 humidity added; pressure trend arrow fixed |
| 063 | AC screen and Room screen; fourth home tile |
| 064 | Photographic tiles, Hebrew font, AC short/long press |
| 065 | Six fixes: About humidity, settings text, **jitter cause found** |
| 066 | Screensaver dimmed to 75%, smaller temperature, light blue |
| 067 | **Home screen rebuilt: 3 × 2 photographic tiles, no words** |
| 068 | Six soft tile colours; shade animation removed; BG BEDROOM |
| 069 | **Radio screen built**; screensaver 56%, grey temperature |
| 070 | Title in Hebrew |
| 071 | **Lamp animation removed entirely**; AC dimmed to 75% |
| 072 | Header version guards with self-explaining errors |
| 073 | **Settings screens grey**, text inverted to suit |
| 074 | Room temperature moved right of the + on the AC screen |

### The home screen as it stands

Six 200×200 photographic tiles, three across and two down, **no text in any
language**. Grid: 78 px left margin, 22 px between columns, 16 px between rows,
64 px of status strip.

| Tile | Behaviour |
|---|---|
| **Bed** | opens the massage screen |
| **Radio** | opens the radio screen; dial lights when playing |
| **AC** | SHORT press toggles and the louvre opens with air; LONG press (3 s) opens the AC screen |
| **Lamp** | toggles. Grey when off, warm amber when lit. **No animation** |
| **Shutters** | press to send the command; the picture flips at once |
| **Settings** | opens settings |

Title reads **BG חדר שינה**. Status line is **time then temperature**, nothing
else — pressure, trend arrow, humidity and the degree letter were all removed
from the home screen. They are still measured and still on the About tab.

### The tile images — how they were made

All from Shemi's own photographs, processed in Python:

- **Lamp** — Morning Glory by Ayala Serfaty, Aqua Creations. Stored once with
  an alpha channel and recoloured by the firmware, so the two states cannot
  drift apart.
- **Shutters** — one photograph of an open shuttered window. The closed state
  was made by sampling the shutters' own plank texture and tiling it across the
  glass, so the closed shutters are literally the real ones. Flowers masked by
  colour and never painted over.
- **Air conditioner** — the white Tadiran, **shortened to three quarters** by
  splicing a slice out of the plain middle of the body, which keeps both the
  logo and the display. The open louvre is drawn: a dark cavity, blue-lit
  vanes, the flap tilted out, and falling air. Dimmed to 75% because the white
  glared.
- **Radio and bed** — cut from their studio backgrounds by flood fill.

**Flash used by assets: about 890 KB of 16 MB, 5.3%.**

### Voice recognition — working, but intermittent

MFCC + DTW with three reference slots, a 250 ms pre-roll, an adaptive noise
floor and a duration gate. Measured: 10 repeats scored 5.91–8.36, six other
words 13.02–23.86, no overlap. `WAKE_THRESHOLD` 10.5, replayed as 10/10 true
accepts and 0/6 false.

In practice it recognises sometimes and not others, notably in the morning.
That is DTW behaving as designed — templates are speaker- and
condition-specific. **The plan is ESP-SR on a dedicated ESP32-S3.** English
MultiNet does not exist for the classic ESP32; the table in the esp-sr
repository has an empty cell. Free pre-trained English wake words include
Computer, Jarvis, Sophia, Astrolabe, Hey Wand. MultiNet supports up to 300
English commands, so "volume up", "stop", "shoulders harder" become possible —
and they work for Shemi, Ira **and a guest**, which DTW cannot do.

**To buy: ESP32-S3-DevKitC-1 N16R8** (16MB flash, 8MB PSRAM), ~$8–12. Must be
N16R8. ESP-SR is an ESP-IDF component, not Arduino — that learning curve is
accepted deliberately.

---

## 4. Bed box — STILL THE OLDEST OPEN ITEM

QuinLED-ESP32, 8× 12V ERM motors via 4× L298N. Motor PWM pins 13,14,18,19,21,
22,23,25. Four zones × 2 motors. `MIN_DUTY_BIG` 165, `MIN_DUTY_SMALL` 60. PWM
20 kHz, 60 ms kick-start. UART2 RX=16 TX=17, crossed, ground shared, **5V never
connected**.

`bedbox/src/main.cpp` is TEST 007, a motor demo that does not listen to the
panel — which is why massage taps still do nothing. **TEST 0014**, the UART
receiver, was written 22 July and never committed. It is in the chat "Bed door
project", `claude.ai/chat/c5d478a6-4aec-4178-89f9-9e7918bb4c8c`. Attachments
expire.

---

## 5. Audio node — firmware proven, hardware abandoned

Design intent: WT32-ETH01 on wired Ethernet + PCM5102A DAC → 3.5mm →
analog-to-TOSLINK → Klipsch FLEXUS CORE 100.

Bring-up was attempted on a spare **ESP32-CAM**. TEST 004 has internet radio and
a web control page. **It played** — SomaFM connected, MP3 decoded at 44.1 kHz
stereo, sound came out of headphones. The firmware, the library and the DAC are
proven.

**The ESP32-CAM is abandoned** because it browns out with WiFi and I2S running
together, every free pin is a strapping or SD pin, and sound came out of one
channel only. Move to the WT32-ETH01 when it arrives; only the network setup
changes.

Wiring that worked: VIN 5V, GND, **SCK to GND (mandatory)**, BCK GPIO14,
LCK GPIO2, DIN GPIO13.

**Station URLs, extracted from `data-player-hls-src` in kan.org.il page source:**

| Station | ID | Colour | Stream |
|---|---|---|---|
| Kan 88 | 4504 | `#8C24FF` | `https://kancdn.medonecdn.net/livehls/oil/kancdn-live/live/radio/kan_88/live.livx/playlist.m3u8` |
| Kan Gimel | 4490 | `#FF931E` | `https://kancdn.medonecdn.net/livehls/oil/kancdn-live/live/radio/kan_gimel/live.livx/playlist.m3u8` |

Reshet Bet is 4483, Kol HaMusica 4518, same pattern.

**audionode/platformio.ini — three non-obvious lines**

```ini
platform = espressif32                     ; unpinned ON PURPOSE
monitor_rts = 0                            ; or the monitor kills the board
monitor_dtr = 0
lib_deps = https://github.com/schreibfaul1/ESP32-audioI2S.git#3.4.0
```

The library must be pinned to **3.4.0** — unpinned fetches 3.4.7, which calls
`dsps_biquad_sf32()` from a newer esp-dsp and fails to build. And **3.4.0 uses
the OLD callback style**: free functions `audio_info()`, `audio_showstation()`,
`audio_showstreamtitle()`, not the `Audio::msg_t` struct.

---

## 6. Air conditioning — Switcher Breeze, working from Python

**The Tadiran is controlled through a Switcher Breeze**, which sits away from
the beds with line of sight to the unit. Panel → UART → audio node → WiFi →
Breeze. The panel stays radio-silent.

**Discovered device, confirmed working:**

```
device_id     0998b7
device_key    02
ip_address    192.168.1.122
remote_id     YACIBI00      (Tadiran; verified against the irset database)
token_needed  False
```

Two scripts live in `C:\Users\User\` and are not yet in the repo:

- **`switcher_discover.py`** (TEST 001) — listens for the UDP broadcast and
  prints every Switcher device. Written because aioswitcher 6.1.3 ships a
  broken `discover_devices.exe` that imports a module not in the wheel.
- **`breeze_control.py`** (TEST 006) — reads and controls the AC from the
  command line. **This works.** It has read the state and changed it.

**Three things learned the hard way, all recorded in that script:**

1. **A target temperature of 0 is NORMAL.** This Breeze reports 0 no matter
   what it is really set to. Four versions of the script were spent treating
   that as corruption and refusing to send anything.
2. **The Breeze often acts on a command and never replies.** Silence is not
   failure. Every call needs a timeout or the script hangs forever inside
   `reader.read()`.
3. **Swing is on/off only.** The API has no step-swing versus continuous-sweep.

**Consequence for the panel: the AC's target temperature can only ever be shown
as what was last COMMANDED.** Mode, fan, swing and room temperature do read
back. The AC screen says "measured here" under the room temperature for exactly
this reason.

**Still to do:** port the Breeze protocol to C++ for the audio node. The
protocol is CRC-16/HQX signed twice with a peculiar byte-swapped key, and the
IR codes come from a 13.7 MB database — but the entry for `YACIBI00` alone is
55 KB and fits in flash.

---

## 7. Lights and shades — 433 MHz, decided

**The bedroom light and the shades will both use 433 MHz**, not WiFi and not
Zigbee. The reasoning, worked through carefully:

- A **433 MHz transmitter is silent until pressed** — a few hundred
  milliseconds per command, then nothing. A WiFi or Zigbee device stays
  associated and transmits continuously.
- The shutter switch is **half a metre from the pillow**, so a permanently
  transmitting module there was rejected.
- Zigbee is no better than WiFi for this: it is a mesh member and mains-powered
  nodes relay other devices' traffic.

**To buy:** one FS1000A transmitter (~₪5) on the audio node, one 2-channel
433 MHz receiver for the shades in **latching + stop** mode, one 1-channel for
the light in toggle mode. Buy with the keyfob remotes for pairing. The
receivers use the 1527 encoding, which the ESP32 can generate directly with
RCSwitch — so no cloning is needed, we choose the codes.

**Shade logic agreed:** press Up to go, press either button again to stop. The
panel tracks its own idea of movement with a 45-second timeout, because there
is no feedback.

**Existing Shelly 1 Mini Gen4** is in the study, 2.5 m from the desk, not the
bedroom. It has a local HTTP API: `/rpc/Switch.Set?id=0&on=true`.

---

## 8. Protocol — panel to audio node (address 9)

`{bedId, cmd, target, value}`, four bytes, same as the bed boxes.

| cmd | Meaning |
|---|---|
| 0–5 | massage, to the bed boxes |
| 6 | RADIO_PLAY, target = station |
| 7 | RADIO_STOP |
| 8 | RADIO_VOL, value 0–21 |
| 9 | RADIO_SLEEP, value = minutes |
| 10 | AC power |
| 11 | AC temperature |
| 12 | AC mode |
| 13 | AC fan |
| 14 | AC swing |
| 15 | RF — target 0 shade, 1 light |

---

## 9. Open questions, waiting on Shemi

1. **The Hebrew spelling of Ira and Shemi**, exactly as they should appear on
   the massage screen buttons. Not guessed — names matter.
2. **"Both" in Hebrew** — שניהם, or a preference.
3. **The sentence about "set time"** that cut off mid-way.
4. Whether the bed tile's "running" glow should stay warm or become cool, to
   distinguish it from the lamp.

**Massage screen, still to build:** the three selector buttons in Hebrew,
stacked vertically on the right; a header showing the date, the time, and the
temperature with a small "c" beside it.

---

## 10. Next steps

**Immediate**
1. **Commit and push.** Panel TEST 074 and all five headers, the two Switcher
   scripts, this handout.
2. **Change the WiFi password** and keep credentials out of the public repo.
3. Recover bed box TEST 0014 before the attachment expires.
4. Order: ESP32-S3-DevKitC-1 N16R8, FS1000A transmitter, two 433 MHz receivers.

**When hardware arrives**
5. Audio node onto the WT32-ETH01; port TEST 004 across.
6. Port the Switcher Breeze protocol to C++.
7. ESP-SR speech on the S3.
8. 433 MHz transmitter and the two receivers.

**Later**
9. Spotify Connect via cspot — ESP-IDF only, Premium required, unofficial.
   Much easier once IDF is familiar from the ESP-SR work.
10. The RJ45 by the bed is a **telephone socket, not Ethernet** — but it has
    two twisted pairs, which is all 100 Mbit needs. Wire pins 1,2,3,6 keeping
    each pair together, straight through, into a LAN port on the router.
    **Disconnect the phone line first — ringing voltage is about 90 V.**

---

## 11. Hard-won lessons

**New this session**

- **Any repeated redraw of a large image on this RGB panel shows as a twitch
  across the whole screen.** Three attempts were spent slowing an animation
  before removing it. The panel scans continuously and LVGL repaints while it
  does. On a bedside screen, calm beats cleverness.
- **A header in the wrong folder fails silently.** Only `include/` is searched;
  a header beside `platformio.ini` is ignored and the stale copy is used. The
  error names a missing symbol, not the file. TEST 072 added `#error` guards
  that name the file and the version.
- **Flood-fill background removal must be tolerance-tuned per photograph.** The
  Tadiran's white body is (232,232,232) against a (248,249,251) backdrop — only
  16 apart — so a tolerance of 16 floods inside the unit and punches a hole in
  it. It needs 8. And the mask must be taken **before** any dimming, or the
  reference colour moves.
- **A device reporting a nonsense value may be reporting correctly.** The
  Breeze's target temperature of 0 was real behaviour, not corruption. Four
  script versions were spent defending against it.
- **Silence is not failure.** The Breeze acts on commands without replying.
  Every network call needs a timeout.
- **lv_font_conv names the font symbol after the output filename.** Writing to
  `/tmp/heb.c` produced a font called `heb`, which would have failed at link
  time.
- **LVGL's light theme fights you.** With `LV_CONF_SKIP` the tab pages are near
  white. Paint them explicitly rather than choosing text colours that survive.
- **Ask rather than guess at names.** Hebrew name spellings were requested, not
  invented.

**Standing**

- Known-good beats deduced. Read the working file rather than reasoning from
  the code.
- Chat attachments expire. Anything delivered in a chat gets committed the
  same day.
- Never let a failure be silent. Print the reason, and put it on screen.
- A black screen with no serial output means the failure is before `setup()`.
- `board_build.*` and `board_upload.*` are different things and both matter.
- LVGL's default heap is 32 KB under `LV_CONF_SKIP`; build lists on demand.
- `GFXglyph` offsets are `int8_t` — reference a large font from the top of the
  digits, not the baseline. `bitmapOffset` is `uint16_t`, capping a font at
  65535 bytes of bitmap.
- JPEGDEC decodes **baseline** JPEG only; progressive files fail silently.
- Compose off-screen and blit once; a full-screen repaint flickers.
- The INMP441 is a 24-bit mic in 32-bit slots.
- **A serial monitor can stop an ESP32 dead** — `monitor_rts = 0` and
  `monitor_dtr = 0`. Found by Shemi noticing the LED stopped the moment the
  monitor connected.
- Windows renumbers COM ports on replug.
- Keep project paths free of Hebrew characters.

---

*End of handout — Revision 10.*
