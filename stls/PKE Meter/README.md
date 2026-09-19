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

### `screen_holder_aperture_minus_1p5mm_vertical.stl`

Current preferred screen-holder geometry for the prototype OLED fit.

The holder's overall dimensions remain unchanged from the original. Only the display aperture was reduced vertically by **1.5 mm total**, centered on the original opening:

- Overall dimensions: **34.950 × 40.312 × 1.858 mm**
- Original display opening: **26.998 × 17.592 mm**
- Revised display opening: **26.998 × 16.092 mm**
- Horizontal opening dimension unchanged.
- Top edge moved inward by 0.75 mm.
- Bottom edge moved inward by 0.75 mm.
- Mesh validated as watertight with consistent winding.

Prototype fit check: this revised opening fits the current 128×64 OLED reasonably well and is the preferred geometry for continued prototyping.

Local generated-file verification:

- SHA-256: `dcacb8c6cdf5c6035c79d0ed45789929e3a3046a10e49d3449f0254d773ee019`
- Size: `84684` bytes

> Note: STL files in this repository are tracked with Git LFS. The revised screen-holder binary should be added through the normal local Git/LFS workflow so the LFS object is uploaded correctly.

## Original STL package

The working PKE prototype package includes:

- `back_bottom.stl`
- `back_top.stl`
- `buttons_filled.3mf`
- `buttons_filled.stl`
- `front_bottom.stl`
- `front_top.stl` — replace with the repaired repository version
- `pke_meter_belt_holder.stl`
- `screen_holder.stl` — use the revised aperture version for the current OLED prototype
- `wings_cover_left.stl`
- `wings_cover_right.stl`
- `wings_left.stl`
- `wings_right.stl`

## Project notes

This model is being used as the fast prototype platform for the custom PKE Meter project. Planned prototype electronics include an Arduino Nano, 128x64 OLED, motorized wings, LEDs, and a manual proximity input. Future versions may move to ESP32 and add BLE-based ghost detection and wireless integration with the proton pack.
