# Adjustable Bed Massage Retrofit — Project Handout
**Revision 4 — single central panel, fully wired**
Status as of this session. Self-contained: this document alone should let any future session pick up without re-learning the project.

---

## 1. Goal

A DIY smart bedside system for two adjustable beds (two people, side by side). One shared touchscreen panel controls vibration massage on both beds, plus internet radio, a clock face with environmental sensors, and alarms.

---

## 2. Governing principles (locked)

1. **Zero RF at the heads.** No WiFi, no Bluetooth, no ESP-NOW anywhere near the sleeping positions. All inter-device communication is wired UART. Only the audio module has a radio, and it lives away from both heads.
2. **Night quiet.** When the screen goes dark, the panel only watches its PIR — a passive sensor, zero emission.
3. **Each person controls their own bed by default**, with explicit selection on a shared screen.
4. **Bed boxes are autonomous.** They remember their last massage setting and session timer in flash and resume on power-on, so a screen failure never leaves a bed uncontrollable.
5. **Test discipline.** Every change bumps `TEST_NUMBER`. No exceptions.

---

## 3. Architecture — Revision 4

### What changed from Rev 3
Rev 3 had **two** panels, one at each head, linked to each other over UART. Rev 4 replaces them with **one panel mounted in the middle of the headboard**, serving both beds.

This deletes: the second screen, the second PIR/AHT20/LDR set, the panel-to-panel cable run, and all state-mirroring logic between panels. One brain, one source of truth.

### Devices

| Device | Qty | Hardware | Role |
|---|---|---|---|
| Central panel | 1 | Waveshare ESP32-S3-Touch-LCD-4.3" (800×480 RGB, GT911 capacitive touch) | All UI, clock, sensors, command origin |
| Bed box | 2 | ESP32 + 5× L298N modules + 12V PSU (6–7A) | Drives 10 vibration motors per bed |
| Audio module | 1 | ESP32 + PCM5102A I²S DAC + TPA3116 class-D amp + 2 speakers | Internet radio; **the only radio in the system** |
| Remote (optional) | 2 | Small wired button pad / encoder | Per-pillow massage control — see §9 open item |

### Sensors — all on the central panel
- **LDR** (GPIO6) — ambient light, drives clock brightness
- **HC-SR501 PIR** — motion wake
- **AHT20 or SHT31** (I²C) — temperature + humidity. **Mount away from the display board** — the panel electronics self-heat a degree or two and a sensor glued behind the screen reads the panel, not the room.

### Links

| Link | Bus | Notes |
|---|---|---|
| Panel → both bed boxes | **UART1**, shared multi-drop | Both boxes sit on one line; the `bedId` byte addresses them. Replies on request only, so no collisions. |
| Panel ↔ audio module | **UART2** | Bidirectional — commands down, now-playing text back |
| Panel → USB | UART0 | Debug / serial monitor only, 115200 |

Two cable runs leave the headboard, both from the same central point. Twisted/shielded pair, 3 wires each (TX/RX/GND), routed alongside the existing DC cabling.

---

## 4. Motors

- 10 per bed, 12V ERM vibration type, single direction.
- Grouped in 5 zones of 2 motors: **Shoulders, Upper back, Lower back, Thighs, Calves**.
- Driven via L298N: PWM on ENA/ENB from the ESP32's LEDC peripheral; IN1 (or IN3) held HIGH, IN2 (or IN4) LOW for fixed direction.
- Common ground between ESP32 and every L298N module — mandatory.
- Remove the L298N module's onboard 5V regulator jumper if VS exceeds 12V.

> **Note on the reference sheets in this project:** `L298N_Reference_Sheet.pdf` and `DRV8833_Reference_Sheet.pdf` were written when the plan assumed **24V** motors. The design is now **12V**. The DRV8833 was rejected regardless — its 10.8V maximum supply voltage is below our rail. L298N stands.

---

## 5. Command protocol

4-byte message: `{bedId, cmd, target, value}`

| cmd | Name | target | value |
|---|---|---|---|
| 0 | OFF | — | — |
| 1 | ALL | — | 0–100 intensity |
| 2 | ZONE | 0–4 (Shoulders…Calves) | 0–100 |
| 3 | MOTOR | 0–9 | 0–100 |
| 4 | PRESET | 0 Wave / 1 Pulse / 2 Ripple | — |
| 5 | TIMER | — | minutes |

