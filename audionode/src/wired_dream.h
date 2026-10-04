#pragma once
// AudioNode TEST055: faster verified 2048-byte blocks before playback.
// Wiring: TX GPIO4 -> Listener RX15; RX GPIO6 <- Listener TX14;
// READY GPIO5 -> Listener GPIO21; GND -> GND. Logic is 3.3 V.
#include <Arduino.h>
#include <HardwareSerial.h>
#include <atomic>
#include <driver/i2s_std.h>
#include <esp_heap_caps.h>
#include <math.h>

namespace wiredDream {
constexpr int TX = 4, RX = 6, READY = 5;
constexpr uint32_t BAUD = 1000000;
constexpr uint32_t MAX_PCM_BYTES = 6U * 1024U * 1024U;
static HardwareSerial link(1);
static uint8_t *ring = nullptr;
static i2s_chan_handle_t i2sTx = nullptr;
static std::atomic<uint32_t> readAt{0}, writeAt{0};
static std::atomic<bool> runTask{false}, taskDone{true}, receiveDone{false};
static std::atomic<uint8_t> status{0}; // 0 idle, 1 waiting, 2 playing, 3 error
enum Fault : uint8_t {
  NoFault, NoMemory, I2SInit, I2SWrite, BadHeader, BadFrame,
  BufferFull, BadCRC, LinkTimeout, PlayerTask
};
static std::atomic<uint8_t> fault{NoFault};
static TaskHandle_t playerTask = nullptr;
static uint32_t expected = 0, received = 0, crc = 0xFFFFFFFFU, lastByteMs = 0;
static uint32_t totalBytes = 0, startByte = 0;
static uint32_t lastPingMs = 0, lastPongMs = 0;
static bool headerMode = false, trailerMode = false;
static String line;
static uint8_t frame[2056];
static uint16_t frameLen = 0, nextSeq = 0;
static bool frameStarted = false, frameEscape = false;
static uint32_t goodBlocks = 0, badBlocks = 0, transferMs = 0, requestStartMs = 0;
static std::atomic<uint8_t> volume{11};
static uint32_t crcByte(uint32_t value, uint8_t b) {
  value ^= b;
  for (unsigned bit = 0; bit < 8; ++bit)
    value = (value >> 1) ^ (0xEDB88320U & (0U - (value & 1U)));
  return value;
}
static void completeFrame() {
  if (frameLen < 8) return;
  const uint16_t seq = uint16_t(frame[0]) | uint16_t(frame[1]) << 8;
  const uint16_t bytes = uint16_t(frame[2]) | uint16_t(frame[3]) << 8;
  uint32_t check = 0xFFFFFFFFU;
  if (bytes && bytes <= 2048 && frameLen == bytes + 8) {
    for (uint16_t i = 0; i < bytes + 4; ++i) check = crcByte(check, frame[i]);
    check ^= 0xFFFFFFFFU;
    const uint32_t sent = uint32_t(frame[bytes + 4]) |
        (uint32_t(frame[bytes + 5]) << 8) |
        (uint32_t(frame[bytes + 6]) << 16) |
        (uint32_t(frame[bytes + 7]) << 24);
    if (check == sent) {
      // A lost ACK may cause a duplicate; acknowledge without appending it.
      if (seq == uint16_t(nextSeq - 1)) { link.printf("A %u\n", seq); return; }
      if (seq == nextSeq && bytes <= expected - received) {
        memcpy(ring + received, frame + 4, bytes);
        for (uint16_t i = 0; i < bytes; ++i) crc = crcByte(crc, frame[i + 4]);
        received += bytes;
        ++nextSeq; ++goodBlocks;
        link.printf("A %u\n", seq);
        if (received == expected) { digitalWrite(READY, LOW); trailerMode = true; }
        return;
      }
    }
  }
  ++badBlocks;
  link.printf("N %u\n", nextSeq);
}

// Listener sends 16-kHz, signed 16-bit stereo little-endian PCM.  The
// PCM5102A has no MCLK on this board, so its PLL derives a clock from BCK.
// At 16 kHz the DAC requires 64 BCK cycles per stereo frame (1.024 MHz).
// I2S 16-bit stereo provides only 32 BCK cycles (512 kHz), which the PLL
// does not support.  Send each 16-bit sample left-aligned in a 32-bit slot.
static void player(void *) {
  // Only one block on the task stack: 256 32-bit samples = 128 stereo frames.
  int32_t block[256];
  while (runTask.load()) {
    const uint32_t fill = writeAt.load() - readAt.load();
    status.store(2);
    size_t count = fill < sizeof(block) / 2 ? fill : sizeof(block) / 2;
    count &= ~size_t(3); // complete stereo input frames only
    if (!count) {
      if (receiveDone.load()) break;
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }
    const uint32_t rd = readAt.load();
    const int32_t gain = volume.load();
    for (size_t i = 0; i < count / 2; ++i) {
      const uint32_t pos = rd + 2 * i;
      const uint16_t bits = uint16_t(ring[pos]) |
                            (uint16_t(ring[pos + 1]) << 8);
      const int16_t sample = static_cast<int16_t>(bits);
      // Multiply instead of shifting a negative signed number.
      block[i] = ((int32_t(sample) * gain) / 21) * 65536;
    }
    const size_t outputBytes = count * 2; // 16-bit PCM -> 32-bit I2S slots
    size_t written = 0;
    if (i2s_channel_write(i2sTx, block, outputBytes, &written, 250) != ESP_OK ||
        written != outputBytes) {
      fault.store(I2SWrite);
      status.store(3); break;
    }
    readAt.store(rd + count); // transport position is still in source WAV bytes
  }
  if (status.load() != 3 && runTask.load()) status.store(0);
  taskDone.store(true);
  vTaskDelete(nullptr);
}

static bool begin() {
  pinMode(READY, OUTPUT);
  digitalWrite(READY, LOW);
  link.setRxBufferSize(16384);
  link.begin(BAUD, SERIAL_8N1, RX, TX);
  return heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >= 1024U * 1024U;
}

static bool stop(bool sendStop = true) {
  digitalWrite(READY, LOW);
  if (sendStop) link.print("S\n");
  runTask.store(false);
  const TickType_t waitTicks = pdMS_TO_TICKS(10) ? pdMS_TO_TICKS(10) : 1;
  for (unsigned i = 0; i < 120 && !taskDone.load(); ++i) vTaskDelay(waitTicks);
  // Never delete a channel while its playback task could still use it.
  if (!taskDone.load()) return false;
  playerTask = nullptr;
  if (i2sTx) {
    i2s_channel_disable(i2sTx);
    i2s_del_channel(i2sTx);
    i2sTx = nullptr;
  }
  if (ring) { heap_caps_free(ring); ring = nullptr; }
  // Give the Listener a chance to see STOP before a new request is sent.
  if (sendStop) vTaskDelay(pdMS_TO_TICKS(30) ? pdMS_TO_TICKS(30) : 1);
  while (link.available()) link.read();
  headerMode = trailerMode = false;
  frameStarted = frameEscape = false; frameLen = 0;
  line = "";
  status.store(0);
  return true;
}

static bool openOutput() {
  i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
  if (i2s_new_channel(&channel, &i2sTx, nullptr) != ESP_OK) {
    i2sTx = nullptr; fault.store(I2SInit); status.store(3); return false;
  }
  i2s_std_config_t cfg = {};
  cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000);
  cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                      I2S_SLOT_MODE_STEREO);
  cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
  cfg.gpio_cfg.bclk = gpio_num_t(14);
  cfg.gpio_cfg.ws = gpio_num_t(2);
  cfg.gpio_cfg.dout = gpio_num_t(13);
  cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
  if (i2s_channel_init_std_mode(i2sTx, &cfg) != ESP_OK ||
      i2s_channel_enable(i2sTx) != ESP_OK) {
    i2s_del_channel(i2sTx); i2sTx = nullptr;
    fault.store(I2SInit); status.store(3); return false;
  }
  return true;
}

