# PKE Meter STL Files

## Original source

These files are based on **Ghostbusters PKE Meter for Arduino** by **CountDeM0net** on Thingiverse.

- Thingiverse model: https://www.thingiverse.com/thing:2338494
- Thingiverse files: https://www.thingiverse.com/thing:2338494/files
- Thing ID: `2338494`

The Thingiverse page is the canonical source for the original models, build notes, attribution, and licensing information.

## Repository modifications

### `front_top_repaired.stl`

The original `front_top.stl` had mesh/topology issues. This repository contains a repaired version:

- Closed the unintended open boundary defects.
- Corrected inconsistent face orientation/topology.
- Preserved the intended display opening.
- Preserved the original overall dimensions.
- Validated as a watertight, single-volume STL with no remaining boundary or non-manifold edges.

Use `front_top_repaired.stl` in place of the original `front_top.stl`.

## Original STL package

The working PKE prototype package includes:

- `back_bottom.stl`
- `back_top.stl`
- `buttons_filled.3mf`
- `buttons_filled.stl`
- `front_bottom.stl`
- `front_top.stl` — replace with the repaired repository version
- `pke_meter_belt_holder.stl`
- `screen_holder.stl`
- `wings_cover_left.stl`
- `wings_cover_right.stl`
- `wings_left.stl`
- `wings_right.stl`

## Project notes

This model is being used as the fast prototype platform for the custom PKE Meter project. Planned prototype electronics include an Arduino Nano, 128x64 OLED, motorized wings, LEDs, and a manual proximity input. Future versions may move to ESP32 and add BLE-based ghost detection and wireless integration with the proton pack.
