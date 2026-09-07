# Hardware

## Shemi BedBox
Board: QuinLED ESP32, ESP32-WROOM-32E.

### Diagnostic LEDs
Exactly 7 addressable LEDs are installed.

**PROVEN discovery:** the pixels are RGBW, requiring 32 bits per pixel. Earlier firmware used RGB/24-bit and produced the characteristic failure where only about five of seven physical LEDs responded and colors appeared random.

Current data path tested in the standalone LED test:
`GPIO23 -> external level shifter -> DIN`

Level shifter wiring used:
- LV = 3.3V
- HV = 5V
- common GND
- GPIO23 -> LV1
- HV1 -> first pixel DIN

## Massage controllers
L298N controllers, two motors per controller. Four controllers allow an 8-motor architecture.