// Direct DAC check: the same I2S output as dream playback, no Listener or cable.
static bool toneTest() {
  if (!stop(false)) return false;
  fault.store(NoFault);
  if (!openOutput()) return false;
  int32_t block[256];
  bool okay = true;
  for (unsigned chunk = 0; chunk < 100 && okay; ++chunk) {
    for (unsigned frame = 0; frame < 128; ++frame) {
      const float phase = 6.28318530718f * 440.0f * (chunk * 128 + frame) / 16000.0f;
      const int32_t sample = int32_t(sinf(phase) * 3500.0f) * 65536;
      block[2 * frame] = block[2 * frame + 1] = sample;
    }
    size_t written = 0;
    if (i2s_channel_write(i2sTx, block, sizeof(block), &written, 250) != ESP_OK ||
        written != sizeof(block)) okay = false;
  }
  i2s_channel_disable(i2sTx);
  i2s_del_channel(i2sTx);
  i2sTx = nullptr;
  status.store(0);
  return okay;
}

static bool start(uint8_t rank, uint16_t key, uint8_t targetVolume,
                  uint32_t offsetBytes = 0) {
  if (!stop()) { fault.store(PlayerTask); status.store(3); return false; }
  fault.store(NoFault);
  readAt.store(0); writeAt.store(0);
  expected = received = 0; crc = 0xFFFFFFFFU;
  nextSeq = 0; goodBlocks = badBlocks = 0;
  transferMs = 0; requestStartMs = millis();
  frameStarted = frameEscape = false; frameLen = 0;
  totalBytes = 0; startByte = offsetBytes;
  volume = targetVolume > 21 ? 21 : targetVolume;
  receiveDone.store(false);
  taskDone.store(true);
  if (!openOutput()) return false;
  status.store(1);
  headerMode = true; trailerMode = false;
  lastByteMs = millis();
  link.printf("P %u %04X %lu\n", rank, key, (unsigned long)offsetBytes);
  return true;
}

