// ============================================================
// RF LEARN - TEST 005  (transmit, ESP32-S3 DevKitC-1)
// ============================================================
//
// Westinghouse SQ19D-F20 ceiling fan - replay of the 14 captured codes.
// Board: ESP32-S3-DevKitC-1 N16R8.
//
// WHAT CHANGED vs TEST 004
//   Nothing in this file except the test number. platformio.ini adds
//   monitor_rts = 0 and monitor_dtr = 0, so opening the serial monitor
//   stops dropping the board into ROM download mode.
//
// WHAT CHANGED IN TEST 004 vs TEST 003
//   Nothing in this file except the test number. The real change is in
//   platformio.ini: ARDUINO_USB_CDC_ON_BOOT goes 1 -> 0, so Serial comes
//   out of UART0 (the CP2102 port, COM6) instead of the native USB port.
//   TEST 003 booted correctly - the banner was just going to the other
//   connector.
//
// WHAT CHANGED IN TEST 003 vs TEST 002
//   TEST 002 targeted the classic ESP32 and put the transmitter on
//   GPIO 26. On the S3 that pin does not exist as a free GPIO - 26..32
//   are wired to the SPI flash, and on an N16R8 part 33..37 are eaten
//   by the octal PSRAM. Moved to GPIO 10, which is free and is also
//   outside the proposed bedbox_ira pin map (4,5,6,7,15,21,38,39 for
//   motors, 16,17,18 for the SP3485), so nothing collides later.
//
// WIRING
//   FS1000A  VCC  -> 5V
//   FS1000A  GND  -> GND        (must be common with the ESP32)
//   FS1000A  DATA -> GPIO 10
//   FS1000A  ANT  -> 17.3 cm straight solid copper wire
//
// NOTHING HERE IS CONFIRMED ON HARDWARE.
// Pulse length and bit length are both still open questions - that is
// exactly why they are runtime settings and not #defines. Sweep them.
//
// ============================================================

#include <Arduino.h>
#include <RCSwitch.h>

#define TEST_NUMBER 5

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------

const int TX_PIN = 10;

// Starting point, all three provisional:
int  gProtocol    = 2;     // what RCSwitch reported on receive
int  gBitLength   = 29;    // what RCSwitch reported - possibly wrong
int  gPulseLength = 0;     // 0 = use the protocol's own default (650 us)
int  gRepeats     = 10;    // frames per press

RCSwitch tx = RCSwitch();

// ------------------------------------------------------------
// The 14 captured codes
// Captured 24 Aug 2026. Protocol 2, 29 bits, pulse ~605 or ~678 us.
// The first 20 bits are identical in all of them - that is the
// remote's address:  0011 1101 0111 0101 0101
// ------------------------------------------------------------

struct Button {
  const char *name;
  unsigned long code;
};

const Button BUTTON[] = {
  { "fan on/off",  128887697UL },
  { "speed 1",     128887784UL },
  { "speed 2",     128887752UL },
  { "speed 3",     128887721UL },
  { "speed 4",     128887689UL },
  { "speed 5",     128887658UL },
  { "speed 6",     128887626UL },
  { "lamp on/off", 128887477UL },
  { "timer 1h",    128887445UL },
  { "timer 2h",    128887351UL },
  { "timer 4h",    128887414UL },
  { "timer 8h",    128887634UL },
  { "reverse",     128887595UL },
  { "waves",       128887563UL },
};

const int BUTTON_COUNT = sizeof(BUTTON) / sizeof(BUTTON[0]);

// ------------------------------------------------------------
// Apply the current settings to the transmitter
//
// Order matters. setProtocol() resets the pulse length to that
// protocol's default, so any custom pulse length must be applied
// AFTER it, never before.
// ------------------------------------------------------------

void applySettings() {
  tx.setProtocol(gProtocol);
  if (gPulseLength > 0) {
    tx.setPulseLength(gPulseLength);
  }
  tx.setRepeatTransmit(gRepeats);
}

