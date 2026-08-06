# Adjustable Bed Massage Retrofit — Project Handout

**Revision 11 — 6 August 2026**
Supersedes Rev 10 (5 August). Self-contained: this document alone should let a
future session pick up without re-learning the project.

---

## 1. Goal

A DIY smart bedside system for Shemtov (Shemi) in Petah Tikva, Israel. One
central touch panel on the headboard controls vibration massage, a clock face
with environmental sensors, voice control, internet radio, air conditioning,
lighting, shades and a ceiling fan.

**Governing rule: zero RF at the heads.** No WiFi, Bluetooth or ESP-NOW near
the sleeping positions. All inter-device communication is wired UART. Only the
audio node has a radio, and it lives away from both heads.

**NEW IN REV 11 — the massage is for ONE bed.** Ira was asked and does not want
it. Bed box 2 is cancelled, and with it the whole bed-selector idea. Every
massage command goes to bed 1. The rest of the room — radio, air conditioning,
light, shades, fan — is still shared, because it always was.

`buildBedSelector()` is still in the panel source but is no longer called, so
that a change of mind is one line and not a rewrite.

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
│   └── src/main.cpp      TEST 078
├── bedbox/     platformio.ini, src/main.cpp (TEST 012)
├── audionode/  platformio.ini, src/main.cpp (TEST 004)
├── switcher/   switcher_discover.py, breeze_control.py
├── readme.md   STALE - describes TEST 041 and demands an lv_conf.h that must
│               not exist. Delete it or replace it with a pointer to this file.
├── .gitignore
└── Handout.md
```

**Conventions**
- The code file is always `src/main.cpp`. Filenames never carry versions.
- **The test number appears in the FIRST three lines and the LAST three lines
  of every file**, and in `#define TEST_NUMBER n`. Bumped on every change.
- Every deliverable is the **whole file**, ready to copy and paste. Never a
  diff, never "change this one line".
- Version numbers go in commit messages, not filenames.
- `.pio/` is never committed.
- Each project folder is opened separately in VS Code — never the repo root.
  Opening the repo root is why PlatformIO once appeared not to load at all.
- **Headers go in `include/`, not the project root.**
- In the VS Code explorer, `M` beside a filename is **git status, not save
  status**. It means modified since the last commit and stays until you commit.
  Unsaved is a filled dot in the editor tab, not a letter.

---

## 3. Panel — ESP32-8048S043

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
| DS3231 real-time clock | 0x68 — battery-backed, now holds the DATE too |
| BMP280 | 0x76 — pressure only |
| INMP441 microphone (I2S0) | DIN=IO11, BCK=IO12, WS=IO13 |
| SD card | **DISABLED** — shares the microphone's pins |
| Backlight | GPIO2, LEDC ch7 |
| UART1 to the bed box | TX=IO17, RX=IO18 @ 115200 — **now two-way** |

### platformio.ini — do not change casually

`platform = espressif32 @ 6.9.0` is mandatory here. Both `board_build.*` and
`board_upload.*` flash-size lines are mandatory. `-DLV_CONF_SKIP` means **no
`lv_conf.h` anywhere**. `-DLV_MEM_SIZE=65536` is required or the Files tab
crashes at boot. Libraries: GFX 1.3.8, JPEGDEC, LVGL ^8.3.11.

### Firmware history, TEST 074 → 078

| Test | Change |
|---|---|
| 074 | Room temperature moved right of the + on the AC screen |
| 075 | **Massage screen rebuilt** — see below |
| 076 | Settings, Clock tab: the DATE can be set, not just the time |
| 077 | **The sliders follow the running pattern**, fed up the wire |
| 078 | Clock tab spacing — the date row no longer sits on the time |

### The massage screen as it now stands

- **No bed selector, no demo.** Everything goes to bed 1.
- **Top right:** the time in large digits, then the date, then the temperature
  with a small `c`.
- **Twelve mode tiles**, four across and three down, on the left. Colour is by
  family, not by sequence: green travels along the body, purple crosses it,
  orange is rhythm and atmosphere. The running mode is lit and outlined.
