// =============================================================================
//  HEADBOARD PANEL - main.cpp - TEST 002
//  ESP32-8048S043: massage control + internet radio + voice wake word
//  NEW: INMP441 microphone + keyword detection for "אָחִינוּ" (ahinu)
// =============================================================================
#include <Arduino.h>
#include <lvgl.h>
#include "config.h"
#include "display.h"
#include "espnow_link.h"
#include "massage_ui.h"
#include "radio.h"
#include "comfort.h"
#include "microphone.h"

#if ENABLE_WIFI
#include <WiFi.h>
#endif

void setup() {
  Serial.begin(115200);
  Serial.printf("\n=== PANEL — TEST %d — LVGL UI + Microphone Voice Wake ===\n", TEST_NUMBER);

  display::begin();

  // Test number badge, top-right corner
  lv_obj_t *testLbl = lv_label_create(lv_layer_top());
  lv_label_set_text_fmt(testLbl, "T%d", TEST_NUMBER);
  lv_obj_set_style_text_color(testLbl, lv_color_hex(0x8b92ab), 0);
  lv_obj_align(testLbl, LV_ALIGN_TOP_RIGHT, -8, 6);

#if ENABLE_WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[wifi] connecting");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] connected, IP %s, channel %d\n",
                  WiFi.localIP().toString().c_str(), WiFi.channel());
    configTzTime(TZ_STRING, NTP_SERVER);
  } else {
    Serial.println("[wifi] FAILED - radio/NTP unavailable this session");
  }
#endif

  espnow_link::begin();     // after WiFi so the channel is settled

  // ---- top-level UI: tabview with Massage and Radio ----
  lv_obj_t *tabs = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 48);
  lv_obj_set_style_bg_color(tabs, lv_color_hex(0x14161f), 0);

#if ENABLE_MASSAGE_UI
  lv_obj_t *tabM = lv_tabview_add_tab(tabs, "Massage");
  massage_ui::build(tabM);
#endif

#if ENABLE_RADIO
  lv_obj_t *tabR = lv_tabview_add_tab(tabs, "Radio");
  radio::build(tabR);
#endif

  autobright::begin();
  bedselect::begin();

#if ENABLE_MICROPHONE
  microphone::begin();
  Serial.println("[setup] microphone enabled");
#endif

  Serial.println("[panel] ready");
}

void loop() {
  display::loop();           // LVGL rendering
  radio::loop();             // audio pump + sleep timer
  autobright::loop();        // ambient brightness sensor
  bedselect::loop();         // bed selection logic

#if ENABLE_MICROPHONE
  microphone::loop();        // I2S audio input + VAD + keyword detection
  
  // Debug: print microphone status occasionally
  static uint32_t last_status = 0;
  if (millis() - last_status > 5000) {
    last_status = millis();
    if (microphone::isListening()) {
      Serial.printf("[mic] listening... audio_level=%.3f time=%ums\n",
                    microphone::getAudioLevel(),
                    microphone::getListeningTime());
    }
  }
  
  // If keyword detected, wake the screen
  if (microphone::isKeywordDetected()) {
    Serial.println("[mic] *** KEYWORD DETECTED: אָחִינוּ ***");
    // TODO: Trigger screen wake, play beep, etc.
    microphone::resetKeywordDetection();
  }
#endif

  delay(5);
}
