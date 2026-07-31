# Adjustable Bed Massage Retrofit — Project Handout

**Revision 7 — 31 July 2026**
Supersedes Rev 6 (same day, earlier). Self-contained: this document alone
should let a future session pick up without re-learning the project.

---

## 1. Goal

A DIY smart bedside system for two adjustable beds shared by Shemtov (Shemi)
and Ira, in Petah Tikva, Israel. One central touch panel on the headboard
controls vibration massage on both beds, plus a clock face with environmental
sensors, and later voice control, internet radio and AC control.

**Governing rule: zero RF at the heads.** No WiFi, Bluetooth or ESP-NOW near
the sleeping positions. All inter-device communication is wired UART. Only a
future audio node will have a radio, and it lives away from both heads.

---

## 2. Source control

**Repository:** `github.com/shemtov/Bg_Bed` (public, branch `main`)
**Local working copy:** `C:\projects\Bg_Bed`

```
Bg_Bed/
├── panel/
│   ├── platformio.ini
│   ├── include/
│   │   ├── temp_font.h      TEST 050+ — screensaver temperature font
│   │   └── dial_image.h     TEST 053+ — the dial, embedded in flash
│   └── src/main.cpp         TEST 055
├── bedbox/     platformio.ini, src/main.cpp (TEST 007 — NOT the latest)
├── audionode/  not started
├── .gitignore
└── Handout.md
```

**Working loop:** edit in VS Code → save → GitHub Desktop → write a summary →
`Commit to main` → `Push origin`.

**Conventions**
- Filenames never carry version numbers. The code file is always
  `src/main.cpp`. The version lives in `#define TEST_NUMBER n` inside the
  file, in the banner top and bottom, and in the boot print.
- Version numbers go in commit messages, not filenames.
- Never put decorative equal-sign border lines inside code or `.ini` files.
- `.pio/` is never committed.
- Each of `panel/`, `bedbox/`, `audionode/` is a separate PlatformIO project.
  Open that folder in VS Code — never the repository root.
- Every firmware deliverable is the whole program as one complete file.

**Loose files still in `panel/`:** `microphone.cpp`, `microphone.h` and a
legacy `config.h`. PlatformIO only compiles `src/`, so none are built. The
microphone pair has now been superseded — its logic was rewritten directly
into `main.cpp` at TEST 054, with four bugs fixed (see §9). They can be
deleted. `config.h` is from the abandoned Waveshare/ESP-NOW design and must
never be `#include`d: it defines `TEST_NUMBER 2`, which would collide.

---

## 3. Panel

**Hardware:** Guition ESP32-8048S043 — ESP32-S3-WROOM-1 N16R8, 16MB flash,
8MB octal PSRAM, 4.3" 800×480 IPS parallel RGB (ST7262), GT911 touch.

### THE PIN BUDGET — read this before adding any peripheral

The board exposes **only seven GPIOs**, across four headers:

| Header | Pins |
|---|---|
| P2 (SPI) | IO11, IO12, IO13, IO19 |
| P3 (USB/UART) | IO17, IO18, IO19, IO20 |
| P4 | IO17, IO18, +3.3V, GND |
| P1 (UART) | UART0 Rx/Tx, +5V, GND |

Every one is committed. IO35/36/37 are consumed by the octal PSRAM. The
GT911 touch controller is hard-wired to IO19/IO20 on the PCB and cannot
move. **There is no free pin.** Any new peripheral must displace something
or live on another board.

| Function | Pins / address |
|---|---|
| I2C bus | SDA = IO19, SCL = IO20 |
| BH1750 ambient light | 0x23 — present |
| GT911 touch | 0x5D — present |
| BMP280 temp + **pressure** | 0x76 — present (no humidity) |
| DS3231 RTC | 0x68 — **absent, not yet bought** |
| **INMP441 microphone (I2S0)** | **DIN=IO11, BCK=IO12, WS=IO13** |
| SD card (SPI) | **DISABLED** — same pins as the microphone |
| Backlight | GPIO2, LEDC channel 7, 5 kHz, 8-bit |
| UART1 → bed box | TX = IO17, RX = IO18 @ 115200, via P3 header |
| USB / serial monitor | UART0, 115200 |

### platformio.ini — do not change casually

