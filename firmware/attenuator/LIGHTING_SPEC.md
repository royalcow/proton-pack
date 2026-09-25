# Attenuator Lighting Specification — V1

**Status:** PLANNED behavioral specification, agreed 2026-09-25; not implemented or hardware-validated.  
**Owner:** Attenuator Arduino Nano. **Source of pack state:** main pack controller (future ESP32).  
**Scope:** Top status light, radiation lens and lower Carclo dome only. The yellow BL28Z bargraph, audio and physical controls are related but specified separately.

## Design intent and responsibilities

| Output | Meaning | Control authority |
| --- | --- | --- |
| Top status indicator | Attenuator has power and a live connection to the pack controller | Nano detects valid communication locally |
| Radiation lens | What the pack is doing / how energetically it is operating | Nano animates from pack state, simulated heat and power level |
| Lower dome | **Fictional/in-universe** thermal warning, overheat, venting and recovery | Nano animates from pack state and simulated heat |

The dome is **not** a generic real electronic fault indicator. A lost bus connection appears on the **top** lamp, not as a fictional overheat in the dome. These three outputs can be addressed individually even if their NeoPixels share one data chain; physical chain versus separate GPIO allocation is still a wiring decision.

The main controller sends high-level state, not colors or per-pixel frames. The Nano owns all timing, local rendering and brightness adjustment.

## Complete state table

| Condition | Top: power / connection | Radiation: yellow-to-red pack activity | Dome: simulated warning |
| --- | --- | --- | --- |
| Attenuator unpowered | Off | Off | Off |
| Powered, no valid connection yet | Pulsing amber | Off | Off |
| Connected; pack off | Solid green | Off | Off |
| Pack startup | Solid green | Brief yellow ignition flickers settling into a glow | Brief amber self-test |
| Pack idle / ready | Solid green | Dim warm-yellow, slightly irregular breathing | Off |
| Firing | Solid green | Bright yellow/orange irregular flicker with short red accents; scales with power level | Off if heat below warning threshold |
| Heat building | Solid green | Orange becomes more frequent, with increasing red spikes and instability | Slow/fast amber pulse according to heat |
| Overheat | Solid green | Rapid unstable red/orange surges, brief dropouts and occasional yellow flashes | Rapid red flashes |
| Venting | Solid green | Orange/yellow discharge flashes diminishing toward yellow | Distinct cool blue/white discharge pulses |
| Recovery | Solid green | Gradually settles to the idle yellow pattern | Amber fade, then off |
| Pack shutdown | Solid green while connected | Yellow fades out | Off |
| Connection lost / communication timeout | Pulsing amber | Off after timeout | Off |

**Top status remains green through fictional overheat/vent events** as long as the pack communications link is healthy. Pack-off and link-disconnected are distinct: the pack may report OFF while still connected.

## Top lamp: connectivity, not diagnostic severity

- **No supply:** physically off.
- **Nano powered but waiting for a valid pack exchange:** amber breathing/pulse.
- **Established and recently valid communications:** solid green. Power alone does not qualify as connected.
- **No valid exchange for approximately 1.5 seconds:** return to pulsing amber and extinguish the radiation and dome outputs; resynchronize from a complete pack-state snapshot when communication resumes.
- 1.5 s is a proposed configurable timeout to tune at bench test, not a proven bus limit. No traffic must be mistaken for confirmed connection.

## Radiation lens: color and animation vocabulary

The chosen visual theme is **yellow -> orange -> red**, not green/blue “proton energy.”

| State | Suggested baseline animation |
| --- | --- |
| Startup | Several short yellow ignition flickers, then a gradual ramp |
| Idle | Warm yellow at approximately 15–25% brightness; slow *irregular* breathing, occasional small orange flicker |
| Firing | Yellow/orange at approximately 40–90% brightness; rapid irregular flicker, brief red peaks |
| Heat building | Orange dominates more often; red peaks and frequency increase as heat rises |
| Overheat | Strong unstable orange/red, intermittent dropouts and brief bright yellow spikes |
| Venting | A few sharp orange/yellow discharges, then diminishing brightness back toward idle |
| Recovery/shutdown | Smoothly settle or fade out according to pack state |

These percentages/colors are *tuning starting points*; verify through the installed diffuser/lens. Avoid an always-red light: red means escalation.

### Power-level scaling

Preserve the yellow/orange/red vocabulary across levels 1–5; vary the intensity and activity while firing instead of assigning a distinct hue per level.

| Level | Firing effect |
| --- | --- |
| 1 | Subdued yellow/orange flicker |
| 2 | Moderate brightness/activity |
| 3 | Brighter, occasional red peaks |
| 4 | Faster/stronger flicker |
| 5 | Intense flicker with more frequent red accents |

The radiation light should indicate the pack's work, not duplicate the numerical bargraph.

## Lower dome: simulated heat and vent effect