void printSettings() {
  Serial.println();
  Serial.println(F("--- current settings ---"));
  Serial.printf("  protocol     : %d\n", gProtocol);
  Serial.printf("  bit length   : %d\n", gBitLength);
  if (gPulseLength > 0) {
    Serial.printf("  pulse length : %d us\n", gPulseLength);
  } else {
    Serial.println(F("  pulse length : protocol default (650 us)"));
  }
  Serial.printf("  repeats      : %d\n", gRepeats);
  Serial.println();
}

void printMenu() {
  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" RF LEARN - TEST 005 - transmit"));
  Serial.println(F("=================================================="));
  Serial.println(F(" Buttons - type the number and press Enter:"));
  for (int i = 0; i < BUTTON_COUNT; i++) {
    Serial.printf("   %2d  %-12s  %lu\n", i + 1, BUTTON[i].name, BUTTON[i].code);
  }
  Serial.println();
  Serial.println(F(" Settings:"));
  Serial.println(F("   p605 p650 p678   set pulse length in us"));
  Serial.println(F("   p0               back to protocol default"));
  Serial.println(F("   b29  b24  b19    set bit length"));
  Serial.println(F("   t1   t2   t3     set protocol"));
  Serial.println(F("   r10  r20         set repeat count"));
  Serial.println(F("   s                show settings"));
  Serial.println(F("   ?                show this menu"));
  Serial.println();
  Serial.println(F(" Sweep order if nothing happens:"));
  Serial.println(F("   p650 -> p605 -> p678, trying button 1 after each."));
  Serial.println(F("   Then b24, and repeat the three pulse lengths."));
  Serial.println(F("=================================================="));
  printSettings();
}

// ------------------------------------------------------------

void sendButton(int index) {
  if (index < 1 || index > BUTTON_COUNT) {
    Serial.printf("  no button %d\n", index);
    return;
  }
  const Button &b = BUTTON[index - 1];

  applySettings();

  Serial.printf("  sending %-12s  code %lu  %d bits  protocol %d  pulse %s\n",
                b.name, b.code, gBitLength, gProtocol,
                gPulseLength > 0 ? String(gPulseLength).c_str() : "default");

  tx.send(b.code, gBitLength);

  Serial.println(F("  sent."));
}

// ------------------------------------------------------------

void handleLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  char c = line.charAt(0);

  // a bare number = send that button
  if (c >= '0' && c <= '9') {
    sendButton(line.toInt());
    return;
  }

  int value = line.substring(1).toInt();

  switch (c) {
    case 'p':
      gPulseLength = value;
      Serial.printf("  pulse length -> %d\n", gPulseLength);
      printSettings();
      break;

    case 'b':
      if (value < 1 || value > 64) {
        Serial.println(F("  bit length must be 1..64"));
        break;
      }
      gBitLength = value;
      Serial.printf("  bit length -> %d\n", gBitLength);
      printSettings();
      break;

    case 't':
      if (value < 1 || value > 12) {
        Serial.println(F("  protocol must be 1..12"));
        break;
      }
      gProtocol = value;
      // changing protocol drops any custom pulse length
      gPulseLength = 0;
      Serial.printf("  protocol -> %d  (pulse length reset to default)\n", gProtocol);
      printSettings();
      break;

    case 'r':
      if (value < 1 || value > 100) {
        Serial.println(F("  repeats must be 1..100"));
        break;
      }
      gRepeats = value;
      Serial.printf("  repeats -> %d\n", gRepeats);
      printSettings();
      break;

    case 's':
      printSettings();
      break;

    case '?':
      printMenu();
      break;

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
  Serial.println(F("=== RF LEARN - TEST 005 - transmit ==="));

  tx.enableTransmit(TX_PIN);
  applySettings();

  Serial.printf("transmitter on GPIO %d\n", TX_PIN);
  printMenu();
}

void loop() {
  static String line = "";

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length() > 0) {
        handleLine(line);
        line = "";
      }
    } else if (line.length() < 32) {
      line += c;
    }
  }
}

// ============================================================
// END OF FILE - RF LEARN - TEST 005 (transmit, ESP32-S3 DevKitC-1)
// ============================================================
