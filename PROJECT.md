# Ghostbusters Proton Pack Project

## Purpose
Canonical project context for the custom 3D-printed Ghostbusters proton pack. This repository is the source of truth shared between ChatGPT, Codex, and hands-on build work.

## Project Context Rules
Before starting work:
1. Read this file and relevant subsystem documentation.
2. Treat repository documentation as authoritative when chat history conflicts with it.
3. Do not turn speculative ideas into established decisions.

After completing work:
1. Update PROJECT.md when overall status changes.
2. Record committed architecture choices in DECISIONS.md.
3. Update BOM.md when hardware changes.
4. Update PINOUTS.md when wiring, connectors, voltages, pins, or bus addresses change.
5. Update TODO.md as work is completed or discovered.
6. Add concise cross-session changes to CHANGELOG.md.

Use these labels where useful: CURRENT, PLANNED, EXPERIMENTAL, DEPRECATED.

## Platform
- MK4 Q-Pack TacoShell / motherboard-based custom pack.
- GB1/GB2-inspired appearance.
- Electronics should remain modular, serviceable, and accessible.
- Current HasLab Spengler Wand remains supported while a printed wand is developed.

## Controller
### CURRENT
- Arduino Nano used for current/prototype control.
- Working pack-controller sketch is documented under `firmware/pack-controller/`.
- Firmware uses current-qualified, debounced HasLab wand controls and a five-state pack state machine.
- Common-ground power distribution.

### PLANNED
- ESP32-based main controller.
- Adafruit ESP32 option with STEMMA QT/Qwiic is a leading candidate.
- Account for ESP32 3.3 V logic and level shifting for 5 V devices as required.
- Possible separation of main state-machine work from peripheral/background servicing.
- Wireless firmware update capability is desirable.

## Internal Bus
### PLANNED
Use an internal I2C bus where practical for low-bandwidth peripherals.

Potential nodes include:
- Service panel
- Attenuator
- Battery/voltage monitoring
- Audio control
- Wand interface
- Additional displays/sensors

Document every assigned I2C address in PINOUTS.md. Do not assume an address without verifying it on the actual device.

## Wand
### CURRENT
- HasLab Spengler Wand (1984) remains the active wand.
- Activate and Fire signals have been interfaced to the pack/controller.
- Pack and wand use a common ground.
- A 4-pin JST connection has been accessed for integration.
- Preserve stock wand behavior where practical.

### IN PROGRESS
- New printed wand is being built.

### PLANNED
- Make eventual migration to the printed wand straightforward without unnecessarily coupling pack firmware to HasLab-specific hardware.

## Lighting
### CURRENT
- A 16-pixel power-cell NeoPixel chain is configured; pixels 0–14 animate and pixel 15 is held off.
- Cyclotron uses four physically verified 7-pixel NeoPixel Jewels (28 pixels), matching the current firmware configuration.
- LED power is 5 V.

### PLANNED
- Verify/implement appropriate data-level shifting when controlled from a 3.3 V ESP32.

## Audio
### CURRENT
- CH358D amplifier module.
- Existing pack speaker.
- Current firmware controls an Adafruit Audio FX Sound Board over 9600-baud software serial and uses five event tracks (`T00.WAV`–`T04.WAV`). Confirm the installed board model and amplifier interconnect on the hardware.

### PLANNED
Explore an upgraded audio subsystem supporting:
- Polyphonic playback
- Cross-fading
- Independent effects
- Music playback
- External volume control
- Tsunami-class audio hardware is under consideration.

## Battery and Power
### CURRENT
- 12 V LiPo battery.
- Buck converter.
- Central power distribution/common ground.
- Firmware supports an INA219 at address `0x40` for wand-current qualification and an on-demand 100 kΩ / 33 kΩ battery-divider measurement on A0. Installed values and calibration require physical verification.

### PLANNED
- Calibrate and integrate battery voltage monitoring into pack status/indication.
- Better charging/service access.
- Battery status indication.
- Improved modular power distribution/connectors.

## Attenuator
### IN PROGRESS
- Base shell printed.
- Additional parts ordered.

Desired functions:
- Volume control.
- Switch/button capable of triggering the Ghostbusters theme.
- Integration with the future audio subsystem.
- Potential I2C connection to the main controller.

## Service Panel
### EXPERIMENTAL — LOW PRIORITY
Prototype hardware:
- OLED 12864 display.
- I2C.
- POC uses the SSD1306 7-bit address `0x3C` (equivalent to the `0x78` 8-bit write-address notation); verify on the physical module.
- Arduino Nano used for initial prototype.
- Standalone proof-of-concept firmware is stored under `firmware/POC/OLED/`; it is not integrated with the pack controller.

Potential UI:
- Ghostbusters logo.
- Battery voltage/status.
- Pack state.
- Wand state.
- Audio status.
- Diagnostics/service information.

A larger/color display may be considered later.

## Mechanical / Pack Features
- Electronics are primarily motherboard-mounted.
- Serviceability and quick-disconnects are preferred for shell-mounted devices.
- A service/access area near the upper side/ion-arm region has been explored for charging, battery indication, and firmware/USB access.
- Speaker grille concept uses a heat-sink-like finned appearance rather than a conventional speaker grille.

## Future Ideas
### EXPERIMENTAL
- Fog integration around the N-filter.
- Removable cyclotron.
- More capable service-panel display.
- Expanded internal I2C peripherals.

## Design Principles
1. Preserve compatibility with current hardware unless replacement is intentional.
2. Prefer modular connectors over permanent interconnections.
3. Maintain clearly documented common-ground and power architecture.
4. Clearly distinguish 12 V, 5 V, and 3.3 V domains.
5. Never assume a peripheral is 3.3 V tolerant.
6. Prefer I2C for suitable low-bandwidth internal peripherals.
7. Document connector pinouts and I2C addresses.
8. Prefer configurable pin/address assignments over hard-coded hardware assumptions.
9. Keep prototype/experimental code identifiable.
10. Keep HasLab wand support until the printed-wand migration is intentionally completed.

## Current Project Status

| Subsystem | Status |
|---|---|
| Pack / MK4 TacoShell | CURRENT |
| Arduino Nano controller | CURRENT |
| ESP32 controller | PLANNED |
| HasLab wand | CURRENT |
| Printed wand | IN PROGRESS |
| NeoPixel lighting | CURRENT |
| Pack-controller firmware documentation | CURRENT |
| Attenuator | IN PROGRESS |
| Audio upgrade | PLANNED |
| Battery/power upgrade | PLANNED |
| Service panel | EXPERIMENTAL — LOW PRIORITY |
| Fog system | EXPERIMENTAL |
| Removable cyclotron | EXPERIMENTAL |