- **Four upright zone sliders** down the right side, numbered 1 to 4 — head,
  upper back, lower back, legs. **The Hebrew words under them were removed at
  Shemi's request; at 30 px wide they were unreadable.**
- **Bottom row:** 15, 30, 60 and a red כיבוי.

Eleven tiles map to bed box patterns 0–10. The twelfth, "מלא", is not a pattern
at all — it is `CMD_ALL` at 100, so it needed no bed box change.

### Hebrew on the panel

LVGL is built with `LV_CONF_SKIP`, so bidirectional text is off and **every
Hebrew string is stored PRE-REVERSED**, exactly as `font_hebrew.h` already does
for `HEB_BEDROOM` and `HEB_MAZGAN`. TEST 075 added a table of pre-reversed
words at the top of `buildMassage()`.

**Open risk:** it is not known which glyphs were included when `font_hebrew.h`
was generated. If a word renders as empty boxes, a letter is missing — most
likely a final form such as ם. The fix is either a different word or a
regenerated font.

---

## 4. Bed box — WORKING AT LAST

**QuinLED-ESP32, six 12V ERM motors, three L298N modules.**

### The new motor layout

| Motor | Pin | Zone | Size | Driver |
|---|---|---|---|---|
| m0 | 13 | Head left | small | L298N 1 |
| m1 | 14 | Head right | small | L298N 1 |
| m2 | 18 | Upper back | **BIG** | L298N 2 |
| m3 | 19 | Lower back | **BIG** | L298N 2 |
| m4 | 21 | Leg left | small | L298N 3 |
| m5 | 22 | Leg right | small | L298N 3 |

Head and legs are left/right pairs. The two spine motors sit alone on the
centre line and are the big ones. Zones for the protocol are
`{0,0,1,2,3,3}` — head, upper back, lower back, legs.

`MIN_DUTY_BIG` 165, `MIN_DUTY_SMALL` 60, both measured with `calib`. PWM
20 kHz, 8 bit, 60 ms kick-start on direct commands only.

### Wiring, per L298N

12V to `VS`. `GND` to the supply **and** to the QuinLED — common ground is
mandatory. `IN1`/`IN3` to GND, `IN2`/`IN4` to that module's own `5V`. Only
`ENA`/`ENB` go to the ESP32. **The onboard `ENA`/`ENB` jumpers must be
removed** or the motors sit at full speed and the PWM does nothing — this is
the single most common failure with these boards. Keep the 5V regulator jumper
at 12V input. Never join one module's 5V to another's, or to a buck converter.

Panel link: `IO16` = RX, `IO17` = TX, crossed, ground shared, **5V never
connected**.

### Firmware history, TEST 007 → 012

| Test | Change |
|---|---|
| 007 | The old two-motor demo. Did not listen to the panel at all |
| 008 | UART receiver, pattern engine, six motors, resume-on-boot connected |
| 009 | Builds on Arduino core 2.x **and** 3.x |
| 010 | Renamed constants that collided with Arduino macros |
| 011 | Boot self-test, CR/LF tolerant console |
| 012 | **Reports zone levels back up the wire** |

### The pattern engine

`tickEngine()` runs every 20 ms. Patterns never write PWM — they set a target
per motor, and each motor crawls toward it at 4 level-units per tick, reaching
full in about half a second. **That cross-fade is what makes it feel like
movement instead of switching**, and it removes the need for a kick-start
inside a pattern because the ramp crosses the break-loose threshold by itself.

`FLOOR_LEVEL` at the top of the file is 0. Raise it to 4–6 if starts feel
abrupt.

Eleven patterns: Waterfall, Rise, Rock, Diagonal, Side, Circle, Knead, Pulse,
Breathe, Rain, Shuffle. `list` on the console prints them with their numbers.

### The uplink — new in TEST 012

Whenever a zone's level changes by 2 or more, and at most every 150 ms, the box
sends `{bedId, 20, zone, level}` up the wire. The panel listens and moves its
sliders. **This is the first two-way traffic in the whole system.** Command 20
is well above the downlink numbers so an echo can never look like a command.

### Console

`motor M N | zone Z N | all N | preset P [N] | timer MIN | off | demo |
calib M | status | link | list`

`link` prints how many frames have arrived from the panel. Zero means wiring.

