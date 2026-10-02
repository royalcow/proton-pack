# Project Decisions

Committed architectural decisions belong here. Include the date, decision, rationale, and consequences when useful.

## 2026-09-18 — Repository as Source of Truth
The Git repository and its documentation are the canonical durable project context shared between ChatGPT and Codex. Conversation history is supporting context, not the authoritative project record.

## 2026-09-18 — Preserve HasLab Wand During Transition
The HasLab Spengler Wand remains supported in the near term while the printed wand is developed. New pack interfaces should avoid making the eventual wand swap unnecessarily difficult.

## 2026-09-18 — Internal I2C Direction
Use I2C where practical for low-bandwidth internal peripherals. Exact topology, connectors, and device addresses remain subject to implementation and testing.

## 2026-09-25 — Attenuator Lighting Roles (PLANNED)
- Top LED indicates accessory power and live connection to the main pack controller: green connected, amber pulse waiting/disconnected, off unpowered.
- Radiation lens indicates pack operating state using yellow-to-orange-to-red effects, with brightness/activity scaling by power and simulated heat.
- Lower dome is reserved for **fictional** thermal/overheat/vent indications rather than generic real hardware diagnostics.
- The local Nano owns animation timing and rendering; the pack supplies high-level state. Specification: [`firmware/attenuator/LIGHTING_SPEC.md`](firmware/attenuator/LIGHTING_SPEC.md).
- Pack-facing shared I2C and local software I2C remain implementation proposals requiring electrical/timing validation, not confirmed wiring.

## 2026-09-25 — Attenuator V1 Control Assignments (PLANNED)
- Flat-paddle toggle 1 requests Ghostbusters theme play on OFF -> ON and stop on ON -> OFF. Track ending while still ON does not auto-replay.
- Flat-paddle toggle 2 requests manual vent/purge once on OFF -> ON; OFF rearms and does not abort an active vent. Staying ON does not repeat.
- At boot/reconnection, both latching switch positions are sampled without creating synthetic play/vent requests. Main pack owns audio actuation and pack-wide vent state; the Nano handles local display effects after state confirmation.
- Details and acceptance scenarios: [`firmware/attenuator/CONTROLS_SPEC.md`](firmware/attenuator/CONTROLS_SPEC.md). Wire protocol and timings remain open.

## 2026-10-02 — Attenuator Owns Master Volume
- The attenuator Nano is the V1 authoritative source for user-selected master volume on a 0–100 scale.
- Encoder movement is handled locally; the pack does not receive encoder deltas and does not need to know what physical control produced the volume.
- Encoder push provides a local mute convenience only. The attenuator preserves/restores its saved selected volume, but the pack receives no separate mute flag: mute is represented solely as **effective volume 0**.
- While locally muted, encoder movement may change the saved volume and bargraph marker while pack-facing effective volume remains 0. Unmute sends the latest saved absolute volume.
- Pack-facing volume updates are absolute and idempotent. Reconnect sends the current effective volume rather than replaying historical deltas or mute events.
- For V1 the attenuator is the sole master-volume writer. Any future second volume control requires an explicit ownership/synchronization design.

## 2026-09-28 — Standalone Attenuator Display Bring-up
Use Nano hardware I2C on A4/A5 for the isolated HT16K33/BL28Z POC. Keep the raw display driver separate from test logic and determine physical order by discovery. This does not decide the future local software-I2C bus or implement the complete attenuator.

## 2026-10-01 — Attenuator Toggle Pin Assignments

- Assign flat-paddle toggle 1 (theme play/stop) to D5 and toggle 2 (manual vent/purge) to D6 on the dedicated attenuator Nano.
- Configure both as `INPUT_PULLUP`: ON closes the input to common GND; OFF leaves it open. For SPDT switches, use common and the selected ON contact; leave the unused throw disconnected.
- These assignments leave the encoder on D2/D3, encoder button on D4, and local HT16K33 bus on A4/A5. They do not change the main pack pin map.
- The standalone sketch simulates accepted theme/vent requests locally. Final pack communication remains unimplemented.
- Pin assignments are decided and implemented; installed wire colors, switch orientation, continuity, and physical operation remain unverified.
