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
- Rotary encoder adjusts authoritative pack master volume; encoder push requests mute/unmute.
- Flat-paddle toggle 1 requests Ghostbusters theme play on OFF -> ON and stop on ON -> OFF. Track ending while still ON does not auto-replay.
- Flat-paddle toggle 2 requests manual vent/purge once on OFF -> ON; OFF rearms and does not abort an active vent. Staying ON does not repeat.
- At boot/reconnection, both latching switch positions are sampled without creating synthetic play/vent requests. Main pack owns audio and pack-wide vent state; the Nano handles local display effects after state confirmation.
- Details and acceptance scenarios: [`firmware/attenuator/CONTROLS_SPEC.md`](firmware/attenuator/CONTROLS_SPEC.md). Wire protocol and timings remain open.