```ini
[env:panel]
platform = espressif32 @ 6.9.0
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder

board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_build.psram_type = opi
board_build.flash_size = 16MB
board_build.partitions = default_16MB.csv
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216

build_flags =
  -DBOARD_HAS_PSRAM
  -DARDUINO_USB_CDC_ON_BOOT=0
  -DLV_CONF_SKIP
  -DLV_COLOR_DEPTH=16
  -DLV_TICK_CUSTOM=1
  -DLV_MEM_SIZE=65536
  -DLV_FONT_MONTSERRAT_20=1
  -DLV_FONT_MONTSERRAT_28=1
  -DLV_FONT_MONTSERRAT_40=1

lib_deps =
  moononournation/GFX Library for Arduino @ 1.3.8
  bitbank2/JPEGDEC @ ^1.2.8
  lvgl/lvgl @ ^8.3.11
```

1. **`@ 6.9.0` is mandatory.** Unpinned, `espressif32` resolves to 54.3.20 on
   this machine (Arduino core 3.2.0), which removed `ledcSetup()` and
   `ledcAttachPin()`, and GFX 1.3.8 will not compile against its ESP-IDF 5.4
   headers.
2. **The `board_upload.*` lines are mandatory.** With only the build side set,
   the bootloader believed the chip was the 8MB no-PSRAM N8 variant and reset
   forever — black screen, no serial at all, `rst:0x3 RTC_SW_SYS_RST`.
3. **`LV_CONF_SKIP`** means LVGL is configured entirely by build flags. **No
   `lv_conf.h` is needed anywhere.** The `lv_conf.h.FROM_TEST1_CHECK_T…`
   placeholder in `include/` is unused and can be deleted.
4. **`LV_MEM_SIZE=65536`** added at TEST 047. Without it LVGL defaults to a
   32 KB heap — see §9.

### Panel firmware — TEST 055

Runs on hardware. History since Rev 6:

| Test | Change |
|---|---|
| 046 | Dial decoded once into PSRAM instead of re-read from SD every second; big whole-degree temperature; boot prints dial status |
| 047 | **Fixed the boot crash.** Files list no longer built at boot; LVGL memory guard; row cap; `LV_MEM_SIZE` raised |
| 048 | Clock face moved left (`CLOCK_CX`/`CLOCK_CY`); temperature enlarged into the freed space |
| 049 | Temperature smaller, C moved to its right on the same bottom line; dial buffer cleared before decode; decode block counter; failures reported on screen |
| 050 | Temperature drawn with a real TrueType-derived GFX font (`temp_font.h`) instead of the 5×7 bitmap scaled 20× |
| 051 | `temp_font.h` regenerated — 050 would not compile (see §9) |
| 052 | Settings → Clock hour/colon/minute changed to black; they were invisible on LVGL's light-theme tab page |
| 053 | **SD card retired**, dial moved into flash (`dial_image.h`) |
| 054 | **INMP441 microphone** — capture, RMS/peak, level meter on Home |
| 055 | **Screensaver flicker fixed** — off-screen composition |

### Screensaver architecture (TEST 055)

- The dial is a 16921-byte baseline 4:2:0 JPEG compiled into flash as
  `dial_image.h`. At boot it is decoded once into a 768 KB PSRAM buffer.
- Each tick, a 424×424 PSRAM buffer covering the clock face is filled from
  the cached dial, the hands are rasterised **into that buffer**, and the
  finished image is blitted **once**. The screen never holds a half-drawn
  frame — this is what removed the 1 Hz flicker.
- The full screen is painted only on entering the screensaver.
- The temperature is repainted only when the whole number changes.
- PSRAM total: dial 768 KB + compose 360 KB + LVGL 96 KB ≈ 1.22 MB of 8 MB.

`CLOCK_CX`/`CLOCK_CY` (240,240) must match the dial artwork, which is centred
on x=239. Changing one without the other puts the hands off the face.

### Microphone (TEST 054)

INMP441 on I2S0. VDD→3.3V, GND→GND, **L/R→GND** (left channel), SD→IO11,
SCK→IO12, WS→IO13.

`ENABLE_MIC 1` and `ENABLE_SD 0` at the top of `main.cpp`. They share pins
and are mutually exclusive; `micBegin()` refuses to start if both are 1.

**Status: capture works.** A level bar sits along the bottom of the Home
screen and serial prints `mic: rms … peak … reads …` once a second.
**There is no keyword recognition yet** — that is the next real project.

---

## 4. Bed box — UNCHANGED, AND STILL THE OLDEST OPEN ITEM

**Hardware:** QuinLED-ESP32 (WROOM-32E), 8× 12V ERM vibration motors via
4× L298N H-bridges, 12V supply separate from the panel.

