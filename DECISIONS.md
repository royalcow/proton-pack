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
