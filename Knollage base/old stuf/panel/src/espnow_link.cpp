#include "espnow_link.h"
#include "config.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace espnow_link {

static uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct __attribute__((packed)) {
  uint8_t bedId;
  uint8_t cmd;
  uint8_t target;
  uint8_t value;
} Msg;

void begin() {
#if ENABLE_ESPNOW
#if !ENABLE_WIFI
  // Standalone mode: fix the channel; bed boxes must use the same one.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_FIXED_CHANNEL, WIFI_SECOND_CHAN_NONE);
#endif
  // With WiFi ON we ride the router's channel - nothing to set here, but the
  // bed boxes MUST be configured to that same channel (see config.h note).

  if (esp_now_init() != ESP_OK) {
    Serial.println("[espnow] init failed");
    return;
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, bcast, 6);
  peer.channel = 0;      // 0 = use current channel
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  uint8_t ch;
  wifi_second_chan_t sc;
  esp_wifi_get_channel(&ch, &sc);
  Serial.printf("[espnow] ready on channel %d - bed boxes must match\n", ch);
#endif
}

void send(uint8_t bedId, Cmd cmd, uint8_t target, uint8_t value) {
#if ENABLE_ESPNOW
  Msg m = {bedId, (uint8_t)cmd, target, value};
  esp_now_send(bcast, (uint8_t *)&m, sizeof(m));
#else
  Serial.printf("[espnow OFF] bed=%d cmd=%d target=%d value=%d\n",
                bedId, cmd, target, value);
#endif
}

} // namespace espnow_link
