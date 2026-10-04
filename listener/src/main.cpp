// ============================================================
// BG BED LISTENER - TEST 030 - FASTER VERIFIED DREAM TRANSFER
// ============================================================

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include <SPI.h>
#include <driver/i2s_std.h>
#include <math.h>
#include <ctype.h>
#include <Preferences.h>
#include <WiFi.h>

#define TEST_NUMBER 30

static constexpr uint8_t NODE_LISTENER = 8;
static constexpr uint8_t NODE_SHEMI = 1;
static constexpr uint8_t NODE_IRA = 2;
static constexpr uint8_t TYPE_DREAM_CMD = 0x50;
static constexpr uint8_t TYPE_DREAM_ACK = 0x51;
static constexpr uint8_t DREAM_START = 1;
static constexpr uint8_t DREAM_STOP = 2;
static constexpr uint8_t DREAM_STATUS = 3;
static constexpr uint8_t DREAM_LIST = 4;
static constexpr uint8_t CATALOG_LIMIT = 64;

static constexpr int PIN_I2S_BCK = 40;
static constexpr int PIN_I2S_WS  = 41;
static constexpr int PIN_I2S_SD  = 42;
static constexpr int PIN_RS485_TX = 17;
static constexpr int PIN_RS485_RX = 18;
static constexpr int PIN_SD_CS   = 10;
static constexpr int PIN_SD_MOSI = 11;
static constexpr int PIN_SD_SCK  = 12;
static constexpr int PIN_SD_MISO = 13;
static constexpr int PIN_PRESS_LEFT_AO = 1;
static constexpr int PIN_PRESS_RIGHT_AO = 2;
static constexpr int PIN_PRESS_LEFT_DO = 7;
static constexpr int PIN_PRESS_RIGHT_DO = 8;
static constexpr int PIN_PIXELS = 47;
// Dedicated four-wire link to AudioNode: RX15<-TX4, TX14->RX6,
// READY21<-GPIO5, and GND. GPIO voltage is 3.3 V.
static constexpr int PIN_LINK_RX = 15;
static constexpr int PIN_LINK_TX = 14;
static constexpr int PIN_LINK_READY = 21;
static constexpr uint32_t LINK_BAUD = 1000000;
static constexpr int PIXEL_COUNT = 7;
static constexpr int PIX_SYSTEM = 0;
static constexpr int PIX_RS485 = 1;
static constexpr int PIX_MIC_LEFT = 2;
static constexpr int PIX_MIC_RIGHT = 3;
static constexpr int PIX_PRESS_LEFT = 4;
static constexpr int PIX_PRESS_RIGHT = 5;
static constexpr int PIX_SD = 6;

static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr uint16_t CHANNELS = 2;
static constexpr uint16_t BITS_PER_SAMPLE = 16;
static int recordGain = 2;
static constexpr size_t FRAMES_PER_BLOCK = 512;
static constexpr int BAR_WIDTH = 32;

static Adafruit_NeoPixel pixels(PIXEL_COUNT, PIN_PIXELS, NEO_GRBW + NEO_KHZ800);
static SPIClass sdSpi(FSPI);
static HardwareSerial rs485(2);
static HardwareSerial dreamLink(1);
static i2s_chan_handle_t i2sRx = nullptr;
static File wavFile;
static int32_t i2sBlock[FRAMES_PER_BLOCK * CHANNELS];
static int16_t pcmBlock[FRAMES_PER_BLOCK * CHANNELS];

