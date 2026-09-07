# Massage System

## Existing proven history
Shemi has already used/slept with the six-motor arrangement.

The existing firmware maps six motor PWM outputs to:
`13, 14, 18, 19, 21, 22`

## Expansion direction
The architecture is now intended to support up to 8 independently PWM-controlled vibration motors.

Hardware concept:
- 4 x L298N controllers
- 2 motors per controller
- motor polarity/direction fixed by wiring
- one PWM control signal per motor

This means the target is 8 PWM GPIOs.

## Next engineering task
Choose two additional safe PWM GPIOs on the QuinLED board after reconciling current reservations and the now-used GPIO23 RGBW diagnostic output.
