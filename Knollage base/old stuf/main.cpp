// =============================================================================
//  HEADBOARD PANEL  -  main.cpp
//  Waveshare ESP32-S3-Touch-LCD-4.3: massage control for two beds + internet
//  radio + comfort features. Every module switches on/off in include/config.h.
//
//  Companion firmware: ../main-board (one classic ESP32 per bed, 5x L298N).
// =============================================================================
#include <Arduino.h>
#include <lvgl.h>
#include "config.h"
#include "display.h"
#include "espnow_link.h"
#include "massage_ui.h"
#include "radio.h"
#include "comfort.h"

#if ENABLE_WIFI
#include <WiFi.h>
#endif

void setup() {
  Serial.begin(115200);
  Serial.printf("\n[panel] booting - TEST %d\n", TEST_NUMBER);

  display::begin();

  // Test number badge, top-right, on the top layer so it stays visible on
  // every screen and tab. The reference for "what is running right now".
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

  Serial.println("[panel] ready");
}

void loop() {
  display::loop();      // LVGL
  radio::loop();        // audio pump + sleep timer (no-op if disabled)
  autobright::loop();
  bedselect::loop();
  delay(5);
}