static bool sdOkay = false;
static bool i2sOkay = false;
static bool recording = false;
static bool recordError = false;
static uint32_t audioBytes = 0;
static uint32_t clippedSamples = 0;
static uint32_t recordStartMs = 0;
static uint32_t lastHeaderMs = 0;
static uint32_t lastMeterMs = 0;
static uint32_t lastLampMs = 0;
static uint32_t micLeftRms = 0;
static uint32_t micRightRms = 0;
static uint32_t rs485LastMs = 0;
static uint8_t rs485Frame[14] = {};
static uint8_t rs485Pos = 0;
static bool rs485Collecting = false;
static uint32_t rs485ByteMs = 0;
static bool clockValid = false;
static uint16_t clockYear = 0;
static uint8_t clockMonth = 0;
static uint8_t clockDay = 0;
static uint8_t clockHour = 0;
static uint8_t clockMinute = 0;
static uint8_t clockSecond = 0;
static char currentFilename[28] = {};
static char latestFilename[28] = {};
struct CatalogEntry { char filename[28]; uint32_t bytes; };
static CatalogEntry catalog[CATALOG_LIMIT] = {};
static uint8_t catalogCount = 0;
static uint16_t recordingKey(const char *filename) {
  uint16_t crc = 0xFFFF;
  while (*filename) {
    crc ^= uint8_t(*filename++);
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}
static void catalogInsert(const char *name, uint32_t bytes) {
  uint8_t at = 0;
  while (at < catalogCount && strcmp(catalog[at].filename, name) > 0) ++at;
  if (at >= CATALOG_LIMIT) return;
  if (catalogCount < CATALOG_LIMIT) ++catalogCount;
  for (uint8_t j = catalogCount - 1; j > at; --j) catalog[j] = catalog[j-1];
  snprintf(catalog[at].filename, sizeof(catalog[at].filename), "%s", name);
  catalog[at].bytes = bytes;
}
static bool validCatalogName(const String &name) {
  if (name.length() != 18 && name.length() != 20) return false;
  if (name[0] != '/' || name[7] != '_' || !name.endsWith(".WAV")) return false;
  for (uint8_t i = 1; i <= 13; ++i)
    if (i != 7 && !isdigit(static_cast<unsigned char>(name[i]))) return false;
  if (name.length() == 20 && (name[14] != '_' || name[15] < '1' || name[15] > '9'))
    return false;
  return true;
}
static String terminalLine;
static bool wiredActive = false;
static File wiredFile;
static uint32_t wiredRemaining = 0;
static String wiredCommand;
static uint32_t wiredLastReadyMs = 0;
static uint32_t wiredCRC = 0xFFFFFFFFU;
// One verified 2048-byte frame in flight. An ACK permits the next SD read.
static uint8_t wiredBlock[2048];
static uint16_t wiredBlockLen = 0, wiredSeq = 0;
static bool wiredAwaitAck = false;
static uint8_t wiredRetries = 0;
static uint32_t wiredSentMs = 0, wiredStartedMs = 0, wiredRetryCount = 0;
static void stopWiredTransfer(bool completed);
static uint32_t blockCrcUpdate(uint32_t crc, uint8_t b) {
  crc ^= b;
  for (unsigned bit = 0; bit < 8; ++bit)
    crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
  return crc;
}
static void sendWiredBlock() {
  // Static storage avoids a 4 KB temporary on the ESP32 loop task's stack.
  static uint8_t frame[4120];
  size_t n = 0;
  uint32_t check = 0xFFFFFFFFU;
  auto put = [&](uint8_t b) {
    check = blockCrcUpdate(check, b);
    if (b == 0x7E || b == 0x7D) { frame[n++] = 0x7D; frame[n++] = b ^ 0x20; }
    else frame[n++] = b;
  };
  frame[n++] = 0x7E;
  put(uint8_t(wiredSeq)); put(uint8_t(wiredSeq >> 8));
  put(uint8_t(wiredBlockLen)); put(uint8_t(wiredBlockLen >> 8));
  for (uint16_t i = 0; i < wiredBlockLen; ++i) put(wiredBlock[i]);
  check ^= 0xFFFFFFFFU;
  for (unsigned i = 0; i < 4; ++i) {
    uint8_t b = uint8_t(check >> (8 * i));
    if (b == 0x7E || b == 0x7D) { frame[n++] = 0x7D; frame[n++] = b ^ 0x20; }
    else frame[n++] = b;
  }
  frame[n++] = 0x7E;
  if (dreamLink.write(frame, n) != n) {
    stopWiredTransfer(false); return;
  }
  dreamLink.flush();
  wiredSentMs = millis();
  wiredAwaitAck = true;
}
struct ReplyCache {
  uint16_t seq = 0;
  uint8_t command = 0;
  uint32_t atMs = 0;
  bool valid = false;
  uint8_t frame[14] = {};
};
static ReplyCache cachedReplies[2];
static bool highPassEnabled = true;
static constexpr int32_t HPF_ALPHA_Q15 = 31754;  // 0.9691, approximately 80 Hz at 16 kHz
struct HighPassState {
  int32_t previousInput = 0;
  int32_t previousOutput = 0;
};
static HighPassState highPassLeft;
static HighPassState highPassRight;

static uint32_t color(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0) {
  return pixels.Color(r, g, b, w);
}

static uint32_t spectrumColor(float level) {
  level = constrain(level, 0.0f, 1.0f);
  if (level < 0.33f) {
    const float t = level / 0.33f;
    return color(0, uint8_t(150 * t), uint8_t(150 * (1.0f - t)));
  }
  if (level < 0.66f) {
    const float t = (level - 0.33f) / 0.33f;
    return color(uint8_t(180 * t), 150, 0);
  }
  const float t = (level - 0.66f) / 0.34f;
  return color(180, uint8_t(150 * (1.0f - t)), 0);
}

static float microphoneLevel(uint32_t rms) {
  if (!rms) return 0.0f;
  const float db = 20.0f * log10f(float(rms) / 32767.0f);
  return constrain((db + 60.0f) / 50.0f, 0.0f, 1.0f);
}

static void updateDiagnosticLamps() {
  const int leftAdc = analogRead(PIN_PRESS_LEFT_AO);
  const int rightAdc = analogRead(PIN_PRESS_RIGHT_AO);
  const float leftForce = float(4095 - constrain(leftAdc, 0, 4095)) / 4095.0f;
  const float rightForce = float(4095 - constrain(rightAdc, 0, 4095)) / 4095.0f;
  pixels.clear();
  pixels.setPixelColor(PIX_SYSTEM, color(0, 150, 0));
  const bool rs485Recent = rs485LastMs && millis() - rs485LastMs < 2000;
  pixels.setPixelColor(PIX_RS485,
                       rs485Recent ? color(0, 150, 0) : color(160, 55, 0));
  pixels.setPixelColor(PIX_MIC_LEFT,
                       i2sOkay ? spectrumColor(microphoneLevel(micLeftRms))
                               : color(170, 0, 0));
  pixels.setPixelColor(PIX_MIC_RIGHT,
                       i2sOkay ? spectrumColor(microphoneLevel(micRightRms))
                               : color(170, 0, 0));
  pixels.setPixelColor(PIX_PRESS_LEFT, spectrumColor(leftForce));
  pixels.setPixelColor(PIX_PRESS_RIGHT, spectrumColor(rightForce));
  pixels.setPixelColor(PIX_SD, sdOkay ? color(0, 150, 0) : color(170, 0, 0));
  pixels.show();
}

static void startupLamps() {
  for (int blink = 0; blink < 2; ++blink) {
    pixels.fill(color(180, 0, 0));
    pixels.show();
    delay(500);
    pixels.clear();
    pixels.show();
    delay(250);
  }
}

static bool startRecording();
static bool stopRecording();
static void findLatestRecording();

static void replyToDreamCommand(const uint8_t *request) {
  const uint8_t source = request[1];
  if (request[2] != NODE_LISTENER || (source != NODE_SHEMI && source != NODE_IRA) ||
      request[3] != TYPE_DREAM_CMD) return;
  const uint8_t command = request[6];
  const uint16_t seq = uint16_t(request[4]) | (uint16_t(request[5]) << 8);
  ReplyCache &cache = cachedReplies[source - 1];
  if (cache.valid && cache.command == command && cache.seq == seq &&
      millis() - cache.atMs < 30000) {
    rs485.write(cache.frame, sizeof(cache.frame));
    rs485.flush();
    return;
  }
  uint8_t result = 0;
  if (command == DREAM_LIST) {
    const uint8_t rank = request[7], part = request[8];
    uint8_t response[14] = {0xA5, NODE_LISTENER, source, TYPE_DREAM_ACK,
                            request[4], request[5], command, 0, catalogCount,
                            0, 0, 0, 0, 0};
    if (!sdOkay) response[7] = 2;
    else if (rank >= catalogCount || part > 2) response[7] = 1;
    else {
      const char *name = catalog[rank].filename;
      unsigned yy=0, mo=0, dd=0, hh=0, mm=0, ss=0, suffix=0;
      if (sscanf(name, "/%2u%2u%2u_%2u%2u%2u_%u.WAV",
                 &yy, &mo, &dd, &hh, &mm, &ss, &suffix) != 7) {
        suffix = 0;
        sscanf(name, "/%2u%2u%2u_%2u%2u%2u.WAV",
               &yy, &mo, &dd, &hh, &mm, &ss);
      }
      if (part == 0) {
        response[9] = yy; response[10] = mo; response[11] = dd; response[12] = hh;
      } else if (part == 1) {
        response[9] = mm; response[10] = ss; response[11] = suffix;
      } else {
        const uint32_t size = catalog[rank].bytes;
        for (uint8_t i = 0; i < 4; ++i) response[9+i] = uint8_t(size >> (8*i));
      }
    }
    for (size_t i = 0; i < 13; ++i) response[13] ^= response[i];
    rs485.write(response, sizeof(response));
    rs485.flush();
    return;
  }
  if (command == DREAM_START) {
    if (recording || wiredActive || !sdOkay || !i2sOkay || !clockValid) result = 1;
    else if (!startRecording()) result = 2;
  } else if (command == DREAM_STOP) {
    if (!recording) result = 1;
    else if (!stopRecording()) result = 2;
  } else if (command == DREAM_STATUS) {
    if (!sdOkay || !i2sOkay || !clockValid) result = 1;
  } else if (command != DREAM_STATUS) {
    result = 2;
  }
  uint8_t response[14] = {0xA5, NODE_LISTENER, source, TYPE_DREAM_ACK,
                          request[4], request[5], command, result,
                          uint8_t(recording ? 1 : 0),
                          uint8_t(latestFilename[0] ? 1 : 0), 0, 0, 0, 0};
  // These two otherwise-unused ACK bytes carry the same microphone levels
  // that drive the Listener's diagnostic pixels (0..100, left then right).
  response[10] = uint8_t(microphoneLevel(micLeftRms) * 100.0f);
  response[11] = uint8_t(microphoneLevel(micRightRms) * 100.0f);
  for (size_t i = 0; i < sizeof(response) - 1; ++i) response[13] ^= response[i];
  memcpy(cache.frame, response, sizeof(response));
  cache.seq = seq;
  cache.command = command;
  cache.atMs = millis();
  cache.valid = true;
  rs485.write(response, sizeof(response));
  rs485.flush();
  Serial.printf("[DREAM ACK] panel=%u seq=%u cmd=%u result=%u recording=%u\n",
                unsigned(source), unsigned(seq), unsigned(command),
                unsigned(result), unsigned(recording));
}

static void serviceRS485() {
  while (rs485.available()) {
    const uint8_t value = uint8_t(rs485.read());
    rs485ByteMs = millis();
    if (!rs485Collecting) {
      if (value != 0xA5) continue;
      rs485Collecting = true;
      rs485Pos = 0;
    }
    rs485Frame[rs485Pos++] = value;
    if (rs485Pos == sizeof(rs485Frame)) {
      uint8_t checksum = 0;
      for (size_t i = 0; i < sizeof(rs485Frame) - 1; ++i) checksum ^= rs485Frame[i];
      if (checksum == rs485Frame[13]) {
        rs485LastMs = millis();
        replyToDreamCommand(rs485Frame);
        if (rs485Frame[3] == 0x10) {
          const uint16_t year = uint16_t(rs485Frame[6]) |
                                (uint16_t(rs485Frame[7]) << 8);
          const uint8_t month = rs485Frame[8];
          const uint8_t day = rs485Frame[9];
          const uint8_t hour = rs485Frame[10];
          const uint8_t minute = rs485Frame[11];
          const uint8_t second = rs485Frame[12];
          if (year >= 2024 && year <= 2099 && month >= 1 && month <= 12 &&
              day >= 1 && day <= 31 && hour <= 23 && minute <= 59 && second <= 59) {
            const bool firstTime = !clockValid;
            clockValid = true;
            clockYear = year;
            clockMonth = month;
            clockDay = day;
            clockHour = hour;
            clockMinute = minute;
            clockSecond = second;
            if (firstTime) {
              Serial.printf("[CLOCK] %04u-%02u-%02u %02u:%02u:%02u received from RS485.\n",
                            clockYear, clockMonth, clockDay,
                            clockHour, clockMinute, clockSecond);
            }
          }
        }
      }
      rs485Collecting = false;
      rs485Pos = 0;
    }
  }
  if (rs485Collecting && millis() - rs485ByteMs > 50) {
    rs485Collecting = false;
    rs485Pos = 0;
  }
}

static void put16(uint8_t *p, uint16_t value) {
  p[0] = uint8_t(value);
  p[1] = uint8_t(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
  p[0] = uint8_t(value);
  p[1] = uint8_t(value >> 8);
  p[2] = uint8_t(value >> 16);
  p[3] = uint8_t(value >> 24);
}

static bool writeWavHeader(File &file, uint32_t dataBytes) {
  uint8_t header[44] = {};
  memcpy(header + 0, "RIFF", 4);
  put32(header + 4, 36 + dataBytes);
  memcpy(header + 8, "WAVEfmt ", 8);
  put32(header + 16, 16);
  put16(header + 20, 1);
  put16(header + 22, CHANNELS);
  put32(header + 24, SAMPLE_RATE);
  put32(header + 28, SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8);
  put16(header + 32, CHANNELS * BITS_PER_SAMPLE / 8);
  put16(header + 34, BITS_PER_SAMPLE);
  memcpy(header + 36, "data", 4);
  put32(header + 40, dataBytes);
  if (!file.seek(0)) return false;
  return file.write(header, sizeof(header)) == sizeof(header);
}

static bool startMicrophones() {
  i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chan.auto_clear = false;
  if (i2s_new_channel(&chan, nullptr, &i2sRx) != ESP_OK) return false;
  i2s_std_config_t cfg = {};
  cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE);
  cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
      I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO);
  cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
  cfg.gpio_cfg.bclk = gpio_num_t(PIN_I2S_BCK);
  cfg.gpio_cfg.ws = gpio_num_t(PIN_I2S_WS);
  cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
  cfg.gpio_cfg.din = gpio_num_t(PIN_I2S_SD);
  cfg.gpio_cfg.invert_flags.mclk_inv = false;
  cfg.gpio_cfg.invert_flags.bclk_inv = false;
  cfg.gpio_cfg.invert_flags.ws_inv = false;
  if (i2s_channel_init_std_mode(i2sRx, &cfg) != ESP_OK) return false;
  if (i2s_channel_enable(i2sRx) != ESP_OK) return false;
  size_t discarded = 0;
  for (int i = 0; i < 8; ++i)
    i2s_channel_read(i2sRx, i2sBlock, sizeof(i2sBlock), &discarded, 100);
  return true;
}

