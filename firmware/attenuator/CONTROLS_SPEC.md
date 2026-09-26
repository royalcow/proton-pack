# Attenuator Controls and Actions — V1

**Status:** PLANNED behavioral specification, agreed 2026-09-25; not implemented.  
**Local controller:** dedicated Arduino Nano. **Authoritative state and actuation:** main pack controller (future ESP32).  
**Related:** [LIGHTING_SPEC.md](LIGHTING_SPEC.md) for the three indicator lights and simulated vent/overheat appearance.

## Control assignments

| Hardware | V1 function | Attenuator responsibility | Pack responsibility |
| --- | --- | --- | --- |
| Rotary encoder, rotation | Master volume up/down | Debounce/decode and report signed relative step delta | Apply volume adjustment to audio subsystem, clamp to supported range, report actual volume |
| Encoder pushbutton | Toggle mute | Report one debounced press event | Toggle authoritative mute state and report it |
| Flat-paddle toggle 1 | Ghostbusters theme play/stop | Report physical switch position and debounced transitions | Start/stop theme playback and report actual music state |
| Flat-paddle toggle 2 | Manual vent/purge | Report physical switch position and debounced OFF -> ON event | Accept/coordinate pack-wide fictional vent sequence (audio, pack lighting, attenuator state, future optional fog) |
| Yellow BL28Z 28-segment bargraph + HT16K33 | Volume and temporary effects display | Render locally from acknowledged pack state | Provide actual volume and high-level states/events |

The Nano owns local display updates and inputs. It **does not play the theme, directly activate pack-level vent hardware, or treat a switch position as proof the pack accepted a request**.

## Volume and mute

- Rotation reports an accumulated signed `ENCODER_DELTA` (e.g., `+3`), rather than a stream of independent plus/minus messages. The pack determines step size and clamps the result.
- Normal bargraph indication reflects **confirmed master volume**, not a guessed local value. V1 proposed logical scale is 0–100, with exact sound hardware and step size TBD.
- Encoder press emits a single debounced `MUTE_PRESS` / `MUTE_REQUEST` event; the pack returns actual mute status. Do not repeatedly toggle while held.
- Muting preserves the saved volume so unmute can restore the previous level. The bargraph may briefly indicate mute but should not imply the saved volume is zero.
- Keep local knob feedback responsive, but reconcile any preview against the pack's authoritative `VOLUME`/`MUTED` snapshot.
- The exact bargraph segment mapping, mute pattern and temporary sequence animations are to be defined with the real BL28Z/HT16K33 hardware.

## Theme toggle (physical latching ON/OFF switch)

- **OFF -> ON** requests `MUSIC_PLAY` / theme start.
- **ON -> OFF** requests `MUSIC_STOP`.
- Report actual position separately as `THEME_SWITCH_ON`, so reconnect/startup can synchronize hardware state without synthesizing a press/toggle event.
- If the theme ends naturally while the toggle remains ON, **do not automatically restart**. Return to OFF, then ON, to request another play.
- If pack startup or attenuator reconnection finds the switch already ON, **do not autoplay** merely from its existing level. Initialize the edge detector from the sampled physical position.
- The main pack/audio subsystem owns playback, interruption policy and final `MUSIC_STATE`; the attenuator displays that real state if needed. Future decisions about interaction with other sounds are out of scope.

## Manual vent/purge toggle (physical latching ON/OFF switch)

- **OFF -> ON** emits **one** debounced `VENT_REQUEST`. This is a one-shot action, **not** a continuous “vent while ON” level.
- Remaining ON does **not** retrigger or loop the sequence; the user must return to OFF to rearm.
- ON -> OFF rearms for the next activation; **it is not an abort command** in V1.
- Booting/reconnecting with the toggle already ON does **not** auto-vent. Baseline the current position on synchronization and await a new OFF -> ON edge.
- The Nano does not begin a simulated accepted vent based solely on switch movement: the main pack may accept/reject/defer it depending on actual state. Display pack-reported `VENTING` / `RECOVERY` or other authoritative result.
- During an accepted vent, the pack coordinates any sound/pack effects and reports state to the attenuator. The local Nano runs the radiation/dome effects in [LIGHTING_SPEC.md](LIGHTING_SPEC.md), with a possible **temporary bargraph animation** that returns to confirmed volume display at the end.
- This switch may later trigger N-filter fog through the **pack controller** when that separate feature is built. No fog output or real pressure/thermal protection is part of V1.
- Exact time profile (buildup, vent, recovery), admissible pack states, sound cue and bargraph effect remain **to be tuned/implemented**, not committed timings.

## Message semantics (transport/wire format still TBD)

The contemplated bus is main ESP32 as I2C master and attenuator Nano as addressed I2C peripheral. The ESP32 polls input state/events and sends authoritative state; the Nano controls its own HT16K33 on a separate local bus if this topology validates.

**Attenuator -> pack:** signed encoder delta; debounced mute press; theme physical state and transitions; vent physical state and request edge; optional input snapshot/sequence.  
**Pack -> attenuator:** authoritative volume, mute, music-playing state, pack state, simulated heat/power, display brightness and accepted vent-state progression.

Because the latching switches encode both **level** and **edge**, do not collapse them into an unqualified `MUSIC_TOGGLE` or `VENT_ACTIVE` flag. Use event sequence IDs and acknowledgement/retry-safe processing for one-shot requests and encoder deltas, so an I2C retry cannot play/vent/toggle twice. Snapshots on startup/reconnect must not replay historical edges. Final wire format, I2C address, command IDs and retry rules are still open.

## Acceptance scenarios for Codex

1. Turn knob +3/-2: pack applies signed deltas once, then reports the actual bounded volume; bargraph follows it.
2. Press and hold knob: mute toggles once, then only after release and a new press; previous volume is retained.
3. Theme OFF -> ON starts once; staying ON does not replay after track end; ON -> OFF requests stop; another OFF -> ON requests a new play.
4. Vent OFF -> ON requests one vent; remaining ON causes no repeats; OFF rearms but does not abort active vent; next OFF -> ON requests another only if pack accepts it.
5. At boot or reconnect, either switch found ON causes **no synthetic play/vent event**; physical levels are still reported.
6. Duplicate polling/retries must not apply a relative delta or edge twice.
7. Rejected/deferred vent does not falsely show VENTING; an accepted vent transitions the dome/radiation animations via pack-reported states and eventually restores volume display.
8. Disconnect during use: apply connection behavior from LIGHTING_SPEC; reconnect from a full authoritative state without replaying old requests.

## Not decided yet

Final encoder step scale/acceleration, music interruption policy, theme sound hardware, vent timing/sound and bargraph pattern, I2C packet format and pin/address mapping, and future fog actuation. Keep these configurable; do not treat examples as electrically or behaviorally verified implementation.
