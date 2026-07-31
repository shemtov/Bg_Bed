# Adjustable Bed Massage Retrofit — Project Handout

**Revision 4** · Self-contained reference · Prepared to carry into a fresh conversation

---

## 0. How to use this document

This is the single source of truth for the project. It captures the full architecture, every hardware decision (including the ones we **rejected** and why, so they don't get re-proposed), the agreed protocols and pin maps, the build roadmap, and the exact current status. A new chat given this document has everything it needs — no older chats required.

The builder is retrofitting a massage system into two beds shared with his wife (Ira). He communicates by voice dictation, so if something reads oddly it may be a transcription artifact — ask rather than guess.

---

## 0b. Code-delivery workflow (standing rule — always follow)

Every firmware deliverable in this project follows these rules, without exception:

1. **Whole program only — never corrections or fragments.** Even for a one-character change, the *entire* program is re-issued. The builder select-alls the old code and pastes the new program as a whole.
2. **One complete C++ code block, delivered as an artifact on the right**, in **C++ mode** (`.cpp`, not `.ino`) — copied in one shot. Files include `#include <Arduino.h>` and forward declarations so they are valid C++.
3. **Test number at the top and at the bottom** of every program, as a comment banner, plus a `#define TEST_NUMBER n`. **One shared sequence across all nodes** (bed box, panel, audio) — a number points to exactly one program.
4. **Boot prints the test number** over Serial (e.g. `=== BED BOX — TEST 001 — ...`), and **later the panel screen displays it**; bed boxes/audio node report their test number over UART so the code in hand, the hardware, and the screen always agree. The protocol/UI will include a small "report your version" message for this.

**Sequence so far:**
- `TEST 001` — bed box Stage 1, core 3.x PWM (`ledcAttach`). *Superseded.*
- `TEST 002` — switched to core 2.x PWM (`ledcSetup`/`ledcAttachPin`) after a compile error; the user's PlatformIO ships Arduino-ESP32 core 2.x. *Superseded.*
- `TEST 003`, `TEST 004` — renumbered reissues (per the "new number every change" rule) while sorting out the Serial Monitor baud (fixed via `platformio.ini` → `monitor_speed = 115200`, plus `monitor_echo = yes` and `monitor_filters = send_on_enter` so typed commands reach the board).
- `TEST 005` — added the automatic **two-motor demo** (solo ramps, alternate 1s/2s, both-together, cross-fade, random, pulse) plus the manual console; press ENTER to stop the demo, type `demo` to restart.
- `TEST 006` — calibrated thresholds baked in.
- `TEST 007` — **current bed-box build.** Bench motor sizes flipped to match wiring (channel 0 = big, channel 1 = small).
- `TEST 008` — panel display bring-up CONFIRMED (color bars correct, test number on screen, heartbeat). GFX Library pinned to **1.3.8** (1.4.x needs core 3.x). Panel is a separate PlatformIO project ("Panel", board `esp32-s3-devkitc-1`, `qio_opi` 16MB/8MB, `-DARDUINO_USB_CDC_ON_BOOT=0`).
- `TEST 009` — GT911 touch bring-up (I2C, SDA=19/SCL=20, polled). GT911 answers at **0x5D**. Touch worked but coordinates were mis-scaled.
- `TEST 010` — touch scaling fix CONFIRMED: GT911 reports a raw **480×272** range (the 4827S043 sibling's config); scale x×800/480, y×480/272 to the 800×480 display. Orientation was already correct (no mirror/swap). Four-corner test passed — dots land under the finger edge to edge.
- `TEST 011` — **current panel build.** Welcome photo from microSD at boot: mounts SD (SPI CS=10/MOSI=11/CLK=12/MISO=13), decodes `/welcome_800x480.jpg` via JPEGDEC, shows it full-screen ~2s (`WELCOME_MS`), then the touch screen. Graceful fallback if card/file missing. Panel `platformio.ini` is now **v3** (adds `bitbank2/JPEGDEC`). The welcome image (`welcome_800x480.jpg`, 800×480, mirrored L-R per user request) goes in the SD card root.

Next sketch of any node = TEST 012 (candidates: I2C sensors BH1750+SHT31 on the touch bus, OR the first panel↔bed-box UART link).

---

## 1. The one governing principle (read first)

**The headboard area must stay RF-free during sleep.** No WiFi, no Bluetooth, no ESP-NOW — no radio transmitter of any kind — near the heads. This is a hard, non-negotiable constraint and it is the reason behind most of the architecture below. Any radio (WiFi for internet radio, WiFi to the AC controller, Bluetooth) is allowed **only** on nodes placed physically away from the heads (foot of the bed, or across the room). All devices at the headboard communicate over **wired UART only**.

When evaluating any new idea, check it against this principle first.

---

## 2. System overview (Revision 4)

The system has four kinds of nodes, all linked by wired UART (115200 baud, shielded twisted pairs run alongside the DC cabling):

1. **One central headboard panel** — the touch UI and voice control. RF-silent.
2. **Two bed boxes** — one per bed, each drives that bed's vibration motors.
3. **One audio / WiFi node** — internet radio + the only WiFi in the system + gateway to the AC. Placed away from the heads.
4. **The AC controller(s)** — off-the-shelf Switcher Breeze unit(s) near the air conditioner(s).

```
        [ Central Panel ]  (Waveshare S3, touch + mic, RF-silent)
                 |  UART (5V, GND, TX, RX) — single slim cable
                 |
     +-----------+-----------+------------------+
     |           |           |                  |
 [Bed Box 1] [Bed Box 2] [Audio/WiFi Node] .....(UART links)
  (QuinLED)   (QuinLED)   (classic ESP32)
     |           |           |
  8 motors    8 motors    PCM5102A -> XH-M543 amp -> speakers
 via 4x L298N  ...         + WiFi internet radio
                           + WiFi/TCP -> Switcher Breeze -> IR -> A/C
```

### What changed from Revision 3 → Revision 4
- **One central panel instead of two.** Velcro-mounted at the center of the headboard, serves both beds via a bed-selector in the UI. Saves the cost of a second panel. (Panel-to-panel messaging is therefore dropped — it's meaningless with one panel. The alarm becomes a single shared alarm.)
- **Panel stays wired, not cordless.** Cordless was considered and rejected (Section 9). A single slim 4-wire cable drops behind the headboard with enough slack to lift the panel off its Velcro and hold it like a handset.
- **Voice control added** (offline, on the panel — Section 7).
- **AC control confirmed and specified** via Switcher Breeze (Section 8).
- **Motor count is 8, in 4 zones** (was 10 in 5 zones). Only 8 motors are on hand; the design leaves room to grow back to 10.

---

## 3. The bed box (the heart of the system)

Each bed has one bed box. This is where the current build effort is focused.

### Controller
**QuinLED-ESP32** (custom board carrying an Espressif **ESP32-WROOM-32E**). D1-Mini32 form factor but **4×9 pins, not 4×10** (the missing pins are internal-flash pins). Has a **U.FL external-antenna connector — leave it empty**; WiFi is never enabled on the bed box, so with no antenna nothing radiates even accidentally. Chosen over the ESP8266 D1 Mini because it has hardware LEDC PWM (16 channels, jitter-free), enough GPIO, and three hardware UARTs.

**Board caution — 750 mA PTC fuse:** the board is *not* meant to pass big loads through itself. Motor power comes from the 12V supply through the L298N modules — **never** through the QuinLED. Control signals draw almost nothing, so this is fine.

### Motors (confirmed facts)
- **12V ERM vibration motors.**
- **Large motors: 0.3 A each. Small motors: 0.1 A each.**
- Both types verified to start spinning even with only 5V at the input.
- ERM motors vibrate identically regardless of rotation direction → **direction pins are hardwired**, only the PWM enable line is controlled.
- **Currently only 8 motors are owned**, so the build starts at 8.
- **Start-threshold duty cycle — CALIBRATED (bench, one small + one big motor):** small motor cold-starts at duty **55** (~22%), big motor at duty **165–180** (~65–71%, varies run to run as ERM thresholds naturally drift). Because a motor needs more duty to *start* than to keep *running*, and the kick-start pulse breaks it loose, the firmware's running minimums are set just below cold-start: **`MIN_DUTY_SMALL = 60`, `MIN_DUTY_BIG = 165`**. Confirmed working: both motors start and hold a gentle buzz at intensity **1**. Below the threshold an ERM stalls silently, so firmware maps user intensity 1–100% onto (minDuty…255) and adds a brief full-power kick-start pulse on turn-on.

### Zone map (4 zones × 2 motors)

| Zone id | Zone | Motors | L298N module | GPIO ENA / ENB |
|---|---|---|---|---|
| 0 | Head | 2 small (0.1A) | #1 | **13 / 14** |
| 1 | Shoulders | 2 big (0.3A) | #2 | **18 / 19** |
| 2 | Back | 2 big (0.3A) | #3 | **21 / 22** |
| 3 | Legs | 2 small (0.1A) | #4 | **23 / 25** |

Motor indices for the MOTOR command: 0–1 Head, 2–3 Shoulders, 4–5 Back, 6–7 Legs.
**GPIO 26 & 27 are reserved** for a future 5th zone (both protocol and hardware slots are ready).
Because the Head zone sits nearest the ears, firmware may cap its max duty / give it a softer curve.

### Motor drivers
**L298N** dual H-bridge modules, **4 per bed box** (8 total for two beds). Rated to 46V — massive margin at 12V. Each motor loads a bridge to only ~15% of its 2A rating, so the heatsinks stay cool.
Per-module wiring for single-direction PWM: **ENA/ENB = PWM from ESP32**; **IN1/IN3 held LOW, IN2/IN4 held HIGH** (fixes direction); remove the ENA/ENB jumper caps so the GPIO drives them.
**12V jumper note:** the onboard 5V regulator jumper *may* stay in place at exactly 12V (only remove it above 12V). Still preferred for the final build: power the QuinLED's logic from a dedicated buck converter, not the L298N's onboard regulator — cleaner and more consistent.

### Bed-box sensors
Environmental sensing moved to the **panel** (both sensors are I2C and drive the panel's screen): **SHT31** temp/humidity and **BH1750** light — see Section 5. The analog **LDR** is dropped (BH1750 replaces it). A bed-box may still carry its own SHT31 later if per-bed temperature is wanted, reporting over UART, but this is not required. **PIR** motion for screen wake/sleep lives at the panel.

### Power (per bed box)
12V rail feeds the motors directly. Worst case with 8 motors is ~1.6A, so a **12V / 5A supply is plenty** (5A chosen for cheap headroom; even 3A would work). A small buck converter drops 12V → 5V for the QuinLED.

---

## 4. Communication protocol

All inter-device links are wired UART, 115200 baud. Core motor protocol is a **4-byte binary message**:

```
{ bedId, cmd, target, value }
```

- **bedId:** 1 = bed #1, 2 = bed #2, 0 = broadcast (both)
- **cmd:**
  - `0` OFF
  - `1` ALL  → value 0–100 (%)
  - `2` ZONE → target = zone id (0 Head / 1 Shoulders / 2 Back / 3 Legs), value 0–100
  - `3` MOTOR → target = motor 0–7, value 0–100
  - `4` PRESET → target = preset (0 Wave / 1 Pulse / 2 Ripple), value = level
  - `5` TIMER → value = minutes (5–120); stops motors when elapsed but keeps saved settings

Bed boxes **save state to flash and auto-resume** the last massage on power-up. The session timer stops the motors but preserves settings for the next boot.

**To be extended later:** audio-node commands (radio station, volume, ducking) and AC commands (power / temp / fan / light). Likely each bed box and the audio node get their own UART line off the panel (the S3 has several).

---

## 5. The central panel

**Guition/JC ESP32-8048S043** (this is the confirmed panel, chosen over the Waveshare). Verified specs: **ESP32-S3-WROOM-1-N16R8** (dual-core S3, **16MB flash, 8MB PSRAM**), **4.3" 800×480 IPS** display driven by the **ST7262** over a parallel RGB-565 interface, **GT911** capacitive touch on I2C (addr 0x5D). USB-C via CH340C. UI framework: **LVGL** (this board is one of the most widely supported LVGL panels; the display works with the "GFX Library for Arduino" / `Arduino_GFX` RGB panel driver). Broken-out headers: **P1 UART** (GND, RX, TX, +5V), **P3** (IO20, IO19, IO18, IO17), **P4** (IO18, IO17, +3.3V, GND), plus SPI (IO13/12/11/19) and the SD slot.
- Velcro-mounted center headboard; single slim 4-wire cable (5V, GND, TX, RX) with slack to hold it in hand.
- **No radio active, ever.** UART only. (The 8MB PSRAM is also what makes on-device voice recognition possible — Section 7.)
- **Touch-interrupt quirk:** by default the GT911 INT pin isn't wired to a GPIO, so touch is polled (high CPU on static screens). Optional fix: 0-ohm bridge across R17 routes INT to GPIO18 (and remove pull-up R5). Optional — polling also works.
- The Waveshare ESP32-S3-Touch-LCD-4.3 (previously the planned panel) becomes a spare / LVGL practice board — same resolution, so UI transfers directly.

### Panel sensors (both digital, shared I2C bus)
- **BH1750** ambient-light sensor (I2C, addr 0x23) → reports real **lux**, drives screen auto-brightness. **Replaces the analog LDR** entirely (no ADC pin / divider needed). *(owned)*
- **SHT31** temp/humidity (I2C, addr 0x44) → clock-face readout. *(owned)*
- Both share **one I2C bus** with each other (different addresses): SDA/SCL + 3.3V + GND. The board's existing I2C (used by GT911) is **SDA=IO19, SCL=IO20**; the sensors can join that bus. Exact final pins to be confirmed in the panel pin map.
- **PIR** (motion, for screen wake/sleep) — still planned; will use a free GPIO on the panel.

### Screen state machine
1. **Welcome photo** of the builder and Ira (placeholder until the photo is supplied).
2. **Home screen** — tiles: Massage / Radio / AC. Massage screen includes the **bed selector** (which bed / both).
3. After ~2 min idle → **Clock face** (large digits + temp/humidity from SHT31, brightness auto via BH1750 lux).
4. After ~10 min no PIR motion → **screen fully dark**.
5. The panel also **displays the running firmware test number** (and collects each node's test number over UART), per the code-delivery workflow.

---

## 6. The audio / WiFi node

The only WiFi-connected brain in the system, placed **away from the heads** (foot of bed or in a bed box).

**Wired audio chain (kept — Bluetooth alternatives were rejected, Section 9):**
`classic ESP32  →  PCM5102A I2S DAC  →  XH-M543 (TPA3116D2 class-D, ~2×120W)  →  passive hi-fi speakers on bedside tables (speaker wire).`

Responsibilities:
- Stream **internet radio** over WiFi (only here).
- Receive UART commands from the panel (station, volume, **volume-ducking** when voice wake word fires).
- Act as **gateway to the AC** (Section 8).
- Sound the **alarm** (including wake-to-radio).

---

## 7. Voice control (offline, on the panel)

**Feasible, fully offline, zero RF** — runs on hardware already owned.

- **Hardware:** one **INMP441 I2S digital microphone** on the panel *(owned)*. (Note: it's an **I2S** mic — a digital audio bus — not I2C. An analog electret mic was rejected as too noisy near the motors/amp.) Six wires to the panel: 3.3V, GND, SCK, WS, SD, and L/R tied to GND.
- **Software:** Espressif **ESP-SR**, which needs the ESP32-**S3** + PSRAM — the ESP32-8048S043 panel has an S3 with **8MB PSRAM**, so it qualifies. Two engines:
  - **WakeNet** — always-listening wake-word detector (light, robust).
  - **MultiNet** — after wake, matches the next few seconds against a **predefined list** of command phrases (a few dozen is the sweet spot). Includes an **AFE** front-end with noise suppression.
- **Language:** MultiNet officially supports **English and Chinese, not Hebrew.** Plan: define commands in **simple English** (reliable, accent-tolerant); try **phonetic-Hebrew** phrases as a bonus and keep whichever prove reliable.
- **Targeting:** one mic on the central panel. **Default commands act on the speaker's own bed by verbal choice** — e.g. plain "massage off" vs. an "Ira bed …" prefix for the other side. Note the system **cannot identify who is speaking** — targeting lives entirely in the words.
- **Noise handling (bedroom is acoustically hostile — motors + 120W amp):** (1) mic aimed at the face, far from speakers; (2) **volume ducking** — the instant WakeNet fires, panel tells the audio node to drop volume ~3s (and optionally soften motors); (3) AFE noise suppression, with the option to add a second INMP441 later for beam-forming.
- **Build order:** voice comes **after** the touch UI works, so every voice failure has a graceful touch fallback. WakeNet first (real-bedroom wake test), then grow MultiNet's command list.
- **Wake word:** not yet chosen — should be 2–3 syllables, uncommon in normal conversation.

---

## 8. Air-conditioning control

**Unit: Switcher Breeze** (Israeli, ~₪159). An IR-blaster smart controller for any AC type, **including ceiling / mini-central units**.

Why it fits:
- **Local control** — the Switcher protocol is fully reverse-engineered (open-source **aioswitcher**, the library behind Home Assistant's Switcher support). Control is plain **TCP on the home LAN**, no cloud tokens, no internet dependency. Initial setup (choosing the AC's remote profile) happens once via Switcher's phone app; afterward the phone is out of the loop.
- **Covers the full wish list:** power on/off, mode (cool/heat), target temperature, fan speed. It also reads the original remote's actions to keep state in sync. **Light/LED control is AC-dependent** — verify against the specific AC brand's IR set.

Architecture (AC lives across the room = RF principle intact):
```
Panel (touch/voice) -> UART -> Audio node -> TCP over home WiFi -> Switcher Breeze -> IR -> A/C
```
Implementation note: aioswitcher is Python; we'll **port the needed slice** (packet structure, signing, command encoding) to the audio node's Arduino C++.

**Multiple ACs:** one Breeze = one AC. A wall unit **and** a ceiling unit = **two Breeze devices** (₪159 each); the audio node holds two IPs and the AC tile gets an A/B selector. Ceiling units need clear **line-of-sight** to the AC's IR receiver eye — confirm the original remote works from across the room (proves a live receiver with range).
**Plan B:** a ₪15 IR LED on the audio node can *be* the remote directly, independent of the Breeze, if ever needed.

**AC brand/model still to be supplied** — needed to verify fan-speed granularity and light command.

---

## 9. Rejected decisions (do not re-propose without new reason)

| Rejected | Why |
|---|---|
| **DRV8833 driver** | 10.8V max — can't handle the 12V motor rail. L298N (46V) used instead. |
| **Second Waveshare panel** | Superseded by one central panel serving both beds. |
| **Cordless panel** | Would require radio at the headboard (breaks the RF principle) **and** a 4.3" backlit screen drains batteries fast (dead at 2 AM). Kept wired with a slim cable + slack instead. |
| **Standalone Bluetooth speaker** | Loses all panel control of audio (no voice "radio on", no ducking, no alarm sound), makes the phone a permanent in-bed component, wastes the owned amp. |
| **ESP32 as Bluetooth *source* while also doing WiFi radio** | WiFi + BT share one 2.4GHz radio; continuous streaming on both is the flakiest case (dropouts). If ever pursued, use two ESP32s. |
| **Phone → Bluetooth → audio node (receiver)** | Examined; the wired chain wins on quality, reliability, and alarm behavior. Staying fully wired. |
| **Arduino Mega + ESP8266 sidecar** | Awkward two-chip architecture. |
| **Guition JC4880P443C (ESP32-P4)** | Immature ESP-NOW; MIPI-DSI display incompatible with TFT_eSPI. (NOTE: this is a *different* board from the accepted ESP32-8048S043 panel, which uses a parallel-RGB ST7262 display and is well supported — see Section 5. Don't confuse the two.) |
| **Battery-powered CYD remote** | Superseded by the fixed, mains-powered panel. |
| **Nextion display** | Waveshare already owned; LVGL richer for radio/clock UI. Not pursued unless direction changes. |
| **Panel-to-panel messaging** | Moot with a single central panel. |

---

## 10. Hardware status

**Owned:** **Guition/JC ESP32-8048S043 panel (×1) — THE PANEL** · Waveshare ESP32-S3-Touch-LCD-4.3 (×1, now spare/LVGL practice) · QuinLED-ESP32 (×1) · classic ESP32 WROOM-32 (×1, audio node) · XH-M543 (TPA3116D2) amplifier · Wemos D1 Mini ESP8266 (spare, no assigned role) · 8 × 12V ERM motors (mix of 0.3A large and 0.1A small) · **BH1750 light sensor** · **SHT31 temp/humidity sensor** · INMP441 I2S microphone · at least one L298N (bench-tested).

**Still to acquire:** L298N modules to reach ×4 per bed (up to 8 total) · 2nd QuinLED-ESP32 (for bed #2) · 12V/5A PSU (×2) · buck converters (12→5V) · PCM5102A DAC (if not already owned) · passive speakers · Switcher Breeze (×1, or ×2 if controlling a ceiling AC too) · PIR sensor · possibly a 2nd INMP441 later · cabling (shielded twisted pair for UART, speaker wire).

---

## 11. Build roadmap

Each stage produces something that works on its own.

1. **Stage 1 — First bed box on the bench. ✅ COMPLETE.** QuinLED + one L298N + one large & one small motor proven: firmware compiles/uploads via PlatformIO, LEDC PWM on both bridges, L298N drives real motors, 4-byte protocol, kick-start, flash save/resume, auto-demo, and calibrated per-motor thresholds giving a smooth low end (both motors start at intensity 1). Current build: TEST 006.
2. **Stage 2 — Full bed box.** All 4 L298Ns, all 8 motors, flash save/resume, session timer, presets. Install into bed #1.
3. **Stage 3 — Panel.** LVGL UI (home, massage + bed selector, presets, timer), connected to the bed box over the real UART cable. Screen state machine + sensors.
4. **Stage 4 — Audio node.** Internet radio, station list, volume, alarm-to-radio; commanded from the panel.
5. **Stage 5 — Voice.** INMP441 + WakeNet (real-bedroom wake test), then MultiNet commands, ducking via the audio node.
6. **Stage 6 — AC + second bed.** Port the Switcher protocol, add the AC tile; then bed box #2 is a copy of Stage 2.

Two purchases gate progress: **L298N modules + any additional motors** gate Stage 1–2 (needed now); the **second QuinLED** gates only Stage 6 (no rush).

---

## 12. Current status & immediate next action

- **Stage 1 is complete.** The current bed-box build is **`TEST 007`** (`bedbox_test007.cpp`): two-motor auto-demo + manual console, core 2.x PWM, calibrated thresholds. In TEST 007 the bench motors are mapped channel 0 = BIG (`MIN_DUTY_BIG=165`), channel 1 = small (`MIN_DUTY_SMALL=60`) to match how they're physically wired; both start and hold a gentle buzz at intensity 1. (In the real bed box the canonical zone map applies — Section 3.)
- **Panel decided and working:** the **ESP32-8048S043** is the panel. Confirmed on hardware: display (TEST 008), calibrated touch (TEST 010, four corners pass), and welcome photo from SD at boot (TEST 011, current panel build). Sensors owned and staged (BH1750 light + SHT31 temp/humidity, both I2C) but **not yet wired** — headers still to be soldered on.
- **Panel toolchain:** separate PlatformIO project "Panel", board `esp32-s3-devkitc-1`, `platformio.ini` = **v3** (GFX Library 1.3.8 + JPEGDEC, `qio_opi` 16MB/8MB, `-DARDUINO_USB_CDC_ON_BOOT=0`, `monitor_speed=115200`). Bed-box project "BedBox" is separate (board `esp32dev`, core 2.x LEDC).
- **Bed-box toolchain:** VS Code + PlatformIO, `esp32dev`, `monitor_speed=115200` + `monitor_echo=yes` + `monitor_filters=send_on_enter`. If an upload fails with "invalid head of packet / serial noise," close the Serial Monitor (it holds the port) and retry; holding BOOT during upload also helps.
- **Immediate next step — TEST 012, choose one:** (a) **I2C sensors** — solder headers onto BH1750 + SHT31, wire to the panel's I2C bus (shared with GT911 at SDA=19/SCL=20), show live lux + temp + humidity on screen; or (b) **first panel↔bed-box UART link** — wire panel TX/RX to the QuinLED and have a screen tap send a real 4-byte massage command to the motors (the two halves talking for the first time). After these, move to **LVGL** for the real home/massage UI. Stage 2 (all four L298Ns / 8 motors into bed #1) proceeds when the remaining L298N modules are on hand.

---

## 13. Open questions to resolve next

1. **Panel pin map (ESP32-8048S043)** — assign: UART to bed box (P1: GND/RX/TX/+5V, or P3 IO17/IO18); BH1750 + SHT31 on the I2C bus (board I2C is SDA=IO19/SCL=IO20, shared with GT911 touch); INMP441 I2S (SCK/WS/SD on free GPIOs); PIR on a free GPIO. Confirm against the board's free-header list; many pins are taken by the RGB display. **Light sensor resolved: BH1750 (digital, lux) replaces the LDR — no ADC pin needed.**
2. **Motor thresholds** — measured and baked in (small 55, big 165–180 cold-start; running minimums 60 / 165). Re-verify per motor when scaling to all 8 in Stage 2.
3. **Wake word + command phrase list** (English core, Hebrew phonetic experiments).
4. **AC brand/model(s)** — to verify Switcher fan-speed granularity and light command; and whether a ceiling unit needs a second Breeze.
5. **Welcome photo** — DONE: photo of the couple by the sea, cropped/mirrored to 800×480 (`welcome_800x480.jpg`), shown from the SD card at boot in TEST 011.
6. Whether to grow back from 8 to 10 motors (5th zone) — hardware/protocol slots (GPIO 26/27) are reserved.

---

## 14. Reference material on file

- **L298N reference sheet** (STMicroelectronics DS0218) — pinout, logic table, heatsinking, module notes.
- **DRV8833 reference sheet** (TI SLVSAR1E) — kept for the record; driver itself rejected (10.8V max).
- **QuinLED-ESP32 board details** — official pinout (4×9 layout), 750mA PTC fuse, U.FL antenna. Source: quinled.info/quinled-esp32-board-details/
- **Switcher protocol** — open-source `aioswitcher` library documents the Breeze packet format / LAN control.
- **ESP-SR** — Espressif's offline speech framework (WakeNet + MultiNet + AFE) for the ESP32-S3.

---

*End of handout — Revision 4. This document is intended to be complete on its own; a new conversation can begin from here.*