- Motor PWM pins: GPIO 13, 14, 18, 19, 21, 22, 23, 25
- 4 zones × 2 motors: Head, Shoulders, Back, Legs
- `MIN_DUTY_BIG = 165`, `MIN_DUTY_SMALL = 60`; PWM 20 kHz, 8-bit
- 60 ms full-duty kick-start breaks the motors loose from rest
- UART2 to panel: RX = GPIO16, TX = GPIO17 @ 115200

| Panel P3 | → | QuinLED |
|---|---|---|
| IO17 (TX) | → | GPIO16 (RX) |
| IO18 (RX) | → | GPIO17 (TX) |
| GND | → | GND |

Crossed: TX goes to RX. **Each board on its own USB. Do NOT connect 5V** —
ground only.

### Command protocol — 4 bytes

`{bedId, cmd, target, value}`

| Byte | Meaning |
|---|---|
| 0 | bedId — 0 both, 1 Shemi, 2 Ira |
| 1 | cmd — 0 OFF, 1 ALL, 2 ZONE, 3 MOTOR, 4 PRESET, 5 TIMER |
| 2 | target — zone id, motor index, or preset number |
| 3 | value — intensity 0–100, or minutes |

### WHAT IS IN THE REPO IS NOT THE LATEST

`bedbox/src/main.cpp` holds **TEST 007**: a standalone motor demo with a
serial console. It does **not** listen to the panel, which is why massage
taps still do nothing.

**TEST 0014 exists** — the UART receiver, written and uploaded 22 July,
never committed. It is in the chat **"Bed door project"**,
`claude.ai/chat/c5d478a6-4aec-4178-89f9-9e7918bb4c8c`

**Chat attachments expire — that is exactly how TEST 044 was lost.** If it
has already gone, rebuilding is straightforward: TEST 007 already contains
`handleMessage(bedId, cmd, target, value)` with all six commands. The
receiver is that plus roughly twenty lines — open `Serial2` on RX=16/TX=17
at 115200, collect four bytes, call the existing function.

### bedbox platformio.ini

```ini
[env:bedbox]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600
```

`board = esp32-wroom-32` is rejected; `lib_deps = arduino-esp32/ESP32 @
^2.0.14` must not be present; use Arduino `ledcSetup`/`ledcAttachPin`/
`ledcWrite`, never a raw `driver/ledc.h` include.

**Untested warning:** the bed box uses the core 2.x LEDC API, and an
unpinned `platform = espressif32` resolves to core 3.2.0 where those
functions do not exist. Expect `'ledcSetup' was not declared in this scope`;
if so add `platform = espressif32 @ 6.9.0` as the panel has.

---

## 5. SD card — retired, not removed

`ENABLE_SD 0` in `main.cpp`. `SPI.begin()` and `SD.begin()` are never called,
so CS is never asserted, the card holds its data line high-impedance, and the
microphone owns IO11/12/13 with no contention. The card can be left in the
slot or taken out; nothing reads it.

**Lost while disabled:** the photo slideshow, the Files browser, and the
welcome photo.

**To bring it back** (when the microphone moves to another board): set
`ENABLE_SD 1` and `ENABLE_MIC 0`. The code was guarded, not deleted.

The card still holds `welcome_800x480.jpg`, `diver_dial.jpg`,
`elevoc_dnn_kernel` and `/photos/pic1…pic38.jpg`. **The card remains the only
copy of those photos** — the repo-visibility decision is still open, since
`Bg_Bed` is public and the pictures are personal.

---

## 6. Panel UI

**State machine:** Home → *(idle)* → diver-clock screensaver → *(touch)* →
Home. The welcome photo is skipped while SD is disabled.

**Home:** icon tiles Massage / Radio / AC / Settings, temperature with
pressure trend top-right, `TEST 055` badge, **microphone level bar along the
bottom**.

**Massage:** bed selector — Shemi (blue), Ira (red), Both (green); 4 zone
sliders; intensity 0–100; presets including Random with a character setting.

**Settings tabs:** Clock (+/− hour, minute, AM/PM, `Set Time`) · Display
(min brightness 30–100, screensaver timeout) · Massage (MIN_DUTY sliders,
random character) · Files (says SD is disabled) · About.

All settings persist in flash via Preferences. Serial command `time HH:MM`.

**Note on colours:** `LV_CONF_SKIP` leaves `LV_THEME_DEFAULT_DARK` at 0, so
LVGL renders its **light** theme — tab pages are near-white. Pale text
vanishes there. Fixed for the clock setter (`SET_TEXT_COL`); the About text
(`0x90C0E0`) and the massage note (`0x7B90A0`) still have the same problem.
Adding `-DLV_THEME_DEFAULT_DARK=1` would fix all of it at once but repaints
every screen.

