# Pin Assignments

## Shemi BedBox — current known assignments

| Function | GPIO | Status |
|---|---:|---|
| RS-485 RX | 16 | proven reception |
| RS-485 TX | 17 | current |
| NeoPixel RGBW DATA | 23 | proven in TEST022 through external level shifter |
| Motor PWM 1 | 13 | existing 6-motor firmware |
| Motor PWM 2 | 14 | existing 6-motor firmware |
| Motor PWM 3 | 18 | existing 6-motor firmware |
| Motor PWM 4 | 19 | existing 6-motor firmware |
| Motor PWM 5 | 21 | existing 6-motor firmware |
| Motor PWM 6 | 22 | existing 6-motor firmware |

## 8-motor expansion
Two additional PWM-capable GPIO assignments still need to be selected and hardware-tested. Do not permanently wire them until the QuinLED pin use and all existing reservations are rechecked.
