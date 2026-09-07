# Test Log

## TEST015
Serial ALIVE diagnostics proved the BedBox firmware was running. RS-485 RX initially showed zero until TTL TX/RX wiring was crossed correctly.

## TEST016
**PROVEN:** Shemi BedBox received and parsed live 14-byte A5 TIME and ACK frames. Matching sequence numbers were observed. CRC/XOR errors were zero in the shown run.

## TEST017–TEST021
NeoPixel debugging. Animation timing/logic was visible, but physical colors remained wrong and only about five LEDs responded.

## TEST022
**PROVEN:** changing the pixel format from `NEO_GRB` (24-bit RGB) to `NEO_GRBW` (32-bit RGBW) fixed the LEDs. All seven responded correctly.

This is a key diagnostic lesson: 7 logical RGB pixels send 168 bits; an RGBW chain consumes 32 bits per physical pixel, so 168/32 = 5.25 physical pixels — matching the observed symptom remarkably closely.

## TEST023
Full BedBox firmware restored with:
- 7 RGBW LEDs
- GPIO23 via external level shifter
- RS-485 parser
- motor logic
- startup red x2
- green diagnostic train
- RX yellow
- TX blue

**Observed:** TIME and ACK traffic on the shared bus with CRCbad=0. When Ira panel was powered off ACK stayed zero; when it was powered on ACK traffic returned.