static void resetHighPass() {
  highPassLeft = {};
  highPassRight = {};
}

static int32_t applyHighPass(int32_t input, HighPassState &state) {
  if (!highPassEnabled) return input;
  const int32_t output = input - state.previousInput +
      int32_t((int64_t(HPF_ALPHA_Q15) * state.previousOutput) >> 15);
  state.previousInput = input;
  state.previousOutput = output;
  return output;
}

static int16_t convertSample(int32_t raw, HighPassState &filterState,
                             bool countClipping) {
  const int32_t sample24 = applyHighPass(raw >> 8, filterState);
  const int32_t amplified = (sample24 * recordGain) >> 8;
  if (amplified > 32767) {
    if (countClipping) ++clippedSamples;
    return 32767;
  }
  if (amplified < -32768) {
    if (countClipping) ++clippedSamples;
    return -32768;
  }
  return int16_t(amplified);
}

static bool chooseFilename() {
  if (!clockValid) return false;
  snprintf(currentFilename, sizeof(currentFilename),
           "/%02u%02u%02u_%02u%02u%02u.WAV",
           unsigned(clockYear % 100), unsigned(clockMonth), unsigned(clockDay),
           unsigned(clockHour), unsigned(clockMinute), unsigned(clockSecond));
  if (!SD.exists(currentFilename)) return true;
  for (unsigned suffix = 1; suffix <= 9; ++suffix) {
    snprintf(currentFilename, sizeof(currentFilename),
             "/%02u%02u%02u_%02u%02u%02u_%u.WAV",
             unsigned(clockYear % 100), unsigned(clockMonth), unsigned(clockDay),
             unsigned(clockHour), unsigned(clockMinute), unsigned(clockSecond), suffix);
    if (!SD.exists(currentFilename)) return true;
  }
  return false;
}

