# Bg_Bed — Full Interconnect Specification

**Revision 1 — 16 Aug 2026**
Companion document to `Handout.md`. This file covers **every physical wire between every module**.
Repo location: **root folder**, alongside `Handout.md`.

---

## 0. How to read this document

Each cable has its own numbered section with a pin-to-pin table.
Every row is one physical conductor.

Status marks used throughout:

| Mark | Meaning |
|---|---|
| ✅ | Confirmed on real hardware |
| 🔷 | Taken from an existing working build, not yet used for this purpose |
| ⚠️ | Proposal only — never flashed, never measured |

**Nothing marked ⚠️ should be soldered permanently before `TEST 001` runs.**

---

## 1. Module inventory

| id | Module | Board | Location | Status |
|---|---|---|---|---|
| 1 | `bedbox_shemi` | `QuinLED-ESP32` | under-bed storage | ✅ `TEST 012` running |
| 2 | `bedbox_ira` | `ESP32-S3-DevKitC-1 N16R8` | under-bed storage | ⚠️ board not arrived, no firmware |
| 8 | `listener` | `ESP32-S3-DevKitC-1 N16R8` | under-bed storage | ⚠️ hardware in hand, zero firmware |
| 9 | `audionode` | `ESP32-S3-WROOM-1 N16R8` | under-bed storage | ✅ `TEST 010` plays radio |
| — | `panel_shemi` | `ESP32-8048S043` | left headboard | ✅ `TEST 085` installed |
| — | `panel_ira` | `Waveshare ESP32-S3-Touch-LCD-4.3` | right headboard | ✅ `TEST 079` installed |

**All four boards with an id live together in the under-bed storage.**
The two panels are at the headboards. The 12V star point is in the storage space.

---

## 2. Cable schedule and estimated lengths

⚠️ **Every length below is an estimate made without measuring the room.**
Replace each one with a real tape-measure figure before buying cable. Add 20% for routing slack.

| # | Cable | From | To | Type | Est. length |
|---|---|---|---|---|---|
| C1 | Panel feed Shemi | star point | `panel_shemi` | shielded Cat6 | 200 cm |
| C2 | Panel feed Ira | star point | `panel_ira` | shielded Cat6 | 320 cm |
| C3 | Mic arm left | `listener` | mic at Shemi's pillow | shielded 4-core + drain | 220 cm |
| C4 | Mic arm right | `listener` | mic at Ira's pillow | shielded 4-core + drain | 300 cm |
| C5 | Bus jumper 1 | `bedbox_shemi` | `listener` | 3-core loose | 25 cm |
| C6 | Bus jumper 2 | `listener` | `audionode` | 3-core loose | 25 cm |
| C7 | Bus jumper 3 | `audionode` | `bedbox_ira` | 3-core loose | 25 cm |
| C8 | Live audio link | `audionode` | `listener` | 4-core ribbon | 25 cm |
| C9 | Audio out | `audionode` | converter box → soundbar | shielded audio | 230 cm |
| C10 | Ethernet | router | `audionode` | ordinary Cat5e/6 patch | ~1000 cm |
| C11 | Motor loom Shemi | `bedbox_shemi` | 6 motors in mattress | 2-core per motor | 100–250 cm each |
| C12 | Motor loom Ira | `bedbox_ira` | 8 motors in mattress | 2-core per motor | 100–250 cm each |
| C13 | Mains to PSUs | wall socket | 12V supplies | mains flex | site-dependent |

---

## 3. The RS-485 chain — order and termination

The bus is **one continuous chain**. No stars, no spurs, no T-joints.

```
panel_shemi ──C1──> bedbox_shemi ──C5──> listener ──C6──> audionode ──C7──> bedbox_ira ──C2──> panel_ira
   [120R]                                                                                        [120R]
```

- **`120Ω` termination resistors go across `A` and `B` at `panel_shemi` and `panel_ira` only.**
  The four under-bed boards are middle nodes. **No resistor on any of them.**
- `panel_shemi` is the bus arbiter and the `DS3231` time source. No other node transmits unprompted.
- Every node needs `A`, `B`, **and** a ground reference conductor. Three wires, not two.

---

## 4. Transceiver — `SP3485`

All six nodes use an `SP3485` module. Its module pinout is identical everywhere:

| Module pin | Goes to |
|---|---|
| `VCC` | 3V3 of the host board — **not 5V**, the `SP3485` is a 3.3V part |
| `GND` | host `GND` |
| `RO` | host `UART RX` |
| `DI` | host `UART TX` |
| `RE` + `DE` | tied together, to one host `GPIO` (direction control) |
| `A` | bus `A` |
| `B` | bus `B` |

⚠️ **Direction control:** most `SP3485` breakout boards bring `RE` and `DE` out separately.
Bridge them and drive with a single pin: HIGH to transmit, LOW to receive.
Some boards have this bridge already fitted — **check yours with a meter before wiring.**

---

## 5. C1 — star point to `panel_shemi` (shielded Cat6, ~200 cm)

