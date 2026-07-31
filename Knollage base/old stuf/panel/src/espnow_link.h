// =============================================================================
//  espnow_link  -  the wireless command channel to the two bed control boxes.
//  Protocol (must match main-board/src/main.cpp):
//    bedId, cmd, target, value   (4 bytes)
//    cmd: 0 OFF | 1 ALL | 2 ZONE | 3 MOTOR | 4 PRESET | 5 TIMER
// =============================================================================
#pragma once
#include <Arduino.h>

namespace espnow_link {
  enum Cmd : uint8_t { OFF = 0, ALL = 1, ZONE = 2, MOTOR = 3, PRESET = 4, TIMER = 5 };

  // Call AFTER WiFi is connected (if ENABLE_WIFI) so the channel is known.
  void begin();

  void send(uint8_t bedId, Cmd cmd, uint8_t target, uint8_t value);
}