static bool refreshHeader() {
  if (!wavFile) return false;
  const uint32_t endPosition = 44 + audioBytes;
  if (!writeWavHeader(wavFile, audioBytes)) return false;
  if (!wavFile.seek(endPosition)) return false;
  wavFile.flush();
  return true;
}

static void printBytes(uint64_t bytes) {
  if (bytes >= 1024ULL * 1024ULL * 1024ULL)
    Serial.printf("%.2f GB", double(bytes) / double(1024ULL * 1024ULL * 1024ULL));
  else if (bytes >= 1024ULL * 1024ULL)
    Serial.printf("%.2f MB", double(bytes) / double(1024ULL * 1024ULL));
  else if (bytes >= 1024ULL)
    Serial.printf("%.1f KB", double(bytes) / 1024.0);
  else
    Serial.printf("%llu B", (unsigned long long)bytes);
}

static void reportSdSpace() {
  const uint64_t total = SD.totalBytes();
  const uint64_t used = SD.usedBytes();
  Serial.print(F("[SD] Card: "));
  printBytes(SD.cardSize());
  Serial.print(F("  Used: "));
  printBytes(used);
  Serial.print(F("  Free: "));
  printBytes(total > used ? total - used : 0);
  Serial.println();
}

static void listSdDirectory() {
  if (recording) {
    Serial.println(F("[SD DIR] Recording active. Type s first."));
    return;
  }
  if (!sdOkay) {
    Serial.println(F("[SD DIR] SD card not ready. Type sd to retry."));
    return;
  }
  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println(F("[SD DIR] ERROR: cannot open root directory."));
    if (root) root.close();
    sdOkay = false;
    return;
  }
  Serial.println(F("========== SD CARD DIRECTORY =========="));
  uint32_t count = 0;
  uint64_t bytes = 0;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    Serial.print(entry.isDirectory() ? F("[DIR]  ") : F("[FILE] "));
    Serial.print(entry.name());
    if (!entry.isDirectory()) {
      const uint64_t size = entry.size();
      ++count;
      bytes += size;
      Serial.print(F("  "));
      printBytes(size);
    }
    Serial.println();
    entry.close();
  }
  root.close();
  Serial.printf("[SD DIR] %lu files, ", (unsigned long)count);
  printBytes(bytes);
  Serial.println(F(" listed."));
  reportSdSpace();
  Serial.println(F("======================================="));
}

