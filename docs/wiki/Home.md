# BG Bed Project Wiki

Smart wired bed / bedroom system — design, development, testing, lessons learned and build guide.

## Purpose
This wiki is written during development. It is both the project's technical source of truth and the foundation for a future public build guide / book.

## Current snapshot — 7 Sep 2026
- Two 7-inch Waveshare ESP32-S3 touch panels are in use.
- Shemi panel has the DS3231 RTC and sends time over RS-485.
- Ira panel receives the time and returns ACK.
- Shemi BedBox uses QuinLED ESP32 (WROOM-32E).
- Shemi BedBox RS-485 reception/parser has been proven on real hardware.
- 7 diagnostic NeoPixels are installed.
- TEST022 proved the pixels are RGBW (32-bit), not RGB (24-bit).
- TEST023 restores the full BedBox system with RGBW LED diagnostics.
- Massage architecture is being expanded from 6 toward 8 independently PWM-controlled motors using four L298N controllers.

## Contents
- [System Architecture](System-Architecture.md)
- [Hardware](Hardware.md)
- [Pin Assignments](Pin-Assignments.md)
- [RS-485 Protocol](RS485-Protocol.md)
- [Massage System](Massage-System.md)
- [Test Log](Test-Log.md)
- [Lessons Learned](Lessons-Learned.md)
- [Build Guide / Book Plan](Build-Guide.md)

## Status language
**PROVEN** = observed on real hardware.  
**CURRENT** = current configuration.  
**PLANNED** = proposed/future.  
**HISTORICAL** = older configuration retained for learning.

Existing `Handout.md` and `WIRING.md` remain source/history documents. Older hardware details must not override newer hardware-proven facts.
