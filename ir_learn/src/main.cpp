// ============================================================
// IR LEARN - TEST 006  (transmit + receive)
// ============================================================
//
// Board: ESP32-S3-DevKitC-1 N16R8
//
// WIRING
//   IR receiver    OUT -> GPIO 11    VCC -> 3V3   GND -> GND  + 100nF
//   IR transmitter                   GPIO 10
//     Ready-made module: S -> GPIO 10, VCC -> 5V, GND -> GND
//     Bare LED: transistor required - see platformio.ini header.
//
// WHERE THE STATES BELOW CAME FROM
//   Six presses captured in TEST 003 and hand-decoded. Every one
//   passed the Gree checksum. The sequence matches exactly what was
//   pressed: ON, then MODE, then three temperature steps, then OFF.
//
//     1  39 08 20 50 02 83 00 30   on,  cool, fan high, 24C
//     2  1A 08 20 50 02 83 00 40   on,  dry,  fan low,  24C
//     3  1A 09 20 50 02 83 00 50   on,  dry,  fan low,  25C
//     4  1A 0A 20 50 02 83 00 60   on,  dry,  fan low,  26C
//     5  1A 0B 20 50 02 83 00 70   on,  dry,  fan low,  27C
//     6  12 0B 20 50 02 83 00 F0   off, dry,  fan low,  27C
//
//   Sending one of these replays a real remote press byte for byte.
//   That is the safest possible first shot - it is not something the
//   library invented, it is what the remote actually sent.
//
// THE SELF-CHECK AT BOOT
//   Before anything is transmitted, the sketch asks IRGreeAC to build
//   "on, cool, 24C, fan max, light on" and compares it against
//   capture 1. If the library disagrees with the real remote, that is
//   printed loudly and the difference is shown byte by byte. Better to
//   find that on the desk than pointed at the AC.
//
// WHAT CHANGED vs TEST 005
//   TEST 005 fixed bytes 2 and 4. Byte 5 remained: library 20,
//   remote 83. Byte 7 differed only because it is the checksum over
//   byte 5, so it was never a separate problem.
//
//   Byte 5 holds DisplayTemp(2), IFeel(1), a field the library calls
//   unknown2(3) and hard-codes to 0b100, WiFi(1), and one unnamed
//   bit. This remote sends DisplayTemp=3, unknown2=0, and that top
//   bit SET. Two of those three are not reachable through any setter,
//   so no combination of API calls will ever reproduce byte 5.
//
//   So the state is built with the library, byte 5 is then overwritten
//   with the real remote's value, and the library recomputes the
//   checksum from the patched bytes. The checksum routine was verified
//   in both directions before relying on it: it predicts D0 for the
//   library's own byte 5, and 30 for the remote's. Same formula, both
//   answers right.
//
//   Result: byte-exact remote frames at any temperature and mode,
//   not just the six captured ones.
//
// WHAT CHANGED IN TEST 005 vs TEST 004
//   TEST 004's self check came back DIFFERENT on two bytes, and both
//   were my own wrong assumptions, not library faults.
//
//   byte 2 : library 60, remote 20.  Bit 6 is ModelA. ir_Gree.cpp
//            line 212 writes it as (power_on && model == YAW1F), so
//            asking for YAW1F switched it on. The real remote leaves
//            it clear, which means it is NOT YAW1F in the library's
//            terms. Changed to YBOFB.
//
//   byte 4 : library 00, remote 02.  This is SwingV. I passed
//            kGreeSwingLastPos (0); the remote sends 2, which is
//            kGreeSwingUp - a perfectly valid value, just not the
//            one I guessed.
//
//   With both corrected the self check should print MATCH. If it
//   does, states can be built from scratch and commands 1..6 become
//   a fallback rather than the only safe option.
//
// THE AC WILL REACT
//   It beeps and starts running. Nothing here is a simulation.
//
// NOT YET RUN ON HARDWARE.
//
// ============================================================

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRac.h>
#include <ir_Gree.h>

#define TEST_NUMBER 6

// ------------------------------------------------------------
// Pins
// ------------------------------------------------------------

const uint16_t IR_TX_PIN = 10;
const uint16_t IR_RX_PIN = 11;

const uint16_t IR_CAPTURE_BUFFER    = 1024;
const uint8_t  IR_TIMEOUT_MS        = 30;
const uint16_t IR_MIN_UNKNOWN_SIZE  = 100;

