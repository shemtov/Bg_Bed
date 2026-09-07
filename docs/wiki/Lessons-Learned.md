# Lessons Learned

## 1. Five of seven LEDs was a data-format clue
The LED fault survived wiring changes and a level shifter. The animation sequence was correct, but colors were wrong and only about five of seven pixels reacted.

Cause: the physical pixels were RGBW (32-bit) while firmware used RGB (24-bit).

The arithmetic itself was diagnostic:
`7 x 24 / 32 = 5.25`

TEST022 changed to `NEO_GRBW` and the hardware worked.

## 2. Separate software timing from electrical/data-format faults
A standalone LED-only test removed RS-485, motors and protocol processing. That showed the animation logic was not the root problem.

## 3. TTL UART crosses; RS-485 bus does not
At the TTL side:
TX -> RX and RX <- TX.

At the differential bus:
A -> A, B -> B, GND -> GND.

## 4. Do not declare a subsystem proven from a log alone
A software `Serial.write()` proves the program attempted transmission. It does not prove another physical node received it.

## 5. Keep failed tests
The failed tests explain why the final design looks the way it does. They belong in the public build story, not in the trash.
