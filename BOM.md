# Bill of Materials

This file tracks hardware actually owned/installed separately from parts that are only candidates.

## Current / On Hand

| Subsystem | Item | Status | Notes |
|---|---|---|---|
| Controller | Arduino Nano | CURRENT | Used for current prototypes/control |
| Lighting | NeoPixel Jewel, 7-pixel | CURRENT | Two circuits recorded as installed; pack-controller firmware currently configures four, so verify physical count |
| Power | 12 V LiPo battery | CURRENT | Existing pack battery |
| Power | Buck converter | CURRENT | Existing |
| Audio | CH358D amplifier | CURRENT | Existing audio hardware |
| Audio | Pack speaker | CURRENT | Existing speaker |
| Wand | HasLab Spengler Wand 1984 | CURRENT | Active wand |
| Service panel | OLED 12864 | CURRENT / PROTOTYPE | I2C; selected address currently 0x78 |

## Candidates / Planned

| Subsystem | Item | Status | Notes |
|---|---|---|---|
| Controller | ESP32 board | PLANNED | Adafruit STEMMA QT/Qwiic option favored |
| Logic | 3.3 V to 5 V level shifting | PLANNED | For devices requiring 5 V logic |
| Audio | Tsunami-class audio board | EVALUATING | Polyphonic/cross-fade capability desired |

Add exact manufacturer part numbers, quantities, purchase links, and electrical specs as components are finalized.

## Standalone attenuator display test

Specified POC hardware: yellow BL28Z-3005SA04Y 28-segment display and Adafruit HT16K33 breakout #1427, driven by the 5 V Nano. Basic display operation has been confirmed by the user; wiring documentation and remaining checks are pending; see `firmware/POC/Attenuator_Bargraph/README.md`.