---

## 7. Generated assets

Both are produced from scripts and can be regenerated on request.

**`include/temp_font.h`** — screensaver temperature font, from Noto Sans Bold
outlines. Digits 150 px, C 46 px, about 27 KB of flash. Covers `-`, `0`–`9`
and `C` only.

**`include/dial_image.h`** — the dial, 800×480 baseline JPEG, 4:2:0, 16921
bytes. Generated as an original drawing, not a photograph.

---

## 8. Next steps

**Immediate**
1. Recover bed box TEST 0014 before the attachment expires
2. Commit it, build it, pin the platform if it fails
3. Test end-to-end: a massage tap on the panel spins a motor
4. Supply `welcome_800x480.jpg` so it can be embedded in flash like the dial

**Voice recognition — the real remaining project**
Capture works; recognition does not exist. The Hebrew wake word `אָחִינוּ`
cannot come from Espressif's ESP-SR, which ships fixed wake words and no
Hebrew. A custom word means training a model (microWakeWord or Edge Impulse)
from hundreds of recorded samples. Decide the approach before writing code.
Note also that the panel is already spending 1.22 MB of PSRAM and driving an
800×480 RGB panel with LVGL — running inference here will contend for memory
bandwidth. Moving the microphone to the WT32-ETH01 audio node, on the UART
bus, is likely the better home for it.

**Soon**
5. Decide on the SD photos and repo visibility
6. Buy DS3231 RTC (~₪10) and SHT31 or BME280 humidity sensor (~₪10–15)
7. Second bed box: QuinLED, 4× L298N, 12V 5A PSU
8. Clone `soulcollage-ira`, `Solarium-website`, `Tractor-2023` into `C:\projects`

**Later**
9. Audio node — WT32-ETH01 + PCM5102A, internet radio over UART from the panel
10. AC control via IR or Switcher Breeze
11. Hebrew UI text — needs a custom LVGL font

---

## 9. Hard-won lessons

**From this session**

- **The panel has seven exposed GPIOs and all are used.** Check the pin
  budget before buying or wiring any peripheral.
- **LVGL's default heap is 32 KB** when `LV_CONF_SKIP` is set. Building one
  list button per SD file at boot exhausted it; the allocator returned NULL
  and LVGL dereferenced it — `StoreProhibited`, `EXCVADDR 0x00000098`, inside
  `lv_obj_class_create_obj`. Build lists on demand, and check
  `lv_mem_monitor()` before adding widgets in a loop.
- **`GFXglyph` stores `xOffset`/`yOffset` as `int8_t` and `GFXfont` stores
  `yAdvance` as `uint8_t`.** A 150 px font referenced from the baseline needs
  −147 and 284 and will not compile. Reference offsets from the top of the
  digits instead. `bitmapOffset` is `uint16_t`, capping a font's bitmap at
  65535 bytes.
- **JPEGDEC decodes baseline JPEG only.** Progressive files open fine on a PC
  and fail silently on the ESP32. Prefer plain 4:2:0, no optimised Huffman.
- **A full-screen repaint on an RGB panel flickers**, because the panel scans
  continuously and can show a half-drawn frame. Compose off-screen and blit
  once. Caching the source image speeds it up but does not fix the flicker.
- **The INMP441 is a 24-bit mic in 32-bit slots.** Requesting
  `I2S_BITS_PER_SAMPLE_16BIT` returns near-silence that looks exactly like a
  wiring fault.
- **`i2s_pin_config_t`'s first field is `mck_io_num`.** A designated
  initialiser that omits it silently selects GPIO0, a strapping pin. Assign
  every field explicitly.
- **Never let a failure be silent.** Every hour lost this session was to
  something that failed without saying so — a missing dial, an unfilled
  buffer, an invisible label. Print the reason, and if possible put it on the
  screen.

**Standing**

- Known-good beats deduced. Read the working file rather than reasoning from
  the code.
- Chat attachments expire. Anything delivered in a chat gets committed the
  same day.
- Version numbers belong in commit messages, not filenames.
- A black screen with no serial output means the failure is before `setup()`.
  Look at flash and PSRAM settings, not at the code.
- `board_build.*` and `board_upload.*` are different things and both matter.
- Keep project paths free of Hebrew characters.

---

*End of handout — Revision 7.*
