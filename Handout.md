# Adjustable Bed Massage Retrofit — Project Handout

**Revision 6 — 31 July 2026**
Supersedes Rev 5 (21 July). Self-contained: this document alone should let a
future session pick up without re-learning the project.

---

## 1. Goal

A DIY smart bedside system for two adjustable beds shared by Shemtov (Shemi)
and Ira, in Petah Tikva, Israel. One central touch panel on the headboard
controls vibration massage on both beds, plus a clock face with environmental
sensors, photo screensaver, and later internet radio and AC control.

**Governing rule: zero RF at the heads.** No WiFi, Bluetooth or ESP-NOW near
the sleeping positions. All inter-device communication is wired UART. Only a
future audio node will have a radio, and it lives away from both heads.

---

## 2. Source control — NEW IN REV 6

The whole project now lives in git. This is the single biggest change since
Rev 5, and it exists because firmware TEST 041 through 044 was lost with no
recoverable copy anywhere.

**Repository:** `github.com/shemtov/Bg_Bed` (public, branch `main`)
**Local working copy:** `C:\projects\Bg_Bed`

```
Bg_Bed/
├── panel/       platformio.ini, src/main.cpp (TEST 045), include/
├── bedbox/      platformio.ini, src/main.cpp (TEST 007)
├── audionode/   not started
├── .gitignore
└── Handout.md
```

**Working loop:** edit in VS Code → save → GitHub Desktop → write a summary →
`Commit to main` → `Push origin`.

**Conventions**
- Filenames never carry version numbers. The code file is always
  `src/main.cpp`. The version lives in `#define TEST_NUMBER n` inside the
  file, in the banner top and bottom, and in the boot print.
- Version numbers go in commit messages, not filenames. Git holds the history.
- `.pio/` is never committed. It is build output and is regenerated.
- Each of `panel/`, `bedbox/`, `audionode/` is a separate PlatformIO project.
  Open that folder in VS Code — never the repository root.

**Other machines and backups.** This PC is `C:\Users\User`. A second machine
under `C:\Users\eira 7-25` holds the soulcollage project. Stale backups still
on disk, safe to delete once trusted: `C:\projects\BedSystem_Beckup` and
`C:\Users\User\Desktop\massage_bed`.

---

## 3. Panel

**Hardware:** Guition ESP32-8048S043 — ESP32-S3-WROOM-1 N16R8, 16MB flash,
8MB octal PSRAM, 4.3" 800×480 IPS parallel RGB (ST7262), GT911 capacitive touch.

| Function | Pins / address |
|---|---|
| I2C bus | SDA = IO19, SCL = IO20 |
| BH1750 ambient light | 0x23 — present |
| GT911 touch | 0x5D — present |
| BMP280 temp + pressure | 0x76 — present |
| DS3231 RTC | 0x68 — **absent, not yet bought** |
| SD card (SPI) | CS=IO10, MOSI=IO11, CLK=IO12, MISO=IO13. **FAT32 only** |
| Backlight | GPIO2, LEDC channel 7, 5 kHz, 8-bit |
| UART1 → bed box | TX = IO17, RX = IO18 @ 115200, via P3 header |
| USB / serial monitor | UART0, 115200 |

### platformio.ini — this took six attempts, do not change casually

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
  -DLV_FONT_MONTSERRAT_20=1
  -DLV_FONT_MONTSERRAT_28=1
  -DLV_FONT_MONTSERRAT_40=1

lib_deps =
  moononournation/GFX Library for Arduino @ 1.3.8
  bitbank2/JPEGDEC @ ^1.2.8
  lvgl/lvgl @ ^8.3.11