Normally **off**. For normal operation outside a higher-priority event, use the simulated 0–100 heat value as a first-pass guide:

| Heat | Dome |
| --- | --- |
| 0–59 | Off |
| 60–79 | Slow amber pulse (~1.5 s per pulse) |
| 80–99 | Faster amber pulse, optionally occasional red accent |
| 100 or explicit OVERHEAT | Rapid red warning flashes |

The main controller's explicit `OVERHEAT` state takes priority even if a stale heat value is below 100. **VENTING overrides OVERHEAT** and changes the dome to blue/white pulses, visibly differentiating pressure release from heat buildup. Recovery fades through amber before turning off. Colors and pulse rates are tunable.

No real thermal/battery protection or physical alert is delegated to this prop animation.

## Priority and state resolution

Render connectivity independently. If disconnected, do not continue playing an old pack animation. For pack-derived visual state, apply this order:

1. Link timeout / unsynchronized: radiation and dome off; top amber.
2. Pack OFF: radiation and dome off.
3. VENTING.
4. OVERHEAT.
5. Heat buildup (where applicable).
6. FIRING.
7. IDLE / ready.

STARTUP, RECOVERY and SHUTDOWN have their named one-shot/transition effects. Define their transition behavior explicitly; they should not be accidentally swallowed by an unrelated general idle case. Heat warning can overlay FIRING: firing radiation continues with increasing orange/red while dome displays heat warning until the higher-priority overheat/vent state arrives.

## Suggested pack-to-attenuator data contract (not yet a fixed wire format)

| Field | Values / semantics |
| --- | --- |
| `packState` | OFF, STARTUP, IDLE, FIRING, OVERHEAT, VENTING, RECOVERY, SHUTDOWN (logical values; numeric encoding TBD) |
| `powerLevel` | 1–5 |
| `heatLevel` | 0–100, *simulated* |
| `brightness` | 0–255; user/system maximum for local effects |
| `flags` | Reserved for future use |

The existing pack firmware currently has a different five-state vocabulary (HOLDING/BOOTING/ACTIVE/FIRING/SHUTTING_DOWN). **Do not assume the enums are already wire-compatible.** Define the mapping when implementing the upgraded pack protocol. Do not require per-frame color data over the shared bus.

## Firmware implementation guidance for Codex

- Keep a local, immutable/copyable current snapshot of the last valid pack state and a timestamp of the last valid pack communication; synchronize before declaring a live connection.
- Keep each effect's independent timebase and render all pixels non-blockingly with `millis()`; avoid `delay()` in the animation path.
- Compute the three output colors from state/heat/time and call the NeoPixel `show()` outside any I2C receive/request ISR or callback.
- If lights are chained, use symbolic indices (e.g. `PIXEL_TOP`, `PIXEL_RADIATION`, `PIXEL_DOME`) and send a coherent frame; chain order is a wiring constant, not an animation requirement. Separate GPIO chains are also permitted if routing is cleaner.
- Account for NeoPixel interrupt masking on the ATmega328P when testing the pack-facing I2C peripheral and local software-I2C HT16K33 driver. Confirm library compatibility, signal levels/pull-ups and actual cable reliability before treating the shared bus as finalized.
- Distinguish **pack off**, **pack powering down**, **accessory connected**, and **connection lost**. A simulated overheat should not turn the top lamp amber by itself.
- On reconnection, request/apply a full state snapshot rather than replaying old animations/events.
- Make palette, max brightness, flicker parameters, pulse periods, heat thresholds and timeout named tuning constants.

### Acceptance scenarios to test

1. Nano powers on alone: top pulses amber, radiation/dome remain off.
2. Valid connection + pack OFF: top steady green, radiation/dome off.
3. Idle and levels 1–5 firing: warm yellow idle; firing intensity/activity scales without changing the base color vocabulary.
4. Cross simulated heat 59 -> 60 -> 80 -> 100: dome off -> slow amber -> faster amber -> red; radiation grows more unstable.
5. Pack explicitly enters OVERHEAT below heat=100: red dome still wins.
6. OVERHEAT -> VENTING -> RECOVERY -> IDLE: immediate blue/white dome on vent, fading back to off; radiation discharges and returns to yellow.
7. Disconnect during firing or vent: after timeout top amber and both pack-derived lights off; reconnection resynchronizes cleanly.
8. Concurrent encoder activity, local HT16K33 updates and I2C polling must not lock up or materially disrupt effects.
9. No NeoPixel update or blocking work runs in the I2C callback.

### Out of scope / still to confirm

- Actual LEDs, RGB ordering, pixels-per-zone and whether there is one chained output or multiple GPIO chains.
- Final bus address, packet/ack encoding, bus pull-ups, 3.3 V/5 V level shifting, heartbeat interval and software-I2C implementation.
- Exact final palette/brightness through the installed lenses and diffuser.
- Bargraph display/volume/mute behavior and music-switch semantics.