`panel_shemi` pins: `UART1 TX = IO17`, `RX = IO18` 🔷 (already defined and in use as bidirectional UART).
`IO11`, `IO12`, `IO13` are free — they were the removed microphone.

| Cat6 pair | Colour | Conductor | Star-point end | Panel end |
|---|---|---|---|---|
| 1 | orange | 12V + | PSU +12V | buck converter input + |
| 1 | orange/white | 12V − | PSU `GND` | buck converter input − |
| 2 | green | 12V + | PSU +12V (paralleled) | buck input + (paralleled) |
| 2 | green/white | 12V − | PSU `GND` (paralleled) | buck input − (paralleled) |
| 3 | blue | `RS-485 A` | first node `A` | `SP3485` `A` |
| 3 | blue/white | `RS-485 B` | first node `B` | `SP3485` `B` |
| 4 | brown | bus `GND` reference | star `GND` | panel `GND` — **connect both ends** |
| 4 | brown/white | spare | — | — |
| — | foil + drain | shield | star `GND` | **open — do not connect** |

**Panel-side connections:**

| Signal | `panel_shemi` pin | Status |
|---|---|---|
| `SP3485 RO` → | `IO18` (RX) | 🔷 |
| `SP3485 DI` ← | `IO17` (TX) | 🔷 |
| `SP3485 RE/DE` | `IO11` | ⚠️ proposal |
| `SP3485 VCC` | 3V3 | — |
| `SP3485 GND` | `GND` | — |
| `120Ω` | across `A`–`B` | — |

---

## 6. C2 — star point to `panel_ira` (shielded Cat6, ~320 cm)

Pair assignment and shield rule are **identical to C1**. Only the panel end differs.

`panel_ira` `RS-485` is documented as `GPIO15`/`GPIO16` via `SP3485`.

| Signal | `panel_ira` pin | Status |
|---|---|---|
| `SP3485 RO` → | `GPIO16` (RX) | ⚠️ **needs verification against the Waveshare schematic** |
| `SP3485 DI` ← | `GPIO15` (TX) | ⚠️ same |
| `SP3485 RE/DE` | not yet assigned | ⚠️ **open item** |
| `SP3485 VCC` | 3V3 | — |
| `120Ω` | across `A`–`B` | — |

⚠️ `panel_ira` has very few free pins. `GPIO6` is free and comes out on the `Sensor AD` connector —
that is the obvious candidate for direction control, but it was also earmarked for the
`MP3302` backlight dimming mod. **One of those two features has to give.**

---

## 7. C3 / C4 — microphone arms (shielded 4-core + drain)

Two `INMP441` breakouts, one per pillow. **Use the two square-board units** — their `L/R` marking is legible and confirmed.

Both mics share `BCK`, `WS` and `SD`. Side is set by the `L/R` pin.

| Conductor | Listener pin | Mic Shemi (left) | Mic Ira (right) | Status |
|---|---|---|---|---|
| `3V3` | 3V3 | `VDD` | `VDD` | ⚠️ |
| `GND` | `GND` | `GND` | `GND` | ⚠️ |
| `BCK` | `GPIO4` | `SCK` | `SCK` | ⚠️ |
| `WS` | `GPIO5` | `WS` | `WS` | ⚠️ |
| `SD` | `GPIO6` | `SD` | `SD` | ⚠️ |
| `L/R` | — | to `GND` at the mic | to `3V3` at the mic | ⚠️ |
| shield | `GND` at board end | **open** | **open** | — |

**Notes**

- `SD` is one wire split to both mics — each speaks only in its own half of the frame.
- `L/R` is strapped **at the mic**, not run back down the cable. Saves a conductor.
- Fit **33Ω** series resistors on `BCK` and `WS` at the listener pin.
- Fit **100nF** across `3V3`/`GND` at each mic.
- Cross motor wiring at 90°, keep ≥10 cm from any `בקר`.

> ⚠️ **This is the single biggest unknown in the whole system.**
> `I2S` over 220–300 cm is past comfortable. It may work; it may return silence or garbage.
> `TEST 001` exists specifically to answer this, and it must run on the **real cable length**
> before anything is mounted permanently.

---

## 8. C5 / C6 / C7 — bus jumpers inside the storage (3-core, ~25 cm)

These three hops are short, inside one enclosure, on one shared 12V distribution.
**They carry bus only — no power pairs, no shield.**

| Conductor | Connects |
|---|---|
| `A` | node `A` to next node `A` |
| `B` | node `B` to next node `B` |
| `GND` | node `GND` to next node `GND` |

**Node-side transceiver pins:**

| Node | TX (`DI`) | RX (`RO`) | `RE/DE` | Status |
|---|---|---|---|---|
| `bedbox_shemi` (QuinLED) | `IO17` | `IO16` | `IO5` | ⚠️ proposal — free pins, motors use 13/14/18/19/21/22, 26/27 reserved |
| `listener` | `GPIO17` | `GPIO18` | `GPIO16` | ⚠️ proposal |
| `audionode` | `GPIO17` | `GPIO18` | `GPIO16` | 🔷 reserved in the handout, never wired |
| `bedbox_ira` | `GPIO17` | `GPIO18` | `GPIO16` | ⚠️ proposal — board not arrived |

---

## 9. C8 — live audio link, `audionode` → `listener` (4-core, ~25 cm)

**The `audionode` is the clock master on this link.** It already runs `I2S0` to the `PCM5102A`;
this link uses its free `I2S1`. The `listener` slaves to the incoming clocks.

This removes the two-clock drift problem — there is only one clock domain, so no buffer
slowly filling or emptying, and nothing to resample.

| Conductor | `audionode` pin | Direction | `listener` pin | Status |
|---|---|---|---|---|
| `BCK` | `GPIO4` | → | `GPIO15` | ⚠️ |
| `WS` | `GPIO5` | → | `GPIO21` | ⚠️ |
| `DATA` | `GPIO6` | ← | `GPIO14` | ⚠️ |
| `GND` | `GND` | — | `GND` | — |

**Free pins used here are genuinely free:** `audionode` spare list is 1, 4, 5, 6, 15, 21, 38, 47, 48
(with 16/17/18 reserved for `RS-485`). Listener 14/15/21 avoid the mic pins (4/5/6),
the microSD pins (10/11/12/13) and the bus pins (16/17/18).

⚠️ **Firmware for this does not exist on either board.** The `audionode` has no logic to switch
its output between the radio decoder and an incoming stream; the `listener` has no playback path.
Run the four wires now regardless — they cost nothing today and are painful to add later.
If live passthrough proves troublesome, the same four conductors can carry a file transfer instead.

---

## 10. C9 — audio out to soundbar (~230 cm)

Unchanged from the working system. Do not modify what is proven.

`PCM5102A` analogue out → converter box → optical → soundbar.

`PCM5102A` solder bridges, **confirmed and critical**: `H1L→L`, `H2L→L`, `H3L→H`, `H4L→L`.
`H3L` low causes a hard mute with no error message.

`PCM5102A` pins ✅ hardware-confirmed: `BCK GPIO14`, `LCK GPIO2`, `DIN GPIO13`, `SCK` pad bridged to `GND`.

---

## 11. C10 — Ethernet (~1000 cm)

Ordinary patch cable, router to `audionode`. No special treatment.

`W5500` pins ✅ hardware-confirmed: `SCLK 12`, `MOSI 11`, `MISO 10`, `SCS 9`, `INT 8`, `RST 7`.

`MAC` is `12:C1:9F:CD:37:C9` — reserve a fixed address for it in the router.

---

## 12. Power distribution

**One star point in the under-bed storage.** Every ground returns there and nowhere else.

| Rail | Feeds | Notes |
|---|---|---|
| 12V high-current | `bedbox_shemi`, `bedbox_ira` | own thick pair each, fused 6A |
| 12V low-current | C1 and C2 to the two panels | Cat6 pairs, ~1A each |
| 12V low-current | `listener`, `audionode` | short local leads |

> ⚠️ **The bed boxes must never be powered through Cat6.**
> They are fused at 6A for the motors and 23 AWG cannot carry that current.
> Their Cat6 (if any) carries bus only; power comes on a separate heavy pair.

Voltage drop reference: 23 AWG ≈ 0.067 Ω/m. Over 10 m at 1A that is about 0.7V — acceptable
for a panel, nowhere near acceptable for a motor box.

---

## 13. Connectors

**Decision: `GX12` aviation connectors** at every enclosure wall.

Screw-locking, keyed, cheap, panel-mount. Rejected alternatives and why:

- **RJ45** — the `audionode` has a real `RJ45` for the `W5500`. Two identical sockets in one
  room, one of them carrying 12V, is a fault waiting to happen.
- **USB-C** — the most common connector in the house. Someone will eventually plug a phone in.
  Also friction-retained, on a bed that articulates every night.
- **RJ11/RJ12** — those plugs physically insert into `RJ45` jacks. Same failure, smaller.

Molex Micro-Fit 3.0 is fine **inside** a box where a screwdriver is needed to reach it.

---

## 14. Open items blocking a final build

1. ⚠️ **Real measured distances** — every figure in §2 is a guess.
2. ⚠️ **`panel_ira` `RE/DE` pin** — `GPIO6` collides with the backlight dimming mod. Must choose.
3. ⚠️ **`panel_ira` `RS-485` pin numbers** — verify `GPIO15`/`GPIO16` against the Waveshare schematic.
4. ⚠️ **`SP3485` module** — confirm whether `RE`/`DE` are already bridged on the boards you bought.
5. ⚠️ **`bedbox_ira` board** — not arrived, pin map unverifiable.
6. ⚠️ **microSD module** — not yet bought. Must be the bare 3.3V SPI type: no `AMS1117`, no `74LVC125`.
7. ⚠️ **`TEST 001` on the listener** — must run at full cable length before anything is mounted.

---

*End of Revision 1.*