static void recheckSd() {
  if (recording || wiredActive) {
    Serial.println(F("[SD] Recording or playback active. Try again later."));
    return;
  }
  SD.end();
  delay(100);
  sdOkay = SD.begin(PIN_SD_CS, sdSpi, 4000000);
  if (!sdOkay) {
    Serial.println(F("[SD] ERROR: card initialization failed."));
    return;
  }
  Serial.println(F("[SD] Card initialized successfully."));
  reportSdSpace();
  findLatestRecording();
}

static void printMenu() {
  Serial.println(F("Commands:"));
  Serial.println(F("  r  start a new stereo WAV recording"));
  Serial.println(F("  s  stop, finalize and save the WAV file"));
  Serial.println(F("  dir  list recordings, sizes and free space"));
  Serial.println(F("  sd   reinitialize and report the SD card"));
  Serial.println(F("  hpf on   enable the 80 Hz microphone filter"));
  Serial.println(F("  hpf off  disable it for a comparison recording"));
  Serial.println(F("  wired status   show dedicated AudioNode link"));
  Serial.println(F("  1, 2, 4 or 8  select recording gain before starting"));
  Serial.println(F("  ?  show this menu"));
}

static bool startRecording() {
  if (recording || wiredActive) {
    Serial.println(F("[REC] Recording/playback is active. Try again later."));
    return false;
  }
  if (!sdOkay || !i2sOkay) {
    Serial.println(F("[REC ERROR] SD card or microphones are not ready."));
    return false;
  }
  if (!clockValid) {
    Serial.println(F("[REC ERROR] Waiting for a valid RS485 TIME frame."));
    return false;
  }
  if (!chooseFilename()) {
    Serial.println(F("[REC ERROR] Timestamp filename and suffixes already exist."));
    return false;
  }
  wavFile = SD.open(currentFilename, FILE_WRITE);
  if (!wavFile || !writeWavHeader(wavFile, 0) || !wavFile.seek(44)) {
    Serial.println(F("[REC ERROR] Could not create WAV file."));
    if (wavFile) wavFile.close();
    return false;
  }
  audioBytes = 0;
  recordError = false;
  clippedSamples = 0;
  recordStartMs = millis();
  lastHeaderMs = millis();
  lastMeterMs = millis();
  recording = true;
  updateDiagnosticLamps();
  Serial.printf("[RECORDING] %s started at gain x%d, 80 Hz HPF %s. LEFT and RIGHT are separate stereo channels.\n",
                currentFilename, recordGain, highPassEnabled ? "ON" : "OFF");
  Serial.println(F("Speak near each microphone. Type s to stop and save."));
  return true;
}

