# Project Changelog

Concise cross-session project changes. This is not a replacement for Git history.

## 2026-09-18

### Pack Controller Firmware
- Documented the current Nano firmware architecture, pin assignments, I/O, NeoPixel animations, wand handling, audio queue, timing, state machine, power sensing, globals, and hardware assumptions.
- Recorded the firmware's INA219 `0x40` current sensing, A0 voltage-divider stub, and Audio FX UART wiring.
- Confirmed the cyclotron physically contains four 7-pixel NeoPixel Jewels (28 pixels), matching the firmware configuration.

### Project Infrastructure
- Created canonical GitHub project repository documentation.
- Established repository files as the shared source of truth between ChatGPT and Codex.

### Service Panel
- OLED identified as 12864-class display.
- Added standalone Nano/SSD1306 proof-of-concept firmware and documentation under `firmware/POC/OLED/`.
- Recorded `0x3C` as the POC's 7-bit SSD1306 address and clarified that `0x78` is its 8-bit write-address notation; physical verification remains pending.
- Larger/color display remains a future option.
- Service-panel project remains low priority.

### Wand
- Printed wand is in progress.
- HasLab wand remains the current active wand.

### Attenuator
- Base shell is printed.
- Additional parts are on order.

### Architecture
- ESP32 migration remains planned.
- Internal I2C bus remains the preferred direction for suitable peripherals.

## 2026-09-25

### Attenuator lighting specification
- Added `firmware/attenuator/LIGHTING_SPEC.md` as a Codex-ready V1 behavioral specification: top power/connection indicator, yellow-to-red radiation activity, fictional lower-dome heat/vent/overheat effects, full state table, priority rules, proposed data fields and acceptance scenarios.
- Added `firmware/attenuator/README.md` documenting hardware ownership and unvalidated communication assumptions.
