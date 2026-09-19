# Pinouts, Buses, and Connectors

Do not treat unverified values as confirmed wiring.

## Power Domains
- Battery: 12 V LiPo
- 5 V domain: used by NeoPixel lighting and applicable peripherals
- 3.3 V logic domain: expected for future ESP32 controller
- Common ground is used across the current pack/controller integration.

## I2C

| Device | Address | Status | Notes |
|---|---:|---|---|
| OLED 12864 service-panel prototype | 0x78 | CURRENT SELECTION | Verify whether documentation/code represents 8-bit address notation vs 7-bit library address before final firmware architecture |

Bus signals:
- SDA
- SCL
- Power as appropriate for each device
- GND

## Lighting
Two 7-pixel NeoPixel Jewel circuits are currently used. Exact controller pins should be added when verified from firmware/wiring.

## Wand
HasLab wand Activate and Fire signals are connected to the controller, with common ground. A 4-pin JST connection is available. Exact pin order and electrical behavior should be documented here after verification.

## Connector Registry
Add connector family, pin numbering orientation, signal, voltage, and mating-part information as interfaces are finalized.
