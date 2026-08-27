// ============================================================
// REMOTE 24 - TEST 004  (A7105 scanner + packet sniffer)
// ============================================================
//
// Board:  ESP32-S3-DevKitC-1 N16R8
// Module: A7105-8928  (2.4 GHz, with PA and LNA)
//
// WIRING - unchanged from TEST 001
//   VCC 3V3 only   GND   SCK 12   SDIO 13   SCS 14
//   TXEN 1   RXEN 2   GIO1/GIO2 not connected
//
// WHAT THIS DOES
//   Puts the radio in receive mode, walks all 256 channels, and
//   reads the RSSI ADC on each one. Compare a quiet baseline against
//   a scan taken while the bed remote is transmitting: the channel
//   the remote uses will stand out.
//
//   This is ENERGY DETECTION, not packet decoding. It finds WHERE
//   the remote talks. It does not decode WHAT it says - that needs
//   the data rate, packet length, ID and CRC to all match, and it
//   comes after.
//
// WHAT CHANGED vs TEST 003
//   Bug fix, and it was blocking everything. The serial monitor sends
//   CR followed by LF. handleLine() fires on the CR and the LF is left
//   sitting in the buffer, so every loop that waits on
//   "while (!Serial.available())" saw that stray byte and exited on its
//   first pass. m, p and pa all quit instantly without measuring once.
//   Every blocking loop now drains the buffer before it starts.
//
// WHAT CHANGED IN TEST 003 vs TEST 002
//   TEST 002 reported "nothing moved" and that result was useless,
//   because it never printed the raw numbers. A dead ADC and a quiet
//   band look identical when only the delta is shown. Baseline now
//   prints every value.
//
//   More importantly: the RSSI approach in TEST 002 was MY deduction.
//   The working implementation this was supposedly based on does not
//   read register 0x1E at all - it detects PACKETS. That method is
//   now here as well, and it is the one with a track record:
//       strobe RX, wait 3 ms, if register 0x00 bit 0 is clear a
//       packet landed, then read 16 bytes out of the FIFO at 0x05.
//
// THE CATCH WITH PACKET SNIFFING
//   The A7105 filters incoming packets by ID and CRC. We do not know
//   the bed remote's ID, so it may reject everything it hears. If the
//   sniffer stays silent everywhere, that is the likely reason, not
//   an absence of signal. Register 0x1F controls ID length and CRC -
//   loosening it is the next lever, and I want to read the datasheet
//   properly before touching it rather than guess again.
//
// SOURCE OF THE INIT SEQUENCE
//   The register writes, the IF filter calibration and the VCO bank
//   calibration below follow a working public A7105 implementation
//   (coptermanager-simple), not the datasheet read cold. Register
//   0x0F is the channel select, 0x1E is the RSSI ADC, 0x02 drives
//   both calibrations, 0x22 bit 4 and 0x25 bit 3 are the failure
//   flags.
//
// WIFI SITS IN THIS BAND TOO
//   Channels will not be quiet. A busy router shows up as a broad
//   raised region. The remote should appear as a narrow spike that
//   is present only while a button is held.
//
// NOT YET RUN ON HARDWARE.
//
// ============================================================

#include <Arduino.h>

#define TEST_NUMBER 4

// ------------------------------------------------------------
// Pins
// ------------------------------------------------------------

const int PIN_SCK  = 12;
const int PIN_SDIO = 13;
const int PIN_SCS  = 14;
const int PIN_TXEN = 1;
const int PIN_RXEN = 2;

// ------------------------------------------------------------
// A7105 registers and strobes
// ------------------------------------------------------------

const uint8_t REG_MODE         = 0x00;
const uint8_t REG_MODE_CONTROL = 0x01;
const uint8_t REG_CALC         = 0x02;
const uint8_t REG_FIFO_I       = 0x03;
const uint8_t REG_ID           = 0x06;
const uint8_t REG_CLOCK        = 0x0D;
const uint8_t REG_DATA_RATE    = 0x0E;
const uint8_t REG_CHANNEL      = 0x0F;
const uint8_t REG_RX           = 0x18;
const uint8_t REG_RX_GAIN_I    = 0x19;
const uint8_t REG_RX_GAIN_IV   = 0x1C;
const uint8_t REG_ADC          = 0x1E;
const uint8_t REG_IF_CALIB_I   = 0x22;
const uint8_t REG_VCO_SBCAL_I  = 0x25;

const uint8_t READ_BIT = 0x40;

const uint8_t STROBE_STANDBY = 0xA0;
const uint8_t STROBE_PLL     = 0xB0;
const uint8_t STROBE_RX      = 0xC0;

const uint8_t MASK_FBCF = 1 << 4;   // IF filter calibration failed
const uint8_t MASK_VBCF = 1 << 3;   // VCO bank calibration failed