### BOOT_SELFTEST

`#define BOOT_SELFTEST 1` at the top makes the box sweep every motor on
power-up and repeat until a key is pressed. It exists to prove the motors
without depending on the console. **Set it to 0 for normal use** — while it
runs it overwrites anything the panel sends.

---

## 5. Audio node — blocked on hardware

TEST 004 works: internet radio and a web control page, proven on a spare
ESP32-CAM. SomaFM connected, MP3 decoded at 44.1 kHz stereo, sound out of
headphones. **The ESP32-CAM is abandoned** — brownout with WiFi and I2S
together, every free pin is a strapping or SD pin, one channel only.

**Shemi has no spare ESP32 at all**, so the radio cannot be moved forward until
a board arrives. This is the only thing blocking the radio.

**The W5500 module HAS arrived.** It is an AMICCOM-free plain SPI Ethernet
board with a HanRun HR911105A jack. Important correction to an earlier
assumption: **the W5500 works fine** — Arduino core 3.x has a native `ETH.h`
driver (`ETH_PHY_W5500`) that registers it as an ordinary lwIP interface and
bypasses its hardwired TCP/IP stack, so `WiFiClient` routes over the cable
unchanged. Only `WiFi.begin()` → `ETH.begin()` and the event name change.

Pin cost: SCK, MISO, MOSI, CS, IRQ. RST can be −1. SPI clock about 12 MHz on a
classic ESP32, 36 MHz on an S3.

The WT32-ETH01 has no USB — it needs an external USB-serial adapter and IO0 to
ground for flashing. Many people think the board is dead at this point.

**Library pin, unchanged and mandatory:** `ESP32-audioI2S` at `#3.4.0`. 3.4.7
calls `dsps_biquad_sf32()` and fails to build, and 3.4.0 uses the OLD
free-function callback style. `monitor_rts = 0` and `monitor_dtr = 0`.

DAC wiring that worked: VIN 5V, GND, **SCK to GND (mandatory)**, BCK GPIO14,
LCK GPIO2, DIN GPIO13.

**Station URLs** from `data-player-hls-src` in the kan.org.il page source:
Kan 88 is 4504, Kan Gimel 4490, Reshet Bet 4483, Kol HaMusica 4518.
`https://kancdn.medonecdn.net/livehls/oil/kancdn-live/live/radio/kan_88/live.livx/playlist.m3u8`

**Station index mismatch, still unfixed:** the panel's `RADIO[8]` and the audio
node's `stations[5]` are in different orders and different lengths. Panel index
0 would play SomaFM. Galgalatz, Eco 99, Reshet Bet and Kol HaMusica have no URL
anywhere yet.

---

## 6. Air conditioning — Switcher Breeze

Panel → UART → audio node → WiFi → Breeze → IR → Tadiran. Discovered and
working from Python: device_id `0998b7`, key `02`, ip `192.168.1.122`,
remote_id `YACIBI00`, no token.

Three things learned the hard way: **a target temperature of 0 is normal** for
this Breeze; **the Breeze often acts without replying**, so every call needs a
timeout; **swing is on/off only**. Consequence: the AC's target temperature can
only ever be shown as what was last commanded.

Still to do: port the Breeze protocol to C++ for the audio node.

---

## 7. Remote controls — investigated in depth this session

### The ceiling fan (Westinghouse) — 433 MHz, straightforward

Board `SQ19D-F20(ZPT)_V1.0`. A crystal, three inductors and a PCB trace
antenna, and **no infrared LED anywhere** — so it is radio, not IR.

**Unknown: 433 or 315 MHz.** Read the marking on crystal `Y1`; multiply by 32
for the rough carrier. 13.56 means 433.92, 9.84 means 315.

Plan: sniff the codes with a receiver and RCSwitch, then transmit them from an
FS1000A on the audio node. **Buy a superheterodyne receiver** (RXB6, RXB12,
SRX882) — the cheap super-regenerative XY-MK-5V that ships in the kits usually
fails to decode remotes and floods you with noise.

**Ordered: 433 and 315 modules, two of each.** Nothing is blocked here.

### The bed motors — 2.4 GHz, a genuine reverse-engineering project

