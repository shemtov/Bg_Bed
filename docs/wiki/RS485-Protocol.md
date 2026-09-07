# RS-485 Protocol

## Hardware-proven temporary A5 frame
Current panel time/ACK traffic observed by Shemi BedBox uses 14 bytes:

`A5 SRC DST TYPE SEQ_LO SEQ_HI P0 P1 P2 P3 P4 P5 P6 XOR`

Checksum is XOR of bytes 0 through 12.

### Types observed
- `0x10` — TIME
- `0x11` — ACK

### Example TIME
`A5 01 02 10 E1 05 EA 07 09 06 17 0F 13 BB`

### Example ACK
`A5 02 01 11 E1 05 00 00 00 00 00 00 00 53`

The matching sequence number connects the TIME and ACK pair.

## Important
Motor-command framing has not yet been mapped from live panel traffic. Do not invent it. Capture real frames first.
