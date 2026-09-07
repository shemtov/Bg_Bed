# System Architecture

## Core idea
BG Bed is a wired smart bedroom/bed system. Touch panels, BedBoxes and other nodes communicate over RS-485. The design deliberately minimizes unnecessary RF near the bed.

## Current major nodes
- Shemi 7-inch Waveshare ESP32-S3 touch panel
- Ira 7-inch Waveshare ESP32-S3 touch panel
- Shemi BedBox — QuinLED ESP32 WROOM-32E
- Ira BedBox — separate node
- Audio node
- Listener / voice node

## Current RS-485 facts
Shemi is the time source. Ira receives TIME and returns ACK. Shemi BedBox has been connected to the shared bus and has successfully parsed both frame types on hardware.
