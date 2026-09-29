# Attenuator Firmware (PLANNED)

The attenuator is a self-contained Arduino Nano peripheral for the proton pack. It reads its own encoder, encoder pushbutton and toggle switches and drives its own indicator NeoPixels and 28-segment yellow bargraph. The main pack controller owns authoritative pack/audio state; the attenuator translates that state into local display and lighting effects.

**Codex implementation references:**

- [CONTROLS_SPEC.md](CONTROLS_SPEC.md) — V1 encoder, push-to-mute, theme play/stop toggle, manual vent/purge toggle, event semantics, startup safety and acceptance tests.
- [LIGHTING_SPEC.md](LIGHTING_SPEC.md) — V1 top connection lamp, radiation lens and simulated-warning lower dome, including vent/overheat effects.

Both are planned behavioral specifications, not implemented firmware.

## Proposed integration

- Pack-facing link: Nano as an I2C peripheral on the planned shared pack bus; ESP32 is the master and polls inputs/sends state.
- The Nano must control its own outputs, not rely on the ESP32 to stream LED frames or drive bargraph segments.
- Local HT16K33 driver beside the BL28Z bargraph; a **separate local software I2C bus** is proposed if the Nano hardware I2C interface is used in peripheral mode. Verify library compatibility and timing experimentally.
- Physical packaging: Nano and power/distribution on the removable base plate; bargraph/driver near the shell window; detachable internal harnesses.
- Current HasLab wand remains supported. The eventual printed wand may reuse the same *pattern*, subject to bus and timing validation.
- The proposed 5-pin GX12 loom assigns 5V, GND, SDA, SCL and one spare. **Pin numbering, voltage interface, address and bus pull-ups are not finalized**; do not connect the 5 V Nano I2C interface directly to 3.3 V ESP32 lines without confirming level shifting.

See the top-level [PROJECT.md](../../PROJECT.md) and [PINOUTS.md](../../PINOUTS.md) for the current-versus-planned distinction and verified wiring. Do not reuse the current main pack Nano pin map as the attenuator Nano pin map.
