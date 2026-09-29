# Project Decisions

Committed architectural decisions belong here. Include the date, decision, rationale, and consequences when useful.

## 2026-09-18 — Repository as Source of Truth
The Git repository and its documentation are the canonical durable project context shared between ChatGPT and Codex. Conversation history is supporting context, not the authoritative project record.

## 2026-09-18 — Preserve HasLab Wand During Transition
The HasLab Spengler Wand remains supported in the near term while the printed wand is developed. New pack interfaces should avoid making the eventual wand swap unnecessarily difficult.

## 2026-09-18 — Internal I2C Direction
Use I2C where practical for low-bandwidth internal peripherals. Exact topology, connectors, and device addresses remain subject to implementation and testing.

## 2026-09-28 — Standalone Attenuator Display Bring-up
Use Nano hardware I2C on A4/A5 for the isolated HT16K33/BL28Z POC. Keep the raw display driver separate from test logic and determine physical order by discovery. This does not decide the future local software-I2C bus or implement the complete attenuator.