static bool stopRecording() {
  if (!recording) {
    Serial.println(F("[REC] Nothing is being recorded. Type r to start."));
    return false;
  }
  recording = false;
  const bool headerOkay = refreshHeader();
  wavFile.close();
  const bool savedOkay = headerOkay && !recordError && audioBytes > 0;
  if (savedOkay) {
    catalogInsert(currentFilename, audioBytes + 44);
    if (catalogCount) snprintf(latestFilename, sizeof(latestFilename), "%s", catalog[0].filename);
  }
  else {
    // Keep a failed capture recoverable but never advertise it for playback.
    char failedFilename[sizeof(currentFilename)] = {};
    snprintf(failedFilename, sizeof(failedFilename), "%s", currentFilename);
    char *extension = strrchr(failedFilename, '.');
    if (extension) {
      strcpy(extension, ".BAD");
      if (!SD.rename(currentFilename, failedFilename))
        Serial.println(F("[REC ERROR] Could not rename incomplete WAV to .BAD."));
    }
  }
  updateDiagnosticLamps();
  const float seconds = float(audioBytes) /
      float(SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8));
  Serial.printf("[%s] %s  %.1f seconds  %lu audio bytes\n",
                savedOkay ? "SAVED" : "REC ERROR", currentFilename,
                seconds, (unsigned long)audioBytes);
  Serial.printf("[QUALITY] clipped samples=%lu  header=%s\n",
                (unsigned long)clippedSamples, headerOkay ? "OK" : "ERROR");
  if (savedOkay) Serial.println(F("Recording finalized on SD. AudioNode can request it over the dedicated cable."));
  Serial.println(F("Type r to make another recording."));
  return savedOkay;
}

static void findLatestRecording() {
  latestFilename[0] = '\0';
  catalogCount = 0;
  if (!sdOkay) return;
  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return;
  }
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory() && entry.size() >= 44) {
      String name = entry.name();
      if (!name.startsWith("/")) name = "/" + name;
      const bool stamped = validCatalogName(name);
      uint8_t header[12] = {};
      const bool validHeader = entry.read(header, sizeof(header)) == sizeof(header) &&
                               memcmp(header, "RIFF", 4) == 0 &&
                               memcmp(header + 8, "WAVE", 4) == 0;
      if (stamped && validHeader && name.length() < sizeof(latestFilename)) {
        catalogInsert(name.c_str(), entry.size());
      }
    }
    entry.close();
  }
  root.close();
  if (catalogCount) snprintf(latestFilename, sizeof(latestFilename), "%s", catalog[0].filename);
  if (latestFilename[0]) Serial.printf("[DREAM] Latest finalized: %s\n", latestFilename);
}

static void stopWiredTransfer(bool completed) {
  if (!wiredActive) return;
  wiredActive = false;
  if (wiredFile) wiredFile.close();
  if (completed) {
    dreamLink.printf("DONE %08lX\n", (unsigned long)(wiredCRC ^ 0xFFFFFFFFU));
    Serial.printf("[WIRED] Complete; %u blocks, %lu retries, %lu ms, CRC32 sent.\n",
                  wiredSeq, (unsigned long)wiredRetryCount,
                  (unsigned long)(millis() - wiredStartedMs));
  } else {
    dreamLink.println(F("ABORT"));
    Serial.printf("[WIRED] Transfer aborted at block %u, remaining=%lu.\n",
                  wiredSeq, (unsigned long)wiredRemaining);
  }
  wiredRemaining = 0;
  wiredAwaitAck = false;
  wiredBlockLen = 0;
}

