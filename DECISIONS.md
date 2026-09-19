# Project Decisions

Committed architectural decisions belong here. Include the date, decision, rationale, and consequences when useful.

## 2026-09-18 — Repository as Source of Truth
The Git repository and its documentation are the canonical durable project context shared between ChatGPT and Codex. Conversation history is supporting context, not the authoritative project record.

## 2026-09-18 — Preserve HasLab Wand During Transition
The HasLab Spengler Wand remains supported in the near term while the printed wand is developed. New pack interfaces should avoid making the eventual wand swap unnecessarily difficult.

## 2026-09-18 — Internal I2C Direction
Use I2C where practical for low-bandwidth internal peripherals. Exact topology, connectors, and device addresses remain subject to implementation and testing.

## 2026-09-19 — ESP32-S3 Controller Architecture
Migrate the main controller toward ESP32-S3 while preserving the current Nano behavior as the reference implementation. Favor an event-driven/FreeRTOS design that separates pack state/input handling from effects and peripheral services. The exact task/core affinity remains an implementation detail until measured on hardware.

## 2026-09-19 — Development Controller
Use Adafruit ESP32-S3 Reverse TFT Feather #5691 as the development/prototyping board. Its integrated TFT can expose state, input, I2C, battery, and OTA diagnostics during development. Adafruit ESP32-S3 Feather #5477 remains a preferred eventual production controller when available; the #5691 can later be reused for another prop/project.

## 2026-09-19 — NeoPixel Logic Level
Keep NeoPixel power on the 5 V domain and level-shift ESP32 3.3 V data with an SN74AHCT125N powered from 5 V. Two channels are required for the current independent power-cell and cyclotron data circuits; unused AHCT125 channels should be disabled rather than left floating.
