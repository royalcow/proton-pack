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
| Pack-controller INA219 | 0x40 | FIRMWARE DEFAULT | `Adafruit_INA219::begin()` default; absence deliberately holds wand state off |

Bus signals:
- SDA
- SCL
- Power as appropriate for each device
- GND

## Lighting
| Nano pin | Circuit | Firmware configuration | Status |
| --- | --- | --- | --- |
| D2 | Power-cell NeoPixel data | 16 pixels, GRB/800 kHz; pixels 0–14 animated, pixel 15 held off | PHYSICALLY VERIFIED |
| D3 | Cyclotron NeoPixel data | Four chained 7-pixel Jewels (28 pixels), GRB/800 kHz | PHYSICALLY VERIFIED — four Jewels installed |

Both chains use 5 V power and common ground. D2 as power-cell data and D3 as cyclotron data are physically verified. Connector pin order, power injection, and protection components remain to be verified.

## Wand
HasLab wand Activate and Fire signals are connected to the controller, with
common ground. A 4-pin JST connection is available, but its physical pin order,
wire colors, and signal voltage levels are not yet verified.

| Nano pin | Signal | Input mode | Firmware behavior |
| --- | --- | --- | --- |
| D4 | Activate control | `INPUT`; external pull-down required | Active-low at read layer; interpreted jointly with D5 |
| D5 | Fire control | `INPUT`; external pull-down required | Active-low at read layer; interpreted jointly with D4 |

With INA219 current confirming wand power, LOW/LOW arms the controls, HIGH/LOW
activates the pack, and HIGH/HIGH requests Fire. LOW/HIGH is not a valid active
pattern. The controls share a 3 ms debounce. Firmware meaning does not establish
the physical order of these signals on the 4-pin JST.

The INA219 is intended ahead of the wand power switch: VIN+ to buck-converter
5 V, VIN- to the switch's positive feed, VCC to always-on Nano 5 V, and GND to
common ground. Wand negative stays directly on common ground.

## Pack-controller auxiliary and audio pins

| Nano pin | Connection | Mode / notes | Status |
| --- | --- | --- | --- |
| D6 | Voltage-check momentary button | `INPUT_PULLUP`; other terminal to GND | CONFIRMED IN FIRMWARE |
| A0 | Battery divider midpoint | Nominal 100 kΩ battery-to-A0 and 33 kΩ A0-to-GND | FIRMWARE VALUES; installation unverified |
| D9 | Audio FX RST | Sound-board reset | CONFIRMED IN FIRMWARE |
| D10 | Audio FX RX | Nano software-serial TX, 9600 baud | CONFIRMED IN FIRMWARE |
| D11 | Audio FX TX | Nano software-serial RX, 9600 baud | CONFIRMED IN FIRMWARE |
| D12 | Audio FX ACT | `INPUT_PULLUP`; LOW while playing | CONFIRMED IN FIRMWARE |
| D13 | Nano built-in LED | Heartbeat output, toggled every 500 ms | CONFIRMED IN FIRMWARE |

Audio FX UG must be tied to GND to select UART mode. Audio FX, Nano, amplifier,
wand, sensors, and LED supplies must share ground. Verify the exact Sound Board
variant's 5 V input requirements before connection.

## Connector Registry
Add connector family, pin numbering orientation, signal, voltage, and mating-part information as interfaces are finalized.