const uint32_t CHIP_ID = 0x5475C52AUL;

// ------------------------------------------------------------
// Bit-banged SPI - unchanged from TEST 001, which passed
// ------------------------------------------------------------

void spiWriteByte(uint8_t value) {
  pinMode(PIN_SDIO, OUTPUT);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_SCK, LOW);
    digitalWrite(PIN_SDIO, (value >> i) & 0x01);
    delayMicroseconds(1);
    digitalWrite(PIN_SCK, HIGH);
    delayMicroseconds(1);
  }
  digitalWrite(PIN_SCK, LOW);
}

uint8_t spiReadByte() {
  uint8_t value = 0;
  pinMode(PIN_SDIO, INPUT);
  delayMicroseconds(2);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_SCK, LOW);
    delayMicroseconds(1);
    if (digitalRead(PIN_SDIO)) value |= (1 << i);
    digitalWrite(PIN_SCK, HIGH);
    delayMicroseconds(1);
  }
  digitalWrite(PIN_SCK, LOW);
  return value;
}

void writeReg(uint8_t addr, uint8_t value) {
  digitalWrite(PIN_SCS, LOW);
  spiWriteByte(addr);
  spiWriteByte(value);
  digitalWrite(PIN_SCS, HIGH);
}

uint8_t readReg(uint8_t addr) {
  digitalWrite(PIN_SCS, LOW);
  spiWriteByte(addr | READ_BIT);
  uint8_t value = spiReadByte();
  digitalWrite(PIN_SCS, HIGH);
  return value;
}

void strobe(uint8_t cmd) {
  digitalWrite(PIN_SCS, LOW);
  spiWriteByte(cmd);
  digitalWrite(PIN_SCS, HIGH);
}

void writeID(uint32_t id) {
  digitalWrite(PIN_SCS, LOW);
  spiWriteByte(REG_ID);
  spiWriteByte((id >> 24) & 0xFF);
  spiWriteByte((id >> 16) & 0xFF);
  spiWriteByte((id >>  8) & 0xFF);
  spiWriteByte((id      ) & 0xFF);
  digitalWrite(PIN_SCS, HIGH);
}

// ------------------------------------------------------------
// Calibration
// ------------------------------------------------------------

bool calibrateIF() {
  writeReg(REG_CALC, 0x01);
  unsigned long start = millis();
  while (millis() - start < 500) {
    if (readReg(REG_CALC) == 0) break;
  }
  if (millis() - start >= 500) {
    Serial.println(F("  IF filter calibration TIMED OUT"));
    return false;
  }
  if (readReg(REG_IF_CALIB_I) & MASK_FBCF) {
    Serial.println(F("  IF filter calibration FAILED"));
    return false;
  }
  Serial.println(F("  IF filter calibration ok"));
  return true;
}

bool calibrateVCO(uint8_t channel) {
  writeReg(REG_CHANNEL, channel);
  writeReg(REG_CALC, 0x02);
  unsigned long start = millis();
  while (millis() - start < 500) {
    if (readReg(REG_CALC) == 0) break;
  }
  if (millis() - start >= 500) {
    Serial.printf("  VCO calibration TIMED OUT on channel 0x%02X\n", channel);
    return false;
  }
  if (readReg(REG_VCO_SBCAL_I) & MASK_VBCF) {
    Serial.printf("  VCO calibration FAILED on channel 0x%02X\n", channel);
    return false;
  }
  Serial.printf("  VCO calibration ok on channel 0x%02X\n", channel);
  return true;
}

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------

bool radioInit() {
  Serial.println();
  Serial.println(F("--- initialising A7105 ---"));

  writeReg(REG_MODE, 0x00);          // reset
  delayMicroseconds(200);

  writeID(CHIP_ID);

  writeReg(REG_MODE_CONTROL, 0x63);  // auto RSSI, auto IF offset, ADC on
  writeReg(REG_FIFO_I,       0x0F);  // 16 byte FIFO
  writeReg(REG_CLOCK,        0x05);  // crystal, clock divider 2
  writeReg(REG_DATA_RATE,    0x04);
  writeReg(REG_RX,           0x62);  // 500 kHz bandwidth
  writeReg(REG_RX_GAIN_I,    0x80);  // LNA + mixer gain 24 dB
  writeReg(REG_RX_GAIN_IV,   0x0A);

  strobe(STROBE_STANDBY);

  if (!calibrateIF())       return false;
  if (!calibrateVCO(0x00))  return false;
  if (!calibrateVCO(0xA0))  return false;

  strobe(STROBE_STANDBY);

  // LNA on, PA off. Nothing is transmitted by this sketch.
  digitalWrite(PIN_TXEN, LOW);
  digitalWrite(PIN_RXEN, HIGH);

  Serial.println(F("  radio ready, receive mode"));
  Serial.println();
  return true;
}

