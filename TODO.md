# Project TODO

## In Progress
- Finish attenuator hardware build as ordered parts arrive.
- Continue printed-wand build.

## Controller
- Select exact ESP32 board.
- Define 3.3 V / 5 V level-shifting strategy.
- Define internal I2C physical wiring/connectors.
- Migrate controller architecture from Nano when ready.

## Wiring / Documentation
- Physically verify the firmware-documented Arduino pin assignments and connector pin orders.
- Document HasLab 4-pin JST pinout and signal behavior.
- Build connector registry.
- Document power distribution and fuse/charging topology.

## Audio
- Evaluate upgraded polyphonic/cross-fade audio hardware.
- Define attenuator-to-audio control interface.

## Battery
- Define battery upgrade path.
- Verify/install and calibrate the firmware's 100 kΩ / 33 kΩ voltage-monitoring divider.
- Plan accessible charging/status interface.

## Service Panel — Low Priority
- Preserve working Nano + OLED prototype.
- Evaluate larger/color display options.
- Define eventual I2C integration with main controller.

## Future / Experimental
- N-filter fog integration.
- Removable cyclotron architecture.
