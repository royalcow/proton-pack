# SSD1306 Service Panel Proof of Concept

This standalone Arduino Nano sketch demonstrates a 128 x 64 I2C SSD1306
service-panel display. It is an isolated proof of concept and is not integrated
with the proton-pack or wand firmware.

The boot sequence is followed by a static screen showing pack and wand status,
11.8V battery voltage, and 70% audio level.

## Wiring

| OLED | Arduino Nano |
|---|---|
| GND | GND |
| VCC | 5V or 3.3V, as required by the breakout |
| SDA | A4 |
| SCL | A5 |

The display address is `0x3C`. Common yellow/blue modules remain monochrome in
software; their colors are fixed physical regions. This layout keeps the title
in rows 0-15 and the boot emblem in rows 16-43.

## Dependencies

- Adafruit GFX Library
- Adafruit SSD1306

Adafruit BusIO may be installed automatically as a dependency.

## Build and upload

Open `POC.ino`, select **Arduino Nano** with the
**ATmega328P** processor, choose the serial port, and upload. For older Nano
clones, select **ATmega328P (Old Bootloader)** if the normal upload fails.

```sh
arduino-cli compile --fqbn arduino:avr:nano firmware/service-panel/POC
arduino-cli upload --fqbn arduino:avr:nano --port /dev/cu.YOUR_PORT firmware/service-panel/POC
```

## Memory

Adafruit_SSD1306 allocates a 1,024-byte framebuffer at runtime. The 140-byte
emblem is stored in flash with `PROGMEM`; constant strings use `F()`. The
sketch avoids dynamic `String` objects and additional framebuffers.