static void handleWiredCommand(String command) {
  command.trim();
  unsigned ackSeq = 0;
  if (wiredActive && wiredAwaitAck &&
      sscanf(command.c_str(), "A %u", &ackSeq) == 1 && ackSeq == wiredSeq) {
    for (uint16_t i = 0; i < wiredBlockLen; ++i)
      wiredCRC = blockCrcUpdate(wiredCRC, wiredBlock[i]);
    wiredRemaining -= wiredBlockLen;
    wiredAwaitAck = false;
    ++wiredSeq;
    if (!wiredRemaining) stopWiredTransfer(true);
    return;
  }
  if (wiredActive && wiredAwaitAck &&
      sscanf(command.c_str(), "N %u", &ackSeq) == 1 && ackSeq == wiredSeq) {
    ++wiredRetryCount;
    Serial.printf("[WIRED] Retry block %u (receiver rejected).\n", wiredSeq);
    if (++wiredRetries > 8) stopWiredTransfer(false);
    else sendWiredBlock();
    return;
  }
  if (command == "PING") { dreamLink.println(F("PONG")); return; }
  if (command == "S") { stopWiredTransfer(false); return; }
  unsigned rank = 0, key = 0;
  unsigned long startByte = 0;
  if (sscanf(command.c_str(), "P %u %x %lu", &rank, &key, &startByte) != 3 ||
      rank > 255 || key > 0xFFFF) {
    dreamLink.println(F("ERR command")); return;
  }
  if (wiredActive) stopWiredTransfer(false);
  if (recording || !sdOkay || !catalogCount ||
      (rank != 255 && (rank >= catalogCount ||
                       recordingKey(catalog[rank].filename) != key))) {
    dreamLink.println(F("ERR busy-or-file")); return;
  }
  const char *filename = rank == 255 ? latestFilename : catalog[rank].filename;
  wiredFile = SD.open(filename, FILE_READ);
  if (!wiredFile || wiredFile.size() < 48) {
    if (wiredFile) wiredFile.close();
    dreamLink.println(F("ERR wav")); return;
  }
  uint8_t header[44] = {};
  if (wiredFile.read(header, sizeof(header)) != sizeof(header) ||
      memcmp(header, "RIFF", 4) || memcmp(header + 8, "WAVE", 4) ||
      header[22] != 2 || header[23] || header[24] != 0x80 || header[25] != 0x3E ||
      header[34] != 16 || header[35]) {
    wiredFile.close(); dreamLink.println(F("ERR format")); return;
  }
  const uint32_t totalBytes = wiredFile.size() - 44;
  if (startByte >= totalBytes || (startByte & 3U) ||
      !wiredFile.seek(44 + startByte)) {
    wiredFile.close(); dreamLink.println(F("ERR seek")); return;
  }
  wiredRemaining = totalBytes - startByte;
  wiredCRC = 0xFFFFFFFFU;
  wiredSeq = 0; wiredBlockLen = 0; wiredAwaitAck = false; wiredRetries = 0;
  wiredRetryCount = 0;
  wiredLastReadyMs = wiredStartedMs = millis();
  wiredActive = true;
  dreamLink.printf("OK %lu %lu %lu\n", (unsigned long)wiredRemaining,
                   (unsigned long)totalBytes, startByte);
  Serial.printf("[WIRED] Sending %s offset=%lu total=%lu PCM bytes\n", filename,
                startByte, (unsigned long)totalBytes);
}

static void serviceWiredLink() {
  while (dreamLink.available()) {
    const char c = char(dreamLink.read());
    if (c == '\n' || c == '\r') {
      if (wiredCommand.length()) {
        handleWiredCommand(wiredCommand);
        wiredCommand = "";
      }
    } else if (wiredCommand.length() < 48) wiredCommand += c;
    else wiredCommand = "";
  }
  if (!wiredActive) return;
  if (wiredAwaitAck) {
    if (millis() - wiredSentMs > 1500) {
      ++wiredRetryCount;
      Serial.printf("[WIRED] Retry block %u (ACK timeout).\n", wiredSeq);
      if (++wiredRetries > 8) stopWiredTransfer(false);
      else sendWiredBlock();
    }
    return;
  }
  if (digitalRead(PIN_LINK_READY) != HIGH) {
    if (millis() - wiredLastReadyMs > 5000) stopWiredTransfer(false);
    return;
  }
  wiredLastReadyMs = millis();
  const size_t want = wiredRemaining < sizeof(wiredBlock) ? wiredRemaining : sizeof(wiredBlock);
  const size_t got = wiredFile.read(wiredBlock, want);
  if (got != want) {
    stopWiredTransfer(false); return;
  }
  wiredBlockLen = got;
  wiredRetries = 0;
  sendWiredBlock();
}

static void handleTerminalCommand(String command) {
  command.trim();
  command.toLowerCase();
  if (command == "wired status")
    Serial.printf("[WIRED] %s RX=%d TX=%d READY=%d Wi-Fi=OFF\n",
                  wiredActive ? "SENDING" : "IDLE", PIN_LINK_RX, PIN_LINK_TX,
                  PIN_LINK_READY);
  else if (command == "r") startRecording();
  else if (command == "s") stopRecording();
  else if (command == "dir" && !wiredActive) listSdDirectory();
  else if (command == "sd" && !wiredActive) recheckSd();
  else if ((command == "dir" || command == "sd") && wiredActive)
    Serial.println(F("[WIRED] SD card busy sending a recording."));
  else if ((command == "hpf on" || command == "hpf off") && !recording) {
    highPassEnabled = command == "hpf on";
    resetHighPass();
    Serial.printf("[FILTER] 80 Hz high-pass filter %s.\n",
                  highPassEnabled ? "ON" : "OFF");
  }
  else if ((command == "1" || command == "2" || command == "4" || command == "8")
           && !recording) {
    recordGain = command.charAt(0) - '0';
    Serial.printf("[GAIN] Recording gain set to x%d.\n", recordGain);
  }
  else if (command == "?") printMenu();
  else Serial.println(F("[COMMAND] Unknown. Type ? for the menu."));
}

static void printMeter(const char *label, uint32_t rms) {
  const float db = rms ? 20.0f * log10f(float(rms) / 32767.0f) : -90.0f;
  int filled = int((db + 60.0f) * BAR_WIDTH / 60.0f);
  filled = constrain(filled, 0, BAR_WIDTH);
  Serial.printf("%-9s [", label);
  for (int i = 0; i < BAR_WIDTH; ++i) Serial.print(i < filled ? '#' : '.');
  Serial.printf("] %6.1f dBFS\n", db);
}