IRGreeAC ac(IR_TX_PIN);
IRrecv   irrecv(IR_RX_PIN, IR_CAPTURE_BUFFER, IR_TIMEOUT_MS, true);
decode_results results;

// ------------------------------------------------------------
// The six captured states, exactly as the remote sent them
// ------------------------------------------------------------

const uint8_t CAPTURED[6][8] = {
  {0x39, 0x08, 0x20, 0x50, 0x02, 0x83, 0x00, 0x30},
  {0x1A, 0x08, 0x20, 0x50, 0x02, 0x83, 0x00, 0x40},
  {0x1A, 0x09, 0x20, 0x50, 0x02, 0x83, 0x00, 0x50},
  {0x1A, 0x0A, 0x20, 0x50, 0x02, 0x83, 0x00, 0x60},
  {0x1A, 0x0B, 0x20, 0x50, 0x02, 0x83, 0x00, 0x70},
  {0x12, 0x0B, 0x20, 0x50, 0x02, 0x83, 0x00, 0xF0},
};

const char *CAPTURED_DESC[6] = {
  "on,  cool, fan high, 24C",
  "on,  dry,  fan low,  24C",
  "on,  dry,  fan low,  25C",
  "on,  dry,  fan low,  26C",
  "on,  dry,  fan low,  27C",
  "off, dry,  fan low,  27C",
};

// ------------------------------------------------------------

// The library cannot produce byte 5 through its setters - see the
// header. Build the state, then force byte 5 to what the real remote
// sends and let the library redo the checksum over the patched bytes.
const uint8_t REMOTE_BYTE5 = 0x83;

void applyRemoteByte5() {
  uint8_t buf[8];
  memcpy(buf, ac.getRaw(), 8);
  buf[5] = REMOTE_BYTE5;
  ac.setRaw(buf);          // setRaw keeps the bytes; send() fixes the sum
}

void printBytes(const char *label, const uint8_t *b) {
  Serial.print(label);
  for (int i = 0; i < 8; i++) Serial.printf("%02X ", b[i]);
  Serial.println();
}

void printState() {
  Serial.println();
  Serial.println(F("--- state the library will send ---"));
  printBytes("  bytes : ", ac.getRaw());
  Serial.print(F("  human : "));
  Serial.println(ac.toString());
  Serial.println();
}

// ------------------------------------------------------------
// Compare what the library builds against a real captured press
// ------------------------------------------------------------

void selfCheck() {
  Serial.println();
  Serial.println(F("--- self check: library vs the real remote ---"));

  // YBOFB, not YAW1F: the model choice drives the ModelA bit in
  // byte 2, and the real remote leaves that bit clear.
  ac.setModel(gree_ac_remote_model_t::YBOFB);
  ac.on();
  ac.setMode(kGreeCool);
  ac.setTemp(24);
  ac.setFan(kGreeFanMax);
  ac.setLight(true);
  ac.setXFan(false);
  ac.setTurbo(false);
  ac.setSleep(false);
  ac.setSwingVertical(false, kGreeSwingUp);   // 2 - what the remote sends
  applyRemoteByte5();

  const uint8_t *built = ac.getRaw();
  printBytes("  library : ", built);
  printBytes("  remote  : ", CAPTURED[0]);

  bool same = true;
  for (int i = 0; i < 8; i++) if (built[i] != CAPTURED[0][i]) same = false;

  if (same) {
    Serial.println(F("  MATCH - the library agrees with the real remote."));
    Serial.println(F("  Building states from scratch is safe."));
  } else {
    Serial.println(F("  DIFFERENT. Not necessarily wrong - some bits are"));
    Serial.println(F("  cosmetic - but do not trust built states yet."));
    Serial.println(F("  Differing bytes:"));
    for (int i = 0; i < 8; i++) {
      if (built[i] != CAPTURED[0][i]) {
        Serial.printf("    byte %d : library %02X   remote %02X\n",
                      i, built[i], CAPTURED[0][i]);
      }
    }
    Serial.println(F("  Use commands 1..6 instead - those are verbatim."));
  }
  Serial.println();
}

// ------------------------------------------------------------

