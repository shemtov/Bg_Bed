// =============================================================================
//  MICROPHONE.H  -  INMP441 VOICE INPUT + KEYWORD DETECTION
//  Digital I2S microphone: "אָחִינוּ" (ahinu) wake word spotting
// =============================================================================
#pragma once

#include <Arduino.h>
#include "config.h"

#if ENABLE_MICROPHONE

namespace microphone {
  
  // ---- Initialization ----
  void begin();
  void loop();
  void end();
  
  // ---- Status queries ----
  bool isListening();           // True if actively listening
  bool isKeywordDetected();     // True if "אָחִינוּ" was just detected
  float getAudioLevel();        // 0–1, RMS of last buffer
  uint32_t getListeningTime();  // Milliseconds spent listening this session
  
  // ---- Control ----
  void resetKeywordDetection(); // Clear detected flag
  void setListeningEnabled(bool en);  // Enable/disable listening
  
  // ---- Debug output ----
  void printStatus();           // Serial print current state
  
} // namespace microphone

#else

// ---- Stub implementations when ENABLE_MICROPHONE = 0 ----
namespace microphone {
  inline void begin() {}
  inline void loop() {}
  inline void end() {}
  inline bool isListening() { return false; }
  inline bool isKeywordDetected() { return false; }
  inline float getAudioLevel() { return 0.0f; }
  inline uint32_t getListeningTime() { return 0; }
  inline void resetKeywordDetection() {}
  inline void setListeningEnabled(bool en) {}
  inline void printStatus() { Serial.println("[mic] disabled"); }
}

#endif // ENABLE_MICROPHONE