static void serviceAudio(bool saveToFile) {
  size_t got = 0;
  if (i2s_channel_read(i2sRx, i2sBlock, sizeof(i2sBlock), &got, 100) != ESP_OK)
    return;
  const size_t samples = got / sizeof(int32_t);
  uint64_t leftSquares = 0;
  uint64_t rightSquares = 0;
  size_t frames = 0;
  for (size_t i = 0; i + 1 < samples; i += 2) {
    const int16_t left = convertSample(i2sBlock[i], highPassLeft, saveToFile);
    const int16_t right = convertSample(i2sBlock[i + 1], highPassRight, saveToFile);
    pcmBlock[i] = left;
    pcmBlock[i + 1] = right;
    leftSquares += uint64_t(int32_t(left) * int32_t(left));
    rightSquares += uint64_t(int32_t(right) * int32_t(right));
    ++frames;
  }
  if (frames) {
    const uint32_t leftNow = uint32_t(sqrt(double(leftSquares) / frames));
    const uint32_t rightNow = uint32_t(sqrt(double(rightSquares) / frames));
    micLeftRms = (micLeftRms * 3 + leftNow) / 4;
    micRightRms = (micRightRms * 3 + rightNow) / 4;
  }
  if (saveToFile) {
    const size_t bytes = samples * sizeof(int16_t);
    const size_t written = wavFile.write(reinterpret_cast<uint8_t *>(pcmBlock), bytes);
    audioBytes += written;
    if (written != bytes) {
      Serial.println(F("[REC ERROR] SD write failed. Recording stopped."));
      recordError = true;
      stopRecording();
      return;
    }
  }
  if (saveToFile && millis() - lastMeterMs >= 500 && frames) {
    lastMeterMs = millis();
    printMeter("MIC LEFT", uint32_t(sqrt(double(leftSquares) / frames)));
    printMeter("MIC RIGHT", uint32_t(sqrt(double(rightSquares) / frames)));
    Serial.printf("[REC] %.1f seconds  file=%s\n\n",
                  float(millis() - recordStartMs) / 1000.0f, currentFilename);
  }
  if (saveToFile && millis() - lastHeaderMs >= 1000) {
    lastHeaderMs = millis();
    if (!refreshHeader()) {
      Serial.println(F("[REC ERROR] Could not update WAV header."));
      recordError = true;
      stopRecording();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println(F("============================================================"));
  Serial.println(F(" BG BED LISTENER TEST 030 - FASTER VERIFIED TRANSFER"));
  Serial.println(F("============================================================"));
  pixels.begin();
  pixels.setBrightness(40);
  startupLamps();
  sdSpi.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  sdOkay = SD.begin(PIN_SD_CS, sdSpi, 4000000);
  Serial.printf("[SD] CS10 MOSI11 SCK12 MISO13: %s\n", sdOkay ? "READY" : "FAILED");
  findLatestRecording();
  i2sOkay = startMicrophones();
  Serial.printf("[MICROPHONES] BCLK40 WS41 SD42: %s\n",
                i2sOkay ? "READY" : "FAILED");
  rs485.begin(115200, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
  Serial.println(F("[RS485] RX=GPIO18 TX=GPIO17, receive monitor active"));
  pinMode(PIN_PRESS_LEFT_DO, INPUT);
  pinMode(PIN_PRESS_RIGHT_DO, INPUT);
  analogReadResolution(12);
  updateDiagnosticLamps();
  Serial.println(F("LED VU: blue=very low, green=low, yellow=medium, red=high."));
  Serial.println(F("Stereo WAV: 16 kHz, 16-bit. Default recording gain x2."));
  Serial.println(F("[FILTER] 80 Hz high-pass filter ON for both microphones."));
  Serial.println(F("Timestamp filename is used after a valid RS485 TIME frame arrives."));
  WiFi.mode(WIFI_OFF);
  Preferences oldWifi;
  if (oldWifi.begin("dream-wifi", false)) {
    if (oldWifi.getString("ssid", "").length() ||
        oldWifi.getString("token", "").length()) oldWifi.clear();
    oldWifi.end();
  }
  pinMode(PIN_LINK_READY, INPUT);
  dreamLink.setRxBufferSize(4096);
  dreamLink.begin(LINK_BAUD, SERIAL_8N1, PIN_LINK_RX, PIN_LINK_TX);
  Serial.println(F("[WIRED] RX15 TX14 READY21, Wi-Fi radio OFF and old credentials erased."));
  printMenu();
}

void loop() {
  serviceRS485();
  while (Serial.available()) {
    const char c = char(Serial.read());
    if (c == '\n' || c == '\r') {
      if (terminalLine.length()) {
        handleTerminalCommand(terminalLine);
        terminalLine = "";
      }
    } else if (terminalLine.length() < 160) {
      terminalLine += c;
    }
  }
  serviceWiredLink();
  if (i2sOkay && !wiredActive) serviceAudio(recording);
  else delay(1);
  if (millis() - lastLampMs >= 50) {
    lastLampMs = millis();
    updateDiagnosticLamps();
  }
}

// ============================================================
// END BG BED LISTENER - TEST 030 - FASTER VERIFIED DREAM TRANSFER
// ============================================================
