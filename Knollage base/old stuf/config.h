#pragma once
// =============================================================================
//  TEST NUMBER  -  bumped on EVERY change, no exceptions. Shown top-right on
//  the screen and printed on the serial monitor at boot.
// =============================================================================
#define TEST_NUMBER 1

// =============================================================================
//  config.h  -  THE control panel of the firmware.
//  Every feature can be switched on/off here (1 = on, 0 = off), so you can
//  bring the system up one piece at a time and study each module separately.
// =============================================================================

// ---------------------------------------------------------------------------
//  FEATURE SWITCHES
// ---------------------------------------------------------------------------
#define ENABLE_MASSAGE_UI     1   // massage tab: modes, sliders, presets, timer
#define ENABLE_ESPNOW         1   // wireless link to the bed control boxes
#define ENABLE_RADIO          0   // radio tab + audio streaming (needs WiFi + DAC)
#define ENABLE_WIFI           0   // join home WiFi (required by RADIO and NTP)
#define ENABLE_LIGHT_BUTTONS  0   // bed selection from the two LED-strip buttons
#define ENABLE_AUTOBRIGHT     0   // LDR + time-of-day screen dimming
#define ENABLE_BACKLIGHT_PWM  0   // ONLY after the one-wire backlight mod (see docs)

// Suggested bring-up order:
//   Stage 1: MASSAGE_UI + ESPNOW           (test motors end to end)
//   Stage 2: + WIFI + RADIO                (audio chain)
//   Stage 3: + LIGHT_BUTTONS + AUTOBRIGHT  (comfort features)

// ---------------------------------------------------------------------------
//  IDENTITY / RADIO LINK
// ---------------------------------------------------------------------------
#define NUM_BEDS        2
#define DEFAULT_BED     1
// IMPORTANT: with WiFi ON, ESP-NOW rides on the router's channel. The two bed
// boxes must then be set to the SAME channel as your router (check your router
// admin page, or the serial log prints it after WiFi connects). With WiFi OFF,
// this fixed channel is used and the bed boxes must match it.
#define ESPNOW_FIXED_CHANNEL  1

// ---------------------------------------------------------------------------
//  WIFI (only used if ENABLE_WIFI)
// ---------------------------------------------------------------------------
#define WIFI_SSID   "YOUR_WIFI_NAME"
#define WIFI_PASS   "YOUR_WIFI_PASSWORD"
#define NTP_SERVER  "pool.ntp.org"
#define TZ_STRING   "IST-2IDT,M3.4.4/26,M10.5.0"   // Israel time with DST

// ---------------------------------------------------------------------------
//  PINS  -  Waveshare ESP32-S3-Touch-LCD-4.3
//  (Display RGB pins live in display.cpp; these are the free/peripheral pins.)
// ---------------------------------------------------------------------------
// Touch (GT911) shares the board I2C bus with the CH422G expander
#define PIN_TOUCH_SDA   8
#define PIN_TOUCH_SCL   9

// I2S audio out to the PCM5102A DAC (any 3 free pins; these avoid RGB pins)
#define PIN_I2S_BCLK    19
#define PIN_I2S_LRCK    20
#define PIN_I2S_DOUT    11

// LDR light sensor - the board's "Sensor" header is wired to GPIO6 (ADC)
#define PIN_LDR         6

// Light-button bed selection inputs (sensed 3.3V signals, one per bed)
#define PIN_BEDBTN_1    15
#define PIN_BEDBTN_2    16

// Backlight PWM - ONLY meaningful after the hardware mod; pick the pin you
// soldered to the MP3302 control point. GPIO6 is taken by the LDR here, so
// use a different free pin for the mod, e.g. GPIO4 if available on your unit.
#define PIN_BACKLIGHT   4
#define BACKLIGHT_PWM_HZ 1000    // ~1 kHz avoids backlight inductor whine

// ---------------------------------------------------------------------------
//  MASSAGE
// ---------------------------------------------------------------------------
#define NUM_MOTORS      10
#define NUM_ZONES       5
#define TIMER_MIN_MIN   5
#define TIMER_MAX_MIN   60        // set 120 for a two-hour ceiling
#define TIMER_STEP_MIN  5

// ---------------------------------------------------------------------------
//  RADIO
// ---------------------------------------------------------------------------
#define RADIO_VOLUME_DEFAULT 8    // 0..21 scale of the audio library
#define SLEEP_TIMER_MIN      30   // "Sleep 30" button length

// ---------------------------------------------------------------------------
//  AUTO BRIGHTNESS
// ---------------------------------------------------------------------------
#define BRIGHT_SAMPLE_MS     1000  // read LDR once per second
#define BRIGHT_AVG_WINDOW    10    // average over 10 samples (hysteresis)
#define BRIGHT_NIGHT_START   21    // hour: force dim after this
#define BRIGHT_NIGHT_END     7     // hour: back to sensor control after this