```

**Why each awkward line is there — all three were learned the hard way:**

1. **`@ 6.9.0` is mandatory.** Unpinned, `espressif32` resolves to 54.3.20 on
   this machine, which ships Arduino core 3.2.0. Core 3.x removed
   `ledcSetup()` and `ledcAttachPin()`, and GFX Library 1.3.8 will not compile
   against the ESP-IDF 5.4 headers that come with it. 6.9.0 gives core 2.0.x.
2. **The `board_upload.*` lines are mandatory.** `board_build.*` configures the
   compiler; `board_upload.*` configures the bootloader header. With only the
   build side set, the bootloader believed the chip was the 8MB no-PSRAM N8
   variant, could not find the app inside a 16MB partition table, and reset
   forever — black screen, no serial output at all, `rst:0x3 RTC_SW_SYS_RST`.
3. **`LV_CONF_SKIP`** means LVGL is configured entirely by the build flags
   above. **No `lv_conf.h` file is needed anywhere.**

`monitor_filters = esp32_exception_decoder` turns a crash backtrace into file
names and line numbers. Leave it on.

### Panel firmware — TEST 045

**TEST 045 is a reconstruction, not recovered code.** TEST 041–044 were lost:
not on any disk, not in Google Drive, and the chat attachment holding 044 had
expired. TEST 045 was rebuilt on top of `panel_test040.cpp` — which was itself
a failed minute-roller attempt — using the change descriptions in Rev 5. The
number 045 was chosen deliberately so a reconstruction is never mistaken for
the original tested 044.

Rebuilt into it:

- **from 041** — memory-safe photos: 600 KB size guard, PSRAM allocation with
  ordinary-heap fallback, buffer freed on every exit path. Fixes the freeze at
  38+ photos.
- **from 041** — Files-tab slideshow: every photo in `/photos`, 2 s each,
  wraps around, any touch stops it.
- **from 042** — time picker rebuilt as `[-] H [+] : [-] MM [+] [AM/PM]`
  buttons. All LVGL roller code is gone; the rollers never worked.
- **from 042** — 12-hour AM/PM on the settings fields, the LVGL clock and the
  7-segment screensaver (leading zero blanked).
- **from 043** — clock hands dimmed to about 70% on both clock faces.
- **from 044** — null guards on `updateSensorLabel`, `updateClockFace`,
  `updateAnalog` and `refreshSetterLabels`, with all display pointers
  initialised to NULL.

**Status: compiles, uploads, boots, and runs.** Serial prints
`=== PANEL — TEST 045 — LVGL UI + Settings ===`, all three I2C sensors are
found, LVGL comes up, welcome photo displays, home screen responds.

**Not yet verified feature by feature.** The +/- clock setter, the slideshow,
the dimmed hands and a long run of photos have not been individually tested.

**One unexplained event.** After inserting the SD card the panel crashed with a
real backtrace and rebooted every ~2 s. It then stopped on its own and has been
stable since. No fix was applied and no cause was found — treat as unresolved.
Note that the old chat described **five** null-guard edits for TEST 044 and only
four were reconstructed; a missing fifth guard is a plausible cause. The
exception decoder is now enabled, so a recurrence will name the exact line.

### Brightness

Auto-brightness maps BH1750 lux to backlight duty on a logarithmic curve
between `brightFloor` and 100%. Default floor is 85%. In a dark bedroom the
screen therefore sits near the floor and looks faint.

**Immediate control, no code change:** Settings → Display → "Min brightness
(night)" slider, range 30–100. Setting it to 100 pins the backlight at full
duty. Saved to flash.

---

## 4. Bed box

**Hardware:** QuinLED-ESP32 (WROOM-32E), 8× 12V ERM vibration motors via
4× L298N H-bridges, 12V supply separate from the panel.

- Motor PWM pins: GPIO 13, 14, 18, 19, 21, 22, 23, 25
- 4 zones × 2 motors: Head, Shoulders, Back, Legs
- Calibrated thresholds: `MIN_DUTY_BIG = 165`, `MIN_DUTY_SMALL = 60`
- PWM: 20 kHz, 8-bit. *(Rev 5 said 5 kHz — the code says 20000, code wins.)*
- Kick-start pulse of 60 ms at full duty breaks the motor loose from rest
- UART2 to panel: RX = GPIO16, TX = GPIO17 @ 115200

### Wiring, panel ↔ bed box

| Panel P3 | → | QuinLED |
|---|---|---|
| IO17 (TX) | → | GPIO16 (RX) |
| IO18 (RX) | → | GPIO17 (TX) |
| GND | → | GND |

Crossed: TX goes to RX. If nothing happens, swap IO17 and IO18.
**Each board on its own USB. Do NOT connect 5V between them** — ground only.

### Command protocol — 4 bytes

`{bedId, cmd, target, value}`

| Byte | Meaning |
|---|---|
| 0 | bedId — 0 both, 1 Shemi, 2 Ira |
| 1 | cmd — 0 OFF, 1 ALL, 2 ZONE, 3 MOTOR, 4 PRESET, 5 TIMER |
| 2 | target — zone id, motor index, or preset number |
| 3 | value — intensity 0–100, or minutes |

### Bed box firmware — WHAT IS IN THE REPO IS NOT THE LATEST

`bedbox/src/main.cpp` currently holds **TEST 007**: a standalone motor demo
with a serial console. It does **not** listen to the panel, which is why
massage taps do nothing.

**TEST 0014 exists.** A session on 22 July wrote and uploaded the UART
receiver firmware for the bed box. It has never been copied into the repo.
It is in the chat **"Bed door project"** —
`claude.ai/chat/c5d478a6-4aec-4178-89f9-9e7918bb4c8c`

**Recovering it is the single highest priority.** Chat attachments expire —
that is exactly how TEST 044 was lost. Get it before doing anything else.

If the attachment has already expired, rebuilding it is straightforward:
TEST 007 already contains `handleMessage(bedId, cmd, target, value)` with all
six commands implemented. The receiver is TEST 007 plus roughly twenty lines —
open `Serial2` on RX=16/TX=17 at 115200, collect four bytes, call the existing
function.

### bedbox platformio.ini

```ini
[env:bedbox]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600
```

Known from the 22 July session: `board = esp32-wroom-32` is rejected,
`lib_deps = arduino-esp32/ESP32 @ ^2.0.14` must not be present, and the
firmware must use Arduino `ledcSetup`/`ledcAttachPin`/`ledcWrite`, never a raw
`driver/ledc.h` include.

**UNTESTED WARNING, new in Rev 6.** The bed box firmware uses the core 2.x
LEDC API. We now know that on this machine an unpinned `platform = espressif32`
resolves to 54.3.20 / Arduino core 3.2.0, where those functions do not exist.
The bed box will very likely fail to compile with exactly the
`'ledcSetup' was not declared in this scope` error the panel hit. If it does,
add `platform = espressif32 @ 6.9.0`. This has not been tested yet.

---

## 5. SD card layout (FAT32)

```
/  (root)
├── welcome_800x480.jpg     boot photo, 800×480
├── diver_dial.jpg          dive-watch clock face, 800×480
├── elevoc_dnn_kernel       voice file — leave alone
└── /photos/
    └── pic1.jpg … pic38.jpg    screensaver rotation, 800×480 each
