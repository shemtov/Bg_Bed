// =============================================================================
//  MICROPHONE.CPP  -  INMP441 I2S INPUT + KEYWORD DETECTION
//  Captures digital audio from INMP441, detects voice activity, logs for ML
// =============================================================================
#include "microphone.h"

#if ENABLE_MICROPHONE

#include <driver/i2s.h>
#include <math.h>

namespace microphone {

// ---- Internal state ----
static bool g_enabled = false;
static bool g_listening = false;
static bool g_keyword_detected = false;
static uint32_t g_listening_start_ms = 0;
static float g_last_rms = 0.0f;

// I2S audio buffer
static const size_t BUFFER_SIZE = I2S_BUFFER_SIZE;
static int16_t g_i2s_buffer[BUFFER_SIZE];
static uint32_t g_bytes_read = 0;

// Voice Activity Detection (VAD) thresholds
static const float VAD_THRESHOLD = 0.05f;     // RMS threshold to detect speech
static const uint32_t VAD_DEBOUNCE_MS = 500;  // Minimum duration to count as speech
static const uint32_t VAD_TIMEOUT_MS = KEYWORD_LISTEN_TIMEOUT_MS;  // Max listen time

// Keyword detection state
static uint32_t g_vad_start_ms = 0;
static bool g_in_speech = false;

// ---- I2S Configuration ----
void begin() {
  Serial.printf("[mic] TEST %d — INMP441 I2S initialization\n", TEST_NUMBER);
  
  i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_RX,
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // INMP441 is mono
    .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0,
  };

  // Install I2S driver
  esp_err_t ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  if (ret != ESP_OK) {
    Serial.printf("[mic] ERROR: i2s_driver_install failed (%d)\n", ret);
    return;
  }

  // Set GPIO pins
  i2s_set_pin(I2S_NUM, &(i2s_pin_config_t){
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_DIN_PIN,
  });

  // Start the I2S peripheral
  i2s_start(I2S_NUM);
  
  g_enabled = true;
  g_listening = false;
  g_keyword_detected = false;
  
  Serial.printf("[mic] I2S started: %d Hz, %d samples/buffer, GPIO [BCK:%d WS:%d DIN:%d]\n",
                I2S_SAMPLE_RATE, BUFFER_SIZE, I2S_BCK_PIN, I2S_WS_PIN, I2S_DIN_PIN);
}

// ---- Calculate RMS (root mean square = audio energy) ----
static float calculateRMS(const int16_t* samples, size_t count) {
  if (count == 0) return 0.0f;
  
  int64_t sum_squares = 0;
  for (size_t i = 0; i < count; i++) {
    int32_t sample = samples[i];
    sum_squares += (int64_t)sample * sample;
  }
  
  float mean_square = (float)sum_squares / count;
  return sqrtf(mean_square) / 32768.0f;  // Normalize to 0–1 (16-bit range)
}

// ---- Voice Activity Detection ----
static void updateVAD(float rms) {
  uint32_t now = millis();
  
  if (rms > VAD_THRESHOLD) {
    // Speech detected
    if (!g_in_speech) {
      g_in_speech = true;
      g_vad_start_ms = now;
      Serial.printf("[mic] speech detected (RMS %.3f)\n", rms);
    }
    
    // Update listening state
    if (!g_listening) {
      g_listening = true;
      g_listening_start_ms = now;
      Serial.printf("[mic] LISTENING for keyword... (say 'אָחִינוּ')\n");
    }
  } else {
    // No speech
    if (g_in_speech && (now - g_vad_start_ms) > VAD_DEBOUNCE_MS) {
      g_in_speech = false;
      Serial.printf("[mic] speech ended\n");
    }
  }
  
  // Timeout: stop listening after max duration
  if (g_listening && (now - g_listening_start_ms) > VAD_TIMEOUT_MS) {
    g_listening = false;
    Serial.printf("[mic] listening timeout (no keyword detected)\n");
  }
}

// ---- Keyword Detection (Placeholder for ML integration) ----
static void detectKeyword(const int16_t* samples, size_t count) {
  if (!g_listening || count == 0) return;
  
  // TODO: Integrate ML model here
  // For now, this is a framework for the user to add:
  // - Edge Impulse trained model (.tflite)
  // - Or custom keyword spotting algorithm
  // - Or frequency-domain analysis (MFCC)
  
  // Placeholder: Log audio characteristics for debugging
  static uint32_t sample_count = 0;
  sample_count += count;
  
  // Every 16000 samples (~1 second at 16kHz), print summary
  if (sample_count >= I2S_SAMPLE_RATE) {
    float rms = calculateRMS(samples, count);
    Serial.printf("[mic] 1s buffer: RMS=%.3f, total_samples=%u\n", rms, sample_count);
    sample_count = 0;
  }
}

// ---- Main loop: read audio and process ----
void loop() {
  if (!g_enabled) return;
  
  // Read audio samples from I2S
  esp_err_t ret = i2s_read(I2S_NUM, (void*)g_i2s_buffer, BUFFER_SIZE * sizeof(int16_t),
                           &g_bytes_read, portMAX_DELAY);
  
  if (ret != ESP_OK) {
    Serial.printf("[mic] i2s_read error: %d\n", ret);
    return;
  }
  
  size_t samples_read = g_bytes_read / sizeof(int16_t);
  
  // Calculate RMS (audio level)
  g_last_rms = calculateRMS(g_i2s_buffer, samples_read);
  
  // Voice Activity Detection
  updateVAD(g_last_rms);
  
  // Keyword detection
  detectKeyword(g_i2s_buffer, samples_read);
}

// ---- Shutdown ----
void end() {
  if (!g_enabled) return;
  
  i2s_stop(I2S_NUM);
  i2s_driver_uninstall(I2S_NUM);
  
  g_enabled = false;
  g_listening = false;
  Serial.println("[mic] stopped");
}

// ---- Status queries ----
bool isListening() {
  return g_listening;
}

bool isKeywordDetected() {
  return g_keyword_detected;
}

float getAudioLevel() {
  return g_last_rms;
}

uint32_t getListeningTime() {
  if (!g_listening) return 0;
  return millis() - g_listening_start_ms;
}

// ---- Control ----
void resetKeywordDetection() {
  g_keyword_detected = false;
}

void setListeningEnabled(bool en) {
  g_listening = en;
  if (en) {
    g_listening_start_ms = millis();
  }
}

// ---- Debug output ----
void printStatus() {
  Serial.printf("[mic] enabled=%d listening=%d detected=%d RMS=%.3f\n",
                g_enabled, g_listening, g_keyword_detected, g_last_rms);
}

} // namespace microphone

#endif // ENABLE_MICROPHONE