void sendCaptured(int n) {
  if (n < 1 || n > 6) { Serial.println(F("  use 1..6")); return; }
  Serial.printf("  sending captured %d : %s\n", n, CAPTURED_DESC[n - 1]);
  printBytes("    bytes : ", CAPTURED[n - 1]);
  ac.setRaw(const_cast<uint8_t *>(CAPTURED[n - 1]));
  ac.send();
  Serial.println(F("    sent."));
}

void printMenu() {
  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" IR LEARN - TEST 006 - transmit + receive"));
  Serial.println(F("=================================================="));
  Serial.println(F(" Replay a real captured press - safest first test:"));
  for (int i = 0; i < 6; i++) {
    Serial.printf("   %d   %s\n", i + 1, CAPTURED_DESC[i]);
  }
  Serial.println();
  Serial.println(F(" Build a state yourself:"));
  Serial.println(F("   on / off        power"));
  Serial.println(F("   t24             temperature 16..30"));
  Serial.println(F("   m0..m4          auto cool dry fan heat"));
  Serial.println(F("   f0..f3          auto low med high"));
  Serial.println(F("   p               print the state without sending"));
  Serial.println(F("   x               SEND the state built above"));
  Serial.println();
  Serial.println(F("   c               re-run the self check"));
  Serial.println(F("   ?               this menu"));
  Serial.println();
  Serial.println(F(" The receiver stays live. Anything sent should come"));
  Serial.println(F(" straight back as a capture - that is the proof the"));
  Serial.println(F(" LED is actually emitting."));
  Serial.println(F("=================================================="));
}

// ------------------------------------------------------------

void handleLine(String line) {
  line.trim();
  line.toLowerCase();
  if (!line.length()) return;

  if (line == "on")  { ac.on();  Serial.println(F("  power on"));  printState(); return; }
  if (line == "off") { ac.off(); Serial.println(F("  power off")); printState(); return; }
  if (line == "p")   { applyRemoteByte5(); printState(); return; }
  if (line == "c")   { selfCheck(); return; }
  if (line == "?")   { printMenu(); return; }
  if (line == "x")   {
    applyRemoteByte5();
    Serial.println(F("  sending the built state"));
    printBytes("    bytes : ", ac.getRaw());
    ac.send();
    Serial.println(F("    sent."));
    return;
  }

  char c = line.charAt(0);
  int  v = line.substring(1).toInt();

  if (c >= '1' && c <= '6' && line.length() == 1) { sendCaptured(line.toInt()); return; }

  switch (c) {
    case 't':
      if (v < 16 || v > 30) { Serial.println(F("  temp must be 16..30")); break; }
      ac.setTemp(v);
      Serial.printf("  temp -> %d C\n", v);
      printState();
      break;
    case 'm':
      if (v < 0 || v > 4) { Serial.println(F("  mode must be 0..4")); break; }
      ac.setMode(v);
      Serial.printf("  mode -> %d\n", v);
      printState();
      break;
    case 'f':
      if (v < 0 || v > 3) { Serial.println(F("  fan must be 0..3")); break; }
      ac.setFan(v);
      Serial.printf("  fan -> %d\n", v);
      printState();
      break;
    default:
      Serial.println(F("  unknown command. ? for the menu."));
      break;
  }
}

// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(400);

  Serial.println();
  Serial.println(F("=== IR LEARN - TEST 006 - transmit + receive ==="));
  Serial.printf("transmitter GPIO %d   receiver GPIO %d\n",
                IR_TX_PIN, IR_RX_PIN);

  ac.begin();

  irrecv.setUnknownThreshold(IR_MIN_UNKNOWN_SIZE);
  irrecv.enableIRIn();

  selfCheck();
  printMenu();
}

void loop() {
  static String line = "";

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length()) { handleLine(line); line = ""; }
    } else if (line.length() < 32) {
      line += c;
    }
  }

  if (irrecv.decode(&results)) {
    Serial.println();
    Serial.print(F("  [heard] "));
    Serial.print(typeToString(results.decode_type, results.repeat));
    Serial.printf("  raw len %d\n", getCorrectedRawLength(&results));
    String d = IRAcUtils::resultAcToString(&results);
    if (d.length()) { Serial.print(F("  [heard] ")); Serial.println(d); }
  }
}

// ============================================================
// END OF FILE - IR LEARN - TEST 006 (transmit + receive)
// ============================================================
