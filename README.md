# MIDI Foot Pedal Firmware

Open source ESP32-S3 firmware for a configurable 
MIDI foot controller with RGB LED feedback and 
web-based configuration interface.

## Hardware
- ESP32-S3 (XIAO form factor or equivalent)
- 4 momentary footswitches
- 1 RGB LED (WS2812B)
- Beautiful handcrafted enclosure

## Features
- USB MIDI — native S3 implementation, no adapter needed
- Web configurator — connect to onboard WiFi hotspot, 
  open browser, assign functions to buttons without touching code
- RGB LED feedback — RMT peripheral driven, 
  no conflicts with USB MIDI
- Configurable per-button: MIDI CC, program change, 
  note on/off, looper commands

## Status
Active development. RMT/USB MIDI conflict resolution 
in progress.

## Why Open Source
The firmware is free. The philosophy is that beautiful 
hardware should be accessible to anyone who wants to 
build it. If you want the hardware itself, 
visit [ragamuffinworkshop.store](https://ragamuffinworkshop.store)

## License
MIT
