# Proton Pack Project Tracker

This file is the high-level tracker for planned and active upgrades to the Proton Pack. Detailed design, firmware, wiring, sourcing, and build notes can live in their respective project areas.

_Last updated: 2026-09-18_

| # | Project / Upgrade | Status | Current direction |
|---|---|---|---|
| 1 | Attenuator | In progress | Base shell printed; parts on order. Functional volume control, Ghostbusters theme control, LEDs, and possible future I2C integration. |
| 2 | Controller / ESP32-S3 | Planning | Migrate from Arduino Nano to ESP32-S3. OTA firmware updates, Wi-Fi/BLE, more I/O, and an internal I2C/accessory bus. |
| 3 | Printed Wand | In progress | New wand is being printed. Continue using the HasLab Spengler wand in the near term while keeping the pack ready for migration to the printed wand. |
| 4 | Battery | Planning | Replace/update the current 12 V battery with higher capacity/current headroom, integrated protection/BMS, and pack-side voltage telemetry. |
| 5 | Audio subsystem | Exploring | Replace/refactor the current CH358D setup. Investigating polyphonic playback, crossfading, simultaneous effects, and independent music/effects control. |
| 6 | PKE Meter | In progress | Fast prototype underway. Motorized wings/display, manual proximity control, and potential BLE ghost-detection mode. |
| 7 | Service Panel | Planned — low priority | Modern OLED service display, Ghostbusters logo, battery/system telemetry, diagnostics, OTA status, and possible USB/service access. |
| 8 | N-filter Fog | Future | Integrate a self-contained fog/steam effect into the N-filter, synchronized with overheat/vent sequences and potentially a manual purge. |
| 9 | Pack V2 / Removable Cyclotron | Long-term | Potential major pack rebuild featuring a removable cyclotron and incorporating lessons from the current electronics architecture. |

## Architecture direction

The current long-term direction is toward an ESP32-S3 main controller with modular power distribution and an internal accessory bus. Candidate smart peripherals include the attenuator, service panel, battery telemetry, audio controls, sensors, and future wand/PKE integrations.

The current HasLab wand should remain supported while the printed wand is developed.

## Project tracking

Use this file for high-level status and priorities. Detailed implementation work should remain separated by subsystem so individual upgrades can evolve without turning this document into a build log.