static void service() {
  if (status.load() == 0) {
    if (millis() - lastPingMs >= 2000) {
      lastPingMs = millis(); link.print("PING\n");
    }
    while (link.available()) {
      const char c = char(link.read());
      if (c == '\n') {
        line.trim();
        if (line == "PONG") lastPongMs = millis();
        line = "";
      } else if (line.length() < 48 && c != '\r') line += c;
      else if (line.length() >= 48) line = "";
    }
    return;
  }
  if (status.load() == 3) return;
  unsigned budget = 8192;
  while (link.available() && budget--) {
    const uint8_t b = uint8_t(link.read());
    lastByteMs = millis();
    if (headerMode || (trailerMode && !frameStarted && b != 0x7E)) {
      if (b == '\n') {
        line.trim();
        if (headerMode) {
          if (line == "PONG" || line == "ABORT") { line = ""; continue; }
          unsigned long bytes = 0, total = 0, offset = 0;
          if (sscanf(line.c_str(), "OK %lu %lu %lu", &bytes, &total, &offset) != 3 ||
              bytes == 0 || bytes % 4 || bytes > MAX_PCM_BYTES ||
              total < bytes || offset != startByte || total - offset != bytes) {
            fault.store(BadHeader); status.store(3); digitalWrite(READY, LOW); return;
          }
          expected = bytes; totalBytes = total; headerMode = false;
          ring = static_cast<uint8_t *>(heap_caps_malloc(expected, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
          if (!ring) {
            fault.store(NoMemory); status.store(3); digitalWrite(READY, LOW); return;
          }
          digitalWrite(READY, HIGH);
        } else {
          unsigned long remote = 0;
          if (sscanf(line.c_str(), "DONE %lx", &remote) != 1 ||
              uint32_t(remote) != (crc ^ 0xFFFFFFFFU)) {
            fault.store(BadCRC); status.store(3);
          }
          else {
            transferMs = millis() - requestStartMs;
            receiveDone.store(true);
            writeAt.store(received);
            runTask.store(true); taskDone.store(false);
            if (xTaskCreatePinnedToCore(player, "dream_pcm", 4096, nullptr, 2,
                                        &playerTask, 1) != pdPASS) {
              runTask.store(false); taskDone.store(true);
              fault.store(PlayerTask); status.store(3);
            }
          }
          trailerMode = false;
        }
        line = "";
      } else if (line.length() < 64 && b != '\r') line += char(b);
      else if (line.length() >= 64) {
        fault.store(BadFrame); status.store(3); digitalWrite(READY, LOW); return;
      }
      continue;
    }
    // 0x7E delimits a frame; 0x7D escapes either delimiter in the payload.
    // A damaged frame is discarded and the Listener retries the same sequence.
    if (b == 0x7E) {
      if (frameStarted && frameLen) {
        completeFrame();
        frameStarted = false;
      } else frameStarted = true;
      frameEscape = false; frameLen = 0;
      continue;
    }
    if (!frameStarted) continue;
    if (b == 0x7D && !frameEscape) { frameEscape = true; continue; }
    const uint8_t data = frameEscape ? (b ^ 0x20) : b;
    frameEscape = false;
    if (frameLen < sizeof(frame)) frame[frameLen++] = data;
    else { frameStarted = false; frameLen = 0; ++badBlocks; link.printf("N %u\n", nextSeq); }
  }
  if (!headerMode && !trailerMode && received < expected)
    digitalWrite(READY, HIGH);
  if (millis() - lastByteMs > 20000 && !receiveDone.load()) {
    fault.store(LinkTimeout); status.store(3); digitalWrite(READY, LOW);
  }
}
} // namespace wiredDream
// END AudioNode TEST055 faster verified dream transport.