// ------------------------------------------------------------
// Scanning
// ------------------------------------------------------------

uint8_t gBaseline[256];
bool    gHaveBaseline = false;

// The monitor sends CRLF. handleLine() runs on the CR, leaving the LF
// behind. Anything that waits for a keypress must clear it first or it
// sees that leftover byte and quits immediately.
void drainSerial() {
  delay(30);
  while (Serial.available()) Serial.read();
}

uint8_t readChannel(uint8_t channel) {
  writeReg(REG_CHANNEL, channel);
  strobe(STROBE_PLL);
  strobe(STROBE_RX);
  delayMicroseconds(600);
  uint8_t v = readReg(REG_ADC);
  strobe(STROBE_STANDBY);
  return v;
}

// Takes the strongest reading seen over several passes, so a short
// burst is not missed by unlucky timing.
void sweep(uint8_t *out, int passes) {
  for (int i = 0; i < 256; i++) out[i] = 0;
  for (int p = 0; p < passes; p++) {
    for (int ch = 0; ch < 256; ch++) {
      uint8_t v = readChannel((uint8_t)ch);
      if (v > out[ch]) out[ch] = v;
    }
  }
}

void printTable(const uint8_t *v) {
  uint8_t lo = 255, hi = 0;
  for (int i = 0; i < 256; i++) {
    if (v[i] < lo) lo = v[i];
    if (v[i] > hi) hi = v[i];
  }
  for (int ch = 0; ch < 256; ch++) {
    if (ch % 16 == 0) Serial.printf("\n  %3d: ", ch);
    Serial.printf("%3d ", v[ch]);
  }
  Serial.println();
  Serial.printf("\n  min %d   max %d   spread %d\n", lo, hi, hi - lo);
  if (hi == lo) {
    Serial.println(F("  FLAT. Every channel reads the same value, so the"));
    Serial.println(F("  RSSI ADC is telling us nothing. Do not trust any"));
    Serial.println(F("  b/s result - use the packet sniffer instead."));
  }
  Serial.println();
}

void takeBaseline() {
  Serial.println(F("--- baseline: keep the remote UNTOUCHED ---"));
  sweep(gBaseline, 4);
  gHaveBaseline = true;
  Serial.println(F("  raw baseline values:"));
  printTable(gBaseline);
}

void scanAndCompare() {
  static uint8_t now[256];

  if (!gHaveBaseline) {
    Serial.println(F("  no baseline yet - press b first"));
    return;
  }

  Serial.println(F("--- scanning: HOLD a button on the remote now ---"));
  sweep(now, 6);

  Serial.println();
  Serial.println(F("  ch   MHz~   base  now   delta"));

  int hits = 0;
  for (int ch = 0; ch < 256; ch++) {
    int delta = (int)now[ch] - (int)gBaseline[ch];
    if (delta < 0) delta = -delta;
    if (delta >= 6) {
      Serial.printf("  %3d  %4d   %3d   %3d   %+d\n",
                    ch, 2400 + ch, gBaseline[ch], now[ch],
                    (int)now[ch] - (int)gBaseline[ch]);
      hits++;
    }
  }

  Serial.println();
  if (hits == 0) {
    Serial.println(F("  nothing moved."));
    Serial.println(F("    Hold the button DOWN through the whole scan."));
    Serial.println(F("    Hold the remote within about 20 cm of the module."));
    Serial.println(F("    If still nothing, the remote may hop channels -"));
    Serial.println(F("    then use m to park on one channel and watch."));
  } else {
    Serial.printf("  %d channel(s) moved. Repeat to see which are consistent.\n", hits);
    Serial.println(F("  A single narrow spike that only appears while the"));
    Serial.println(F("  button is held is the remote. A broad raised region"));
    Serial.println(F("  that is always there is WiFi."));
  }
  Serial.println();
}

// ------------------------------------------------------------
// Packet sniffer - the method taken from the working implementation
// ------------------------------------------------------------

uint8_t gPacket[16];

bool sniffOnce() {
  strobe(STROBE_RX);
  delayMicroseconds(3000);

  // bit 0 of the mode register clears when the FIFO has a packet
  if (readReg(REG_MODE) & 0x01) return false;

  strobe(0xF0);                       // reset FIFO read pointer
  for (int i = 0; i < 16; i++) {
    gPacket[i] = readReg(0x05);
  }
  return true;
}

void printPacket() {
  Serial.print(F("  PACKET  "));
  for (int i = 0; i < 16; i++) Serial.printf("%02X ", gPacket[i]);
  Serial.println();
}

void sniffChannel(int ch, unsigned long ms) {
  writeReg(REG_CHANNEL, (uint8_t)ch);
  strobe(STROBE_PLL);
  int count = 0;
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (sniffOnce()) {
      count++;
      printPacket();
      if (count >= 8) break;
    }
    if (Serial.available()) break;
  }
  if (count) {
    Serial.printf("  channel %3d : %d packet(s)\n", ch, count);
  }
}

