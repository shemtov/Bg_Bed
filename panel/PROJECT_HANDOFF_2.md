# PROJECT HANDOFF — Adjustable Bed Massage Retrofit
**HANDOFF_VERSION: 2**

Paste this document at the start of any new chat to continue the project
without re-explaining. This handoff is kept in sync with the code TEST_NUMBER:
TEST_NUMBER and HANDOFF_VERSION always equal.

Last updated: TEST 002 (INMP441 microphone + keyword detection for אָחִינוּ)

---

## 1. What the project is

A DIY massage system for TWO adjacent beds (user שם טוב + wife Ira) in Israel.
Single touchscreen panel (ESP32-8048S043, not Waveshare) mounted mid-headboard
controls vibration massage on both beds, plus internet radio (when audio module
arrives), displays a big clock with temperature/humidity when idle, and will
gain AC infrared control later. Voice wake word "אָחִינוּ" (ahinu = "our brother")
triggers listening mode.

### Bed motors
- Per bed: 10 small vibration motors (ERM type, 12V, ~0.2–0.5A each)
- Driver: L298N (×5 per bed, 2 motors per module)
- Power: 12V 6–7A supply + 12V→5V buck converters

### Panel hardware (ESP32-8048S043)
- Single board at headboard center
- 4.3" 800×480 RGB LCD, capacitive touch (GT911)
- GPIO 19/20: I2C (GT911 touch, BH1750 light sensor, BME280 temp/humidity/pressure)
- GPIO 17/18: UART to bed motor boxes (wired control, no RF)
- GPIO 11/12/13: I2S from INMP441 microphone (NEW in TEST 002)
- GPIO 6: LDR analog input (light level)
- GPIO 2: Backlight control

### Sensors currently connected (all on I2C GPIO 19/20)
- **BH1750FVI** — Light/lux sensor (0x23)
- **BME280** — Temperature + Humidity + Barometric pressure (0x77)
- **GT911** — Capacitive touch controller (0x5D)

### Microphone (NEW - TEST 002)
- **INMP441** — Digital I2S MEMS microphone
  - GPIO 11 (SD/DOUT): Serial data
  - GPIO 12 (SCK/BCLK): Bit clock
  - GPIO 13 (WS/LRCK): Word select
  - Sample rate: 16 kHz, mono (left channel)

---

## 2. Control architecture — Fully wired, RF-free

**CORE RULE:** User is EMF-sensitive. Zero radio emission at the heads.

### Wired communication (UART)
- **Panel ↔ Bed boxes:** GPIO 17/18 UART to each bed (115200 baud)
  - Commands: {bedId, cmd, target, value} — 4 bytes
  - Bed 1 and Bed 2 share one UART line (bedId addresses them)
- **Panel ↔ Audio module:** (When audio module arrives) UART for radio commands

### Voice control (LOCAL ONLY)
- **INMP441 → I2S → ESP32-S3** (local processing, no WiFi)
- Keyword: "אָחִינוּ" (ahinu)
- When detected: wake screen, play beep (optional), show "listening" state
- No cloud speech-to-text (offline only)

### Audio (WiFi device, away from bed)
- **WT32-ETH01 + PCM5102A DAC + TPA3116 amp** (foot of bed, not yet connected)
- Internet radio streaming
- Commands from panel via UART

---

## 3. Current status — TEST 002

**Done:**
- ✓ Display + touch (LVGL 8.4 UI)
- ✓ Massage modes (All / Zones / Motors / Presets)
- ✓ Bed box UART messaging
- ✓ Comfort module: BH1750 light sensor, BME280 temp/humidity, auto-brightness
- ✓ **NEW:** INMP441 I2S input, Voice Activity Detection (VAD)
- ✓ **NEW:** Keyword detection framework (placeholder for ML)

**Not yet done:**
- ML keyword model for "אָחִינוּ" (TFLite / Edge Impulse)
- Screen wake action when keyword detected
- Audio module wiring (audio is deferred)
- Alarms (set on panel, wake via radio)
- AC infrared control (brand not yet known)
- Second panel (if user decides on two panels vs. one)

**Hardware on hand:**
- ESP32-8048S043 panel with display
- L298N modules + 12V motors (per bed)
- BH1750FVI light sensor (connected)
- BME280 temp/humidity/pressure (connected)
- INMP441 microphone (NEW - ready to wire)
- Klipsch FLEXUS CORE 100 soundbar (for audio module)

**Still to buy / receive:**
- WT32-ETH01 + PCM5102A DAC (audio module — on order)
- AC unit brand (for IR control)

---

## 4. Wiring summary — ESP32-8048S043

```
       ESP32-8048S043
          800×480 LCD
            GPIO 19 (SDA) ─┬─ GT911 touch
            GPIO 20 (SCL) ─┼─ BH1750 light (0x23)
                           └─ BME280 temp (0x77)
          
          GPIO 17 (TX) ──→ Bed 1 control (UART)
          GPIO 18 (RX) ←── Bed 1 feedback
          
          GPIO 11 (SD)  ──→ INMP441 microphone (I2S)
          GPIO 12 (SCK) ──→ INMP441
          GPIO 13 (WS)  ──→ INMP441
          
          GPIO 6 (ADC)  ← LDR ambient light
          GPIO 2        → Backlight control
```

---

## 5. Software architecture

### File structure
```
panel/
├── include/
│   ├── config.h          TEST_NUMBER, pin defines, feature switches
│   ├── lv_conf.h         (you must supply from your PC)
│   ├── display.h
│   ├── espnow_link.h     (legacy, kept for reference)
│   ├── massage_ui.h
│   ├── radio.h
│   ├── comfort.h
│   └── microphone.h      NEW in TEST 002
│
└── src/
    ├── main.cpp          Initialization + loop
    ├── display.cpp
    ├── espnow_link.cpp
    ├── massage_ui.cpp
    ├── radio.cpp
    ├── comfort.cpp
    └── microphone.cpp    NEW in TEST 002
```

### Feature switches (config.h)
```cpp
#define ENABLE_MASSAGE_UI     1   // Massage UI
#define ENABLE_RADIO          0   // Deferred (audio module not yet)
#define ENABLE_ESPNOW         0   // Legacy (wired UART only)
#define ENABLE_AUTOBRIGHT     1   // Light sensor + auto-dim
#define ENABLE_MICROPHONE     1   // INMP441 voice input + VAD
#define ENABLE_WIFI           0   // RF-free design
```

### Microphone module (TEST 002)
- **I2S input:** 16 kHz, mono, 512-sample buffers
- **Voice Activity Detection (VAD):** RMS threshold + debounce
- **Keyword detection:** Framework ready (placeholder for ML model)
- **Serial output:** Debug logs of listening state and audio levels
- **Namespace:** `microphone::begin()`, `microphone::loop()`, `microphone::isListening()`, etc.

---

## 6. Version discipline (strict)

**TEST_NUMBER and HANDOFF_VERSION always stay in sync:**

```
code:     #define TEST_NUMBER 2        (first line of config.h + main.cpp)
handoff:  **HANDOFF_VERSION: 2**       (second line of this markdown)
          Both bump together on every change.
```

**Deliverables per test:**
1. Complete `main.cpp`, `config.h`, + new modules (full files, never diffs)
2. This handoff markdown, updated
3. `bed_system_test2.zip` with all three projects (panel, bedbox, audionode)

---

## 7. Next steps (TEST 003+)

### TEST 003: Keyword detection with ML
- Integrate TensorFlow Lite model or Edge Impulse model for "אָחִינוּ"
- Train model with recorded Hebrew speech samples
- When keyword detected → wake screen, play beep, show listening indicator
- Optional: add simple voice commands ("back", "all", etc.)

### TEST 004: Alarms & Messages
- Set alarm on panel (clock face + time picker)
- Alarm triggers at set time: clock screen + radio fades in gently
- Snooze/Off buttons (large touch targets)
- Optional: "wake my spouse at 7am" message between panels (needs second panel)

### TEST 005: AC Infrared Control
- Capture IR commands from user's AC remote
- Add AC tile to home screen
- Control AC via GPIO IR LED + IRremoteESP8266

### Later: Audio module (when WT32-ETH01 arrives)
- Internet radio via PCM5102A → Klipsch soundbar
- Volume control from panel
- Sleep fade-out timer

---

## 8. User context

- Comfortable with electronics and soldering
- Hobbyist programmer (C++ in VS Code + PlatformIO)
- Communicates via voice dictation (occasional garbles: "wavesetter" = Waveshare, "calipse" = Klipsch)
- Highly sensitive to EMF/RF near head at night (why wired UART + no WiFi on panel)
- Based in Israel (Hebrew UI + radio stations)
- Board is **ESP32-8048S043**, NOT Waveshare (important distinction!)

---

## 9. Quick reference: GPIO map

| GPIO | Function | Bus | Status |
|------|----------|-----|--------|
| 19   | SDA (I2C) | I2C | GT911 + BH1750 + BME280 |
| 20   | SCL (I2C) | I2C | ↑ |
| 17   | TX (UART to bed) | UART | Bed box control |
| 18   | RX (from bed) | UART | ↑ |
| 11   | SD (I2S microphone) | I2S | INMP441 data |
| 12   | SCK (I2S clock) | I2S | INMP441 clock |
| 13   | WS (I2S select) | I2S | INMP441 select |
| 6    | ADC (light) | ADC | LDR ambient light |
| 2    | Backlight | GPIO | Backlight ON/OFF |

---

**End of HANDOFF_VERSION: 2**