Control box `CBG4-X1-3-0`, type `CBG4 (9080577)`, sold by Aminach; the actual
manufacturer could not be identified from any search.

- **29V DC** in and out. Three motors: M1 max 1.5A, M2 and M3 max 3.0A.
- A "USB port" rated **13V / 2A** — do not plug a phone into it.
- **Duty cycle 10%, max 2 minutes on / 18 minutes off.** Any automation must
  enforce this in software; the remote does not.

Remote `TXG-9-3`, type `TXG (9080584)`, **2.4 GHz**, 3×AAA. Board `TXGB E2.1`,
main MCU custom-marked `TXGBA02`. The radio is an **AMICCOM A7105** on module
`MD7105-A04-08` with a 16 MHz crystal.

**This is better news than it looks.** The A7105 datasheet and full register map
are public, cheap A7105 modules (XL7105) are available, and the RC hobby world
has already reverse-engineered several A7105 protocols.

Route: put a logic analyser on the module's `CN1` pads and record the SPI
between the MCU and the radio while pressing buttons. That reveals the ID, the
channel, the data rate and the packet. Then replay from our own A7105.

Related prior art: `github.com/zerog2k/nrf24_bed_remote_hacking` — OKIN/JLDK
beds on nRF24, and the key finding that emulating a remote through the pairing
sequence is straightforward. Most Home Assistant bed integrations
(`smartbed-mqtt`, `ha-adjustable-bed`) are Bluetooth and do not apply here.

**Ordered: 8-channel USB logic analyser and test hook clips.** Still to order:
an A7105 / XL7105 module.

**Three safety rules, not negotiable:** the original remote stays in the room
and untouched; only press-and-hold moves a motor, never a fire-and-forget
button; and a hard time limit in code, on top of the duty cycle.

### Lights and shades — 433 MHz, decided in Rev 10, unchanged

FS1000A on the audio node, a 2-channel receiver for the shades in latching +
stop, a 1-channel for the light. 1527 encoding via RCSwitch, so we choose the
codes rather than cloning them.

---

## 8. Protocol

Four bytes, `{bedId, cmd, target, value}`.

**Downlink — panel to bed box (id 1) and audio node (id 9)**

| cmd | Meaning |
|---|---|
| 0 | OFF |
| 1 | ALL, value = intensity |
| 2 | ZONE, target 0–3, value = intensity |
| 3 | MOTOR, target 0–5 |
| 4 | PRESET, target 0–10 |
| 5 | TIMER, value = minutes |
| 6–9 | RADIO play / stop / volume / sleep |
| 10–14 | AC power, temperature, mode, fan, swing |
| 15 | RF — target 0 shade, 1 light |

**Uplink — bed box to panel**

| cmd | Meaning |
|---|---|
| 20 | LEVEL, target = zone 0–3, value = level 0–100 |

---

## 9. Power — decided this session

**One 12V industrial supply, 9–10A, feeds everything.** A separate 5V supply
was considered and rejected: ground must be shared anyway for the PWM to have a
reference, so a second supply buys another mains brick and another failure
point without the isolation it appears to offer.

**Distribute 12V, step down to 5V locally** — a small buck beside the bed box,
another beside the panel, another beside the audio node. A single 5V rail
dragged across the room loses enough voltage in the cable to make an ESP32
misbehave; the same loss on 12V is negligible.

**Star wiring at the supply terminals:** one pair of wires to the motors,
another to the bucks, meeting only at the screws. This matters more than any
extra supply.

**1000 µF electrolytic on each L298N's 12V input**, close to the terminal, to
absorb the kick-start surge.

**A fuse on the motor branch.** A 10A supply will happily push 10A into a
shorted motor lead under the mattress.

---

## 10. EMI — decided this session

In order of effectiveness:

1. **0.01 µF ceramic across each motor's terminals**, soldered right at the
   motor. This shorts the brush sparks where they are made and beats everything
   else on this list.
2. **Twist the motor pair** — one turn every 2–3 cm. Free, and it closes the
   radiating loop better than copper does.
3. **Ferrite ring on the pair, both wires through the same ring**, close to the
   **motor**, not to the driver. Two passes through the ring if it will fit.
   A single wire through a ring does something quite different and much less
   useful.
