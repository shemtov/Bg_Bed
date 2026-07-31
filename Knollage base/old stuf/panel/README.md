# Headboard panel firmware

Massage control for two beds + internet radio, on the Waveshare
ESP32-S3-Touch-LCD-4.3. Written for VS Code + PlatformIO in C++.

## Project layout

```
panel/
  platformio.ini        build config (pioarduino platform = Arduino core 3.x)
  include/
    config.h            ALL feature switches, pins, credentials  <-- start here
    lv_conf.h           LVGL configuration
  src/
    main.cpp            boot sequence, WiFi/NTP, tabview
    display.cpp/.h      RGB panel + LVGL glue + GT911 touch + dimming
    espnow_link.cpp/.h  wireless commands to the bed boxes
    massage_ui.cpp/.h   Massage tab (All/Zones/Motors/Presets, timer, STOP)
    radio.cpp/.h        Radio tab (genres, presets, audio, sleep timer)
    comfort.cpp/.h      auto-brightness (LDR) + light-button bed selection
```

Companion project: `../main-board` — the classic-ESP32 firmware for each
bed's control box (5x L298N, 10 motors). Flash one with `BED_ID 1` and one
with `BED_ID 2`.

## Bring-up stages (flip switches in config.h)

1. **Stage 1 — motors.** `ENABLE_MASSAGE_UI + ENABLE_ESPNOW`, everything
   else 0. Panel and bed boxes on the fixed channel. Test end to end.
2. **Stage 2 — radio.** Add `ENABLE_WIFI + ENABLE_RADIO`. IMPORTANT: with
   WiFi on, ESP-NOW moves to the router's channel — set the bed boxes'
   `RF_CHANNEL` to your router's channel (the serial log prints it).
3. **Stage 3 — comfort.** Add `ENABLE_LIGHT_BUTTONS + ENABLE_AUTOBRIGHT`.
   `ENABLE_BACKLIGHT_PWM` only after the one-wire backlight mod.

## Known first-boot tuning points

- **Blank/white screen** → check `board_build.arduino.memory_type = qio_opi`
  took effect (PSRAM), and the RGB pin map in `display.cpp`.
- **Touch mirrored/offset** → one-line fix in `touchCb()` in `display.cpp`.
- **Backlight** → stock board is on/off only (CH422G expander). Content
  dimming works out of the box; true dimming needs the hardware mod
  described in the chat (wire from a free GPIO to the MP3302 control point,
  then set `ENABLE_BACKLIGHT_PWM 1` and `PIN_BACKLIGHT`).
- **Audio pins** → `PIN_I2S_*` in config.h were chosen to avoid the RGB
  bus; verify they're free on your board revision before wiring the DAC.
- **Radio Browser** → `openGenre()` uses `all.api.radio-browser.info`;
  if it ever moves, update the URL in `radio.cpp`.

## Honest status

Written before the hardware was on the desk. The architecture and protocol
are solid; expect small compile fixes (library versions move) and the usual
first-boot tuning above. Bring it up stage by stage and it will be easy to
see which module any problem belongs to.