void sniffOne(int ch) {
  drainSerial();
  Serial.printf("--- sniffing channel %d, hold a button. any key stops ---\n", ch);
  writeReg(REG_CHANNEL, (uint8_t)ch);
  strobe(STROBE_PLL);
  while (!Serial.available()) {
    if (sniffOnce()) printPacket();
  }
  while (Serial.available()) Serial.read();
  Serial.println(F("  stopped."));
  Serial.println();
}

void sniffAll() {
  drainSerial();
  Serial.println(F("--- sniffing all 256 channels, HOLD a button ---"));
  Serial.println(F("    about 30 ms each, roughly 8 seconds total"));
  for (int ch = 0; ch < 256; ch++) {
    sniffChannel(ch, 30);
    if (Serial.available()) { while (Serial.available()) Serial.read(); break; }
  }
  Serial.println(F("--- sweep done ---"));
  Serial.println(F("  No packets anywhere most likely means the ID filter"));
  Serial.println(F("  is rejecting them, not that the band is empty."));
  Serial.println();
}

void monitorChannel(int ch) {
  drainSerial();
  Serial.printf("--- watching channel %d (~%d MHz). any key stops ---\n",
                ch, 2400 + ch);
  while (!Serial.available()) {
    uint8_t v = readChannel((uint8_t)ch);
    Serial.printf("  %3d  ", v);
    int bars = v / 4;
    for (int i = 0; i < bars && i < 40; i++) Serial.print('#');
    Serial.println();
    delay(120);
  }
  while (Serial.available()) Serial.read();
  Serial.println(F("  stopped."));
  Serial.println();
}

// ------------------------------------------------------------

void printMenu() {
  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" REMOTE 24 - TEST 004 - A7105 scanner + sniffer"));
  Serial.println(F("=================================================="));
  Serial.println(F("   b      take a quiet baseline (remote untouched)"));
  Serial.println(F("   s      scan while HOLDING a remote button"));
  Serial.println(F("   m<n>   park on channel n and watch, e.g. m80"));
  Serial.println(F("   r      re-print the stored baseline table"));
  Serial.println(F("   pa     packet sniff across all 256 channels"));
  Serial.println(F("   p<n>   packet sniff one channel, e.g. p80"));
  Serial.println(F("   i      re-run the radio init"));
  Serial.println(F("   ?      this menu"));
  Serial.println();
  Serial.println(F(" Order: b, then hold a button and press s."));
  Serial.println(F("=================================================="));
}

void handleLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  char c = line.charAt(0);

  switch (c) {
    case 'b': takeBaseline();   break;
    case 's': scanAndCompare(); break;
    case 'i': radioInit();      break;
    case 'r':
      if (!gHaveBaseline) Serial.println(F("  no baseline yet - press b"));
      else                printTable(gBaseline);
      break;
    case 'p':
      if (line.equalsIgnoreCase("pa")) sniffAll();
      else {
        int ch = line.substring(1).toInt();
        if (ch < 0 || ch > 255) Serial.println(F("  channel must be 0..255"));
        else sniffOne(ch);
      }
      break;
    case '?': printMenu();      break;
    case 'm': {
      int ch = line.substring(1).toInt();
      if (ch < 0 || ch > 255) {
        Serial.println(F("  channel must be 0..255"));
        break;
      }
      monitorChannel(ch);
      break;
    }
    default:
      Serial.println(F("  unknown command. type ? for the menu."));
      break;
  }
}

// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(400);

  Serial.println();
  Serial.println(F("=== REMOTE 24 - TEST 004 - A7105 scanner + sniffer ==="));

  pinMode(PIN_SCK,  OUTPUT);
  pinMode(PIN_SCS,  OUTPUT);
  pinMode(PIN_SDIO, OUTPUT);
  pinMode(PIN_TXEN, OUTPUT);
  pinMode(PIN_RXEN, OUTPUT);

  digitalWrite(PIN_SCK,  LOW);
  digitalWrite(PIN_SCS,  HIGH);
  digitalWrite(PIN_TXEN, LOW);
  digitalWrite(PIN_RXEN, LOW);

  delay(50);

  if (!radioInit()) {
    Serial.println(F("  init failed. the scanner will not work."));
    Serial.println(F("  a calibration failure usually means the crystal"));
    Serial.println(F("  is not running or VCC is out of range."));
  }

  printMenu();
}

void loop() {
  static String line = "";
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length() > 0) { handleLine(line); line = ""; }
    } else if (line.length() < 32) {
      line += c;
    }
  }
}

// ============================================================
// END OF FILE - REMOTE 24 - TEST 004 (A7105 scanner + packet sniffer)
// ============================================================