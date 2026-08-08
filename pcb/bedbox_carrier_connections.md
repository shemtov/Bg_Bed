# Bg_Bed - bed box carrier board, rev A

## Connection table

| Part | Pin       | Net                        | What it is                                          |
|------|-----------|----------------------------|-----------------------------------------------------|
| J1   | 1         | +12V_IN                    | 12 V in from the supply, before the fuse            |
| J1   | 2         | GND                        | THE star point. Every ground in the box lands here. |
| F1   | 1 / 2     | +12V_IN / +12V             | 6 A slow blade fuse                                 |
| C1   | 1 / 2     | +12V / GND                 | 1000uF 25V, close to J4                             |
| J4   | 1 / 2     | +12V / GND                 | 12 V out to the three L298N modules                 |
| U1   | 1 / 2     | +12V / GND                 | buck module, input                                  |
| U1   | 3 / 4     | +5V / GND                  | buck module, output                                 |
| U2   | 1 / 2     | +5V / GND                  | ESP32-S3 left socket, power                         |
| U2   | 5,6,7,8   | PWM1..PWM4                 | GPIO4, 5, 6, 7                                      |
| U2   | 10 / 11   | RS485_TX / RS485_RX        | GPIO8 / GPIO9                                       |
| U3   | 1 / 2     | GND / SPARE_3V3            | right socket, ground and 3V3                        |
| U3   | 5,6,7,8   | PWM5..PWM8                 | GPIO15, 16, 17, 18                                  |
| U3   | 12        | LED_DATA_3V3               | GPIO21, to the level shifter                        |
| U3   | 15..20    | SPARE1..SPARE6             | GPIO38-42, 47                                       |
| U5   | 1 / 2     | +5V / GND                  | RS-485 auto module, power                           |
| U5   | 3 / 4     | RS485_TX / RS485_RX        | module signal side                                  |
| J2   | 1 / 2 / 3 | RS485_A / RS485_B / GND    | screw terminal to the house bus                     |
| J3   | 1..8      | PWM1..PWM8                 | IDC to the L298N ENA / ENB inputs                   |
| J3   | 9 / 10    | GND / GND                  | two grounds in the same flat cable                  |
| U4   | 1 / 7     | U4_OE / GND                | 74AHCT125 output enable tied low                    |
| U4   | 2 / 3     | LED_DATA_3V3 / LED_DATA_5V | one gate: 3V3 in, 5 V out                           |
| U4   | 14        | +5V                        | supply                                              |
| J11  | 1 / 2 / 3 | +12V / LED_DATA_5V / GND   | WS2815 strip                                        |
| J12  | 1..6      | SPARE1..SPARE6             | spare GPIO header                                   |
| J12  | 7 / 8     | SPARE_3V3 / GND            | so a sensor can be powered from it                  |

## Notes

- The three L298N modules are NOT on this board. They take 12 V and GND
  straight from J4 and receive only PWM through J3.
- Their ENA / ENB factory jumpers still have to come off by hand.
- J1 pin 2 is the single star point. Do not create a second ground path.
- Eight channels so one board serves both beds: Shemi wires 8, Ira wires 6.
- GPIO numbers above are the PLAN. Check each socket hole against the board
  in your hand before the layout is finished - stock symbols get this wrong.