4. Shielded motor cable if wanted, grounded at the driver end only.

**A copper-taped enclosure around the bed box was considered and advised
against.** Radiation comes from the wires, not the boards, and twelve wires
leave that box. Shielding only works when it encloses the whole volume; the
cables carry the noise straight out through the wall of it. Use a box for
safety and dust, not for EMI.

The 20 kHz PWM already in the firmware does more than any shielding — it keeps
the switching noise out of the audio band by preventing it, not blocking it.

---

## 11. Motors and the mattress — an unresolved risk

**Shemi's mattress is latex and viscoelastic.** This is the worst possible
material combination for vibration transmission. The Polyurethane Foam
Association describes viscoelastic foam as low-resilience with a ball rebound
below 20% against 50–60% for ordinary flexible polyurethane, and lists damping
vibration as a defining characteristic. Latex is the opposite — high resilience,
transmits motion well.

**Which layer is on top matters enormously and is not yet known.**

And the power numbers are not encouraging. Shemi's big motor is 3.6 W where a
commercial bed massage motor is 10–24 W, and commercial bases with far stronger
motors already lose most of their output to ordinary foam.

**THE TEST THAT SETTLES IT, NOT YET DONE:** tape one big motor to a 15 cm square
of plywood, put it on the platform under the mattress at shoulder height, wire
it straight to 12V at full power, and lie on it. Clearly felt means proceed.
Barely felt means no amount of clever mounting will rescue it, and the answer is
either much stronger motors (10 W, 3800 RPM, 12V — eight of them is 80 W, which
the supply covers) or moving the motors into a thin overlay on top of the
mattress.

**Mounting, from the Leggett & Platt patent (US 7322058):** rigid mounting to
the underside of the platform board is the design the industry moved away from,
because the whole board has to shake and the foam then absorbs it. Their patent
suspends the motor from a semi-flexible plate that rests on the foam pad and
hangs through a hole in the platform, deliberately not fastened to it, and warns
that any contact between plate and platform produces a distracting buzz. The pad
they specify is about 25 mm of polyurethane foam, roughly 29 kg/m³, medium
firmness. **Never memory foam anywhere in this path.**

---

## 12. Snoring and apnea — researched, designed, not built

**Head elevation works, moderately.** Around a 30% AHI reduction across several
studies (Souza 2017 at 7.5°, Iannella 2022 at 30°, a 2025 multicentre study).
Side-sleeping beat elevation in the 2025 study.

**Vibration to make someone roll off their back is well proven** — a Thorax
meta-analysis of 18 studies found −9.19 AHI and −32.8 percentage points of
supine time. **Vibration to end an apnea once started is real research but
almost entirely in premature infants**, using 200–300 Hz targeted at Pacinian
corpuscles. Shemi's ERM motors run at 50–66 Hz, nowhere near that band.

**Acoustic detection is feasible.** A JCSM study with a phone on the chest got
r=0.93 for snore time and r=0.94 for RDI against polysomnography. The mechanism
is not silence — it is the pattern: regular snoring, an abnormally long gap, and
a loud recovery gasp.

**The design agreed:** a dedicated wired listening node near the heads with two
or four INMP441 microphones — **two share one I2S bus using the L/R select pin,
costing no extra GPIOs** — an SD card, and UART to the panel at a new address 8.
Band energy 100–600 Hz, 25 ms frames, the existing adaptive noise floor, channel
comparison to attribute a snore to a person, and a nightly summary of a few
hundred bytes.

**Log first, act later.** Two or three weeks of numbers before any vibration, or
there is no baseline to judge against. Audio is never stored, only band energies
and timestamps, and nothing leaves the panel.

**And it is a screening tool, not a diagnosis.** If the log shows regular pauses,
that is the moment to book a sleep study.

**Board to buy: ESP32-S3-DevKitC-1 N16R8.** Not the N32R16V — 32 MB of flash is
useless when the recording goes to an SD card, its 1.8V VDD_SPI domain makes
IO47/IO48 misbehave, and >16 MB flash has known PlatformIO friction. The N16R8
is the same family as the panel, so its platformio.ini is already proven.