```

~38 photos, ~2.7 MB total. **exFAT is not supported.** If the serial log shows
`f_mount failed` or `Card Failed`, reseat the card firmly and confirm FAT32.

The SD card is currently the only copy of these images. They are not in the
repo — a decision is pending, because `Bg_Bed` is public and the photos are
personal. Options: make the repo private and commit them, commit only
`diver_dial.jpg`, or back the photos up to Google Drive separately.

---

## 6. Panel UI

**State machine:** Welcome photo → Home → *(idle)* → diver-clock screensaver →
*(touch)* → Home

**Home:** icon tiles Massage / Radio / AC / Settings, temperature with pressure
trend arrow top-right, `TEST 045` badge top-right.

**Massage:** bed selector — Shemi (blue), Ira (red), Both (green); 4 zone
sliders; intensity 0–100; presets including Random with a character setting.

**Settings tabs:**
- **Clock** — +/- hour, minute and AM/PM fields, `Set Time` button
- **Display** — min brightness 30–100, screensaver timeout
- **Massage** — MIN_DUTY_SMALL and MIN_DUTY_BIG sliders, random character
- **Files** — SD browser, tap a photo to preview, Slideshow button
- **About** — test number, sensor status

All settings persist in flash via Preferences.

**Serial commands:** `time HH:MM` sets the clock, e.g. `time 14:30`.

---

## 7. Loose files in `panel/`

`microphone.cpp`, `microphone.h` and a legacy `config.h` sit in the panel
project root. **PlatformIO only compiles `src/`, so none of them are built**,
and TEST 045 does not include them. The microphone pair is presumably the start
of the voice-recognition idea; `config.h` is from an abandoned Waveshare-based
design that used ESP-NOW and 10 motors in 5 zones. Both are committed and safe.

---

## 8. Next steps

**Immediate**
1. Recover bed box TEST 0014 from the 22 July chat before it expires
2. Put it in `bedbox/src/main.cpp`, build it, pin the platform if it fails
3. Test end-to-end: massage tap on the panel spins a motor on the bed box
4. Verify the four rebuilt TEST 045 features individually

**Soon**
5. Decide what to do about the SD photos and repo visibility
6. Buy DS3231 RTC (~₪10) and SHT31 or BME280 humidity sensor (~₪10–15)
7. Second bed box: QuinLED, 4× L298N, 12V PSU 5A
8. Clone `soulcollage-ira`, `Solarium-website` and `Tractor-2023` into
   `C:\projects` so everything lives in one place

**Later**
9. Audio node — ESP32 + PCM5102A DAC, internet radio over UART from the panel
10. AC control via IR or Switcher Breeze
11. Hebrew UI text — needs a custom LVGL font

---

## 9. Hard-won lessons

- Known-good beats deduced. When a working config file exists, read it rather
  than reasoning from the code. Three wrong guesses in one day came from this.
- Chat attachments expire. Anything delivered in a chat gets committed the
  same day or it will eventually be lost.
- Version numbers belong in commit messages, not filenames. Two parallel
  numbering schemes — firmware tests and .ini versions — is what made "007 or
  009?" impossible to answer.
- A black screen with no serial output means the failure is before `setup()`.
  Look at flash and PSRAM settings, not at the code.
- `board_build.*` and `board_upload.*` are different things and both matter.

---

*End of handout — Revision 6.*
