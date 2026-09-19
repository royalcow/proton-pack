# Bill of Materials

This file tracks hardware actually owned/installed separately from parts that are only candidates.

## Current / On Hand

| Subsystem | Item | Status | Notes |
|---|---|---|---|
| Controller | Arduino Nano | CURRENT | Used for current prototypes/control |
| Controller dev | Adafruit ESP32-S3 Reverse TFT Feather #5691 | ORDERED | Development/prototyping controller; ESP32-S3, 4 MB flash, 2 MB PSRAM, integrated 240x135 TFT, STEMMA QT |
| Logic | SN74AHCT125N quad buffer/level shifter | ORDERED | 3.3 V ESP32 data to 5 V NeoPixel data; DIP-14; two channels planned |
| Logic | 14-pin DIP sockets | ORDERED | Socket the SN74AHCT125N on controller protoboard |
| Logic | 0.1 uF ceramic capacitors (104) | ORDERED | Local decoupling; one at AHCT125 VCC/GND |
| Controller carrier | Protoboard | CURRENT | Existing board available for controller prototype |
| Lighting | NeoPixel Jewel, 7-pixel | CURRENT | Cyclotron uses four chained Jewels (28 pixels); power-cell is a separate NeoPixel circuit |
| Power | 12 V LiPo battery | CURRENT | Existing pack battery |
| Power | Buck converter | CURRENT | Existing; 5 V rail |
| Audio | CH358D amplifier | CURRENT | Existing audio hardware |
| Audio | Pack speaker | CURRENT | Existing speaker |
| Wand | HasLab Spengler Wand 1984 | CURRENT | Active wand |
| Service panel | OLED 12864 | CURRENT / PROTOTYPE | I2C; selected address currently 0x78 (8-bit notation) |

## Candidates / Planned

| Subsystem | Item | Status | Notes |
|---|---|---|---|
| Controller | Adafruit ESP32-S3 Feather #5477 | PREFERRED PRODUCTION / WAITING | 4 MB flash + 2 MB PSRAM + STEMMA QT; currently unavailable from Adafruit; #5691 used for development |
| Lighting | ~330 ohm series resistors | CHECK ON HAND | One per NeoPixel data output; roughly 300-500 ohm acceptable |
| Lighting power | 470-1000 uF electrolytic capacitor, >=10 V | PLANNED | Bulk capacitance across 5 V/GND near NeoPixel power distribution |
| Controller | Feather socket/female headers | PLANNED | Keep development/production Feather removable |
| Bus | STEMMA QT/Qwiic cables | PLANNED AS NEEDED | Internal I2C peripherals |
| Audio | Tsunami-class audio board | EVALUATING | Polyphonic/cross-fade capability desired |

## Controller sourcing note

Adafruit shipping is roughly $10, so consolidate Adafruit-specific purchases where practical. Commodity parts (resistors, capacitors, sockets, common ICs/connectors) may be sourced from reputable US suppliers/Amazon when economical.

Add exact manufacturer part numbers, quantities, purchase links, and electrical specs as components are finalized.