---

## 13. Next steps

**Tonight / tomorrow**
1. **Install the motors and the three L298N modules on the bed.**
2. Set `BOOT_SELFTEST` to 0 and flash bed box TEST 012.
3. Flash panel TEST 078.
4. Press a mode tile and watch the sliders move — the first end-to-end,
   two-way run of the whole system.
5. **Do the plywood vibration test before committing to a mounting method.**
6. Commit and push everything (see below).

**When parts arrive**
7. An ESP32 for the audio node, then W5500 Ethernet, then TEST 004 across.
8. The 433 receiver → sniff the fan remote → FS1000A on the audio node.
9. Logic analyser and A7105 module → the bed remote.
10. ESP32-S3 → the listening node, and later ESP-SR speech.

**Waiting on Shemi**
- The crystal marking on the fan remote's `Y1`.
- Which layer of the mattress is on top, latex or visco.
- Whether Ira consents to microphones in the bedroom — a separate decision from
  the massage, and hers to make.

---

## 14. Hard-won lessons

**New this session**

- **`LOW` is an Arduino macro.** `const int LOW = ...` expands to `const int 0`
  and the error message never mentions `LOW`. The same trap waits in `HIGH`,
  `IN`, `OUT`.
- **Arduino core 3.x removed `ledcSetup` and `ledcAttachPin`.** The replacement
  is `ledcAttach(pin, freq, res)`, and from then on `ledcWrite` takes the **pin**
  rather than a channel. A rename alone leaves six motors silent with no error.
  The fix is `#if ESP_ARDUINO_VERSION_MAJOR >= 3` and support both.
- **The bed box `platformio.ini` comment saying "do not pin the platform" is
  now out of date.** It was true when unpinned `espressif32` resolved to the 6.x
  line with core 2.x. It no longer does. The code handles both cores instead.
- **A console that only acts on `\n` will look broken.** The PlatformIO monitor
  can send CR alone, leaving text hanging so the next line arrives joined to it,
  and every command comes back as "unknown". Accept CR, LF or both.
- **When nothing works, make the board prove itself without the console.** The
  boot self-test separated "wiring fault" from "console fault" in one flash.
- **The `ENA`/`ENB` jumpers on an L298N must come off.** Factory-fitted, they
  hold the motor at full speed and swallow the PWM entirely.
- **Motor polarity does not matter for ERM motors** — an eccentric weight
  vibrates either way round.
- **Shielding is about wires, not boxes.** A copper enclosure with twelve cables
  leaving it is barely different from no enclosure.
- **`M` in the VS Code explorer is git, not save.**
- **Speech-to-text translating Hebrew into English is not a comprehension
  problem.** Windows dictation is `Win`+`H`, and it writes in whichever keyboard
  language is active.

**Standing**

- Known-good beats deduced. Read the working file rather than reasoning from
  the code. Fetching the real source from GitHub settled several arguments.
- Chat attachments expire. Anything delivered in a chat gets committed the
  same day. **Bed box TEST 0014 was lost exactly this way and could not be
  recovered — it never existed in the repo or in any searchable chat.**
- Never let a failure be silent. Print the reason, and put it on screen.
- A black screen with no serial output means the failure is before `setup()`.
- `board_build.*` and `board_upload.*` are different things and both matter.
- LVGL's default heap is 32 KB under `LV_CONF_SKIP`; build lists on demand.
- `GFXglyph` offsets are `int8_t`; `bitmapOffset` is `uint16_t`.
- JPEGDEC decodes **baseline** JPEG only.
- Compose off-screen and blit once; a full-screen repaint flickers.
- Any repeated redraw of a large image on this RGB panel shows as a twitch.
- The INMP441 is a 24-bit mic in 32-bit slots.
- **A serial monitor can stop an ESP32 dead** — `monitor_rts = 0` and
  `monitor_dtr = 0`. The cost is that opening the monitor no longer resets the
  board, so the boot banner is missed; print it more than once.
- Windows renumbers COM ports on replug.
- Keep project paths free of Hebrew characters.
- A device reporting a nonsense value may be reporting correctly.

---

*End of handout — Revision 11.*