Framed over serial (was ESP-NOW in Test 1 — needs refactoring).

---

## 6. Panel UI

**State machine:**
`Welcome` → `Home` → *(idle ~2 min)* → `Clock` → *(no motion ~10 min)* → `Dark` → *(PIR motion)* → `Clock lights` → *(touch)* → `Home`

**Screens:**
- **Welcome** — boot photo (placeholder until a photo is supplied)
- **Home** — tiles: Massage, Radio, AC *(AC deferred)*
- **Massage** — modes [All / Zones / Motors / Presets], timer 5–60 min, STOP
- **Radio** — genre-colored tiles (Greek/Classical/Rock/Blues/Jazz/Israeli/News/Chill), presets, Radio Browser search, volume, sleep timer
- **Clock** — huge digits (~6–7cm, 120pt+ LVGL font) + temperature + humidity, brightness follows the LDR

**Bed selection** is now a permanent, unmissable element — on a shared screen every massage command must answer *whose bed* before it means anything. Open decision on the exact form (§9).

**Night behavior:** clock stays visible but very dim; PIR wake brings it to full interface. Chosen over full-dark.

---

## 7. Code conventions (strict)

- C++ / PlatformIO / VS Code. **Not** Visual Studio.
- Keep project paths free of Hebrew characters — the toolchain chokes on non-Latin paths. `C:\Projects\BedSystem\` is safe.
- Projects: `panel/`, `main-board/`, `remote/` — each opened as its own folder in VS Code.
- `#define TEST_NUMBER n` is the first line of `panel/include/config.h` and `main-board/src/main.cpp`. Bumped on **every** change. Shown top-right on screen, printed first at boot on serial.
- Every deliverable: complete files presented individually for copying, plus `bed_system_testN.zip`, plus this handout updated when the feature set changes.
- All major features gated by on/off switches in `config.h` so disabled modules compile silently: `ENABLE_CLOCK_FACE`, `ENABLE_TEMP_HUMIDITY`, `ENABLE_PIR`, etc.
- Serial monitor: 115200.

**Bring-up stages:** 1 massage only → 2 clock + sensors → 3 radio + audio → 4 alarms → 5+ AC, enhancements.

---

## 8. Status

**Done**
- Rev 4 architecture settled (single central panel, wired, RF-free)
- UI mockups (massage, radio)
- Wiring diagrams (system overview + bed box detail)
- **Test 1 firmware written** — compiles, untested on hardware. Contains: display/touch/LVGL glue, massage UI, comfort module, motor driver, command handling, flash state, session timer. Still written against ESP-NOW and the two-panel model.

**Not yet written**
- **Test 2** — refactor to wired UART + single-panel model; state machine; big-digit clock; sensor reading
- Test 3 — audio module firmware
- Test 4 — alarms
- Test 5 — AC IR control (needs AC brand)

**Hardware on hand:** L298N modules, 12V motors, Arduino Mega (unused in current design), possibly test ESP32s. Waveshare panel in transit.

**Still to buy:** HC-SR501 PIR ×1, AHT20/SHT31 ×1, audio module parts, UART cable runs. *(Rev 4 halved this list — no second panel or second sensor set.)*

**Immediate next step:** install VS Code + PlatformIO, open `main-board/`, build. Report green SUCCESS or the first red error.

---

## 9. Open decisions

1. **Reach.** A center screen sits 60–80cm to the side of each head. Fine for setting an alarm at 22:00; bad at 2am and bad for adjusting a massage while lying down with eyes closed. The `remote/` project exists for exactly this — a small wired pad per pillow, ~$5 a side, on the same bus, zero RF. **Undecided.**
2. **Bed selection UI** — toggle at top (one bed at a time) vs. split view (both beds always visible). 800×480 supports either.
3. **Messaging feature** — largely obsolete on a shared screen. Drop, or repurpose as shared notes on the clock face.
4. **Alarms** — one screen and one speaker pair means an alarm wakes both people. Two genuinely independent wake times aren't possible unless one is silent/visual only.
5. **PIR identity** — one central sensor can't tell who moved. Fine for waking the clock, useless for anything person-specific.
6. **Bed box controller board** — plain ESP32 assumed; the QuinLED-ESP32 (schematic in project files) is a candidate for its screw terminals.
7. Welcome-screen photo, and the AC brand for IR control.

---

*End of handout — Revision 4.*
