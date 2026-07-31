// =============================================================================
//  CONFIG.H  -  HEADBOARD PANEL
//  ESP32-8048S043  |  Waveshare 4.3" 800x480 RGB touchscreen
//  CORE SETTINGS — PINS, CREDENTIALS, FEATURE SWITCHES
// =============================================================================
#define TEST_NUMBER 2

// ---- WIFI & NETWORK ----
#define ENABLE_WIFI 0                    // 0 = RF-free, 1 = WiFi enabled
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASSWORD"
#define NTP_SERVER "time.nist.gov"
#define TZ_STRING "IST-2IDT,M3.4.4,M10.5.0"  // Israel timezone

// ---- FEATURE SWITCHES (compile-time) ----
#define ENABLE_MASSAGE_UI     1
#define ENABLE_RADIO          0           // Deferred: audio module not yet connected
#define ENABLE_ESPNOW         0           // RF-free design: UART only
#define ENABLE_LIGHT_BUTTONS  0           // Deferred: light button sensing
#define ENABLE_AUTOBRIGHT     1           // Auto-dimming + sensor reading
#define ENABLE_BACKLIGHT_PWM  0           // Optional: hardware PWM dim mod
#define ENABLE_MICROPHONE     1           // NEW: INMP441 voice keyword detection

// ============================================================================
//  GPIO PIN ASSIGNMENTS — ESP32-8048S043
// ============================================================================

// ---- DISPLAY & TOUCH (Built-in) ----
// RGB panel pins handled by GFX Library (see display.cpp)
// GT911 touch on I2C: GPIO 19 (SDA), GPIO 20 (SCL)
#define I2C_SDA_PIN           19
#define I2C_SCL_PIN           20
#define TOUCH_I2C_ADDR        0x5D

// ---- SERIAL LINKS ----
#define UART_TX_TO_BEDBOX     17          // Bed box control (UART1)
#define UART_RX_FROM_BEDBOX   18
#define UART_BAUD             115200

// ---- SENSORS (I2C: GPIO 19/20, shared bus) ----
#define BH1750_I2C_ADDR       0x23        // Light sensor (Lux)
#define BME280_I2C_ADDR       0x77        // Temp + Humidity + Pressure

// ---- LIGHT SENSOR (Analog ADC) ----
#define LDR_ADC_PIN           6           // Ambient light (0–4095)
#define LDR_BRIGHTNESS_MIN    30          // Dimming floor (%)
#define LDR_BRIGHTNESS_MAX    100         // Dimming ceiling (%)

// ---- MICROPHONE (I2S) — NEW TEST 002 ----
// INMP441 digital microphone on I2S bus
#define I2S_BCK_PIN           12          // Bit Clock (SCK)
#define I2S_WS_PIN            13          // Word Select (WS / LRCK)
#define I2S_DIN_PIN           11          // Data In (SD / DOUT)
#define I2S_NUM               I2S_NUM_0   // I2S port 0
#define I2S_SAMPLE_RATE       16000       // 16 kHz
#define I2S_BUFFER_SIZE       512         // Samples per read

// Keyword detection ("אָחִינוּ" - ahinu / "our brother")
#define KEYWORD_DETECTION_ENABLED  1
#define KEYWORD_THRESHOLD     0.6         // Confidence threshold (0–1)
#define KEYWORD_LISTEN_TIMEOUT_MS  3000   // Max listening duration

// ---- BACKLIGHT CONTROL ----
#define BL_CONTROL_PIN        2           // CH422G GPIO for backlight ON/OFF
#define BL_PWM_PIN            4           // (Optional) PWM for dimming mod

// ---- UI TIMEOUTS ----
#define IDLE_TO_CLOCK_MS      120000      // 2 min: HOME → CLOCK
#define CLOCK_TO_DARK_MS      600000      // 10 min: CLOCK → DARK
#define DARK_LIGHT_MS         60000       // 1 min: DARK wakes to CLOCK
#define SCREEN_WAKE_DEBOUNCE  500         // PIR debounce (ms)

// ---- DISPLAY COLORS (LVGL hex) ----
#define COLOR_BG              0x14161f    // Near-black background
#define COLOR_CARD            0x1b1e2a    // Card surface
#define COLOR_TEXT_PRIMARY    0xffffff    // White text
#define COLOR_TEXT_SECONDARY  0x8b92ab    // Dim text
#define COLOR_ACCENT          0x00d4ff    // Bright cyan accent

// ---- RADIO (when audio module arrives) ----
#define RADIO_DEFAULT_VOLUME  10          // 0–21 (ESP32-audioI2S scale)
#define RADIO_SLEEP_TIMER_MAX 120         // Max sleep fade (minutes)
#define RADIO_BUFFER_SIZE     8192        // Stream buffer

// ============================================================================
//  BED SELECTION & DEFAULTS
// ============================================================================
#define PANEL_ID              1           // This panel controls Bed 1 by default
#define DEFAULT_BED_SELECT    0           // Start on Bed 1 (0-indexed)
#define MASSAGE_MODE_DEFAULT  0           // 0 = All, 1 = Zones, etc.
#define MASSAGE_INTENSITY_DEFAULT  50     // 0–100 %
#define MASSAGE_TIMER_DEFAULT 15          // Minutes

// ============================================================================
//  DEBUG / SERIAL OUTPUT
// ============================================================================
#define SERIAL_BAUD           115200
#define DEBUG_ENABLED         1
#define DEBUG_VERBOSE         0           // Extra logging (slower)

// ============================================================================
//  END CONFIG.H
// ============================================================================
