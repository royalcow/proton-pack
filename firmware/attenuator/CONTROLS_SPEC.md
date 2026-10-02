# Attenuator Controls and Actions — V1

**Status:** PLANNED behavioral specification; local encoder/bargraph behavior is implemented in the standalone prototype, pack communication is not yet implemented.  
**Local controller:** dedicated Arduino Nano. **Volume authority:** attenuator Nano. **Pack authority:** audio actuation, theme playback, pack-wide vent state, and other pack state.  
**Related:** [BARGRAPH_SPEC.md](BARGRAPH_SPEC.md) for volume/mute/vent display behavior and [LIGHTING_SPEC.md](LIGHTING_SPEC.md) for the three indicator lights and simulated vent/overheat appearance.

## Control assignments

| Hardware | V1 function | Attenuator responsibility | Pack responsibility |
| --- | --- | --- | --- |
| Rotary encoder, rotation | Master volume up/down | Debounce/decode, update local selected volume 0–100, update bargraph, send resulting absolute effective volume | Apply received absolute volume to audio subsystem |
| Encoder pushbutton | Local mute/unmute convenience | Locally save/restore selected volume; while muted send effective volume 0; render mute marker | No separate mute state required; volume 0 means silent |
| Flat-paddle toggle 1 | Ghostbusters theme play/stop | Report physical switch position and debounced transitions | Start/stop theme playback and report actual music state |
| Flat-paddle toggle 2 | Manual vent/purge | Report physical switch position and debounced OFF -> ON event | Accept/coordinate pack-wide fictional vent sequence (audio, pack lighting, attenuator state, future optional fog) |
| Yellow BL28Z 28-segment bargraph + HT16K33 | Volume and temporary effects display | Render locally from attenuator-owned volume/mute state plus pack-reported high-level effects | No per-segment control; provide only pack state needed for temporary effects |

The pack does **not** need to know that the master volume is controlled by a rotary encoder. The attenuator owns that user interaction and sends only the resulting absolute effective volume. Likewise, the encoder pushbutton does not create a pack-facing mute event; at the pack boundary, mute is simply effective volume 0.

## Encoder hardware wiring status

As of 2026-09-30, the rotary encoder harness is physically soldered and its wire colors are recorded: D2/rotary phase A = **blue** at the top-left outer contact; D3/rotary phase B = **white** at the top-right outer contact; D4/pushbutton signal = **yellow** at the bottom-left contact; GND = **black** from the bottom-right switch contact. The top-center rotary common is locally tied to that bottom-right GND contact. Configure D2/D3/D4 as `INPUT_PULLUP`. The user confirmed correct rotation direction on 2026-09-30 with `ENCODER_DIRECTION = 1` in the standalone sketch.

## Volume and local mute behavior

- The attenuator owns the user-selected master-volume value on a logical **0–100** scale.
- Encoder rotation updates that value locally. The pack receives the resulting **absolute effective volume**, not relative encoder deltas.
- Normal bargraph indication follows the attenuator's local selected volume immediately; it does not wait for the pack to echo the value back.
- The pack/audio subsystem applies the absolute value it receives to its actual audio hardware. The exact gain curve remains an audio-subsystem concern.
- Encoder pushbutton provides a **local mute convenience mode** only. It is useful to remember what value should be restored and to drive the breathing saved-volume marker, but it is not a pack protocol field.
- Entering local mute preserves the selected/saved volume and sends **effective volume 0** to the pack.
- While locally muted, encoder rotation changes the saved volume and moves the bargraph marker without making the pack audible; effective volume remains 0.
- Leaving local mute sends the saved volume as the new effective volume and restores the normal filled-volume display.
- If the selected volume is intentionally turned to 0 without using the pushbutton, the pack still simply receives volume 0. The pack does not distinguish this from mute.
- Reconnect/resynchronization sends the current absolute effective volume. There is no encoder-delta replay and no mute-event replay.
- For V1, the attenuator is the **sole authoritative source for master volume**. If a future service panel, wand, or other control also changes volume, an explicit ownership/synchronization rule must be added rather than silently introducing multiple writers.

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
- During an accepted vent, the pack coordinates any sound/pack effects and reports state to the attenuator. The local Nano runs the radiation/dome effects in [LIGHTING_SPEC.md](LIGHTING_SPEC.md) and the temporary bargraph sequence in [BARGRAPH_SPEC.md](BARGRAPH_SPEC.md), then restores its local volume display.
- This switch may later trigger N-filter fog through the **pack controller** when that separate feature is built. No fog output or real pressure/thermal protection is part of V1.

## Message semantics (transport/wire format still TBD)

The contemplated bus is main ESP32 as I2C master and attenuator Nano as addressed I2C peripheral. The ESP32 polls/receives user-facing state/actions and sends authoritative pack state; the Nano controls its local HT16K33 independently.

**Attenuator -> pack:**
- absolute **effective volume 0–100**;
- theme physical state/transitions;
- vent physical state/request edge;
- optional input snapshot/sequence metadata.

**Do not send:** encoder deltas, encoder-button/mute events, saved volume, or a separate `MUTED` field in V1.

**Pack -> attenuator:** music-playing state, pack state, simulated heat/power, display brightness if pack-wide brightness control is retained, accepted vent-state progression, and other high-level pack status needed for local lighting/effects. The pack does not own or echo master volume for normal operation.

Absolute volume updates are intentionally idempotent: receiving `VOLUME=54` twice has the same effect as receiving it once. One-shot theme/vent actions still need retry-safe event handling so an I2C retry cannot trigger them twice. Snapshots on startup/reconnect must not replay historical switch edges.

## Acceptance scenarios for Codex

1. Turn the knob from 50 to 54: attenuator updates its bargraph locally and sends absolute `VOLUME=54`; no encoder delta is required by the pack.
2. Press and hold the encoder button: local mute toggles once; attenuator preserves saved volume, renders the mute marker, and sends `VOLUME=0` once/statefully as needed. No separate mute message is sent.
3. Rotate while locally muted: saved volume/marker move, effective pack volume remains 0; unmute sends the latest saved volume.
4. Turn volume normally to 0: pack receives `VOLUME=0` and treats it identically to silence from local mute.
5. Disconnect/reconnect after local adjustments: attenuator sends the current absolute effective volume; no historical deltas or mute presses are replayed.
6. Theme OFF -> ON starts once; staying ON does not replay after track end; ON -> OFF requests stop; another OFF -> ON requests a new play.
7. Vent OFF -> ON requests one vent; remaining ON causes no repeats; OFF rearms but does not abort active vent.
8. Rejected/deferred vent does not falsely show VENTING; accepted vent transitions local effects and eventually restores the attenuator-owned volume/mute display.
9. Duplicate absolute volume delivery is harmless; duplicate one-shot theme/vent events are suppressed by the event protocol.

## Not decided yet

Final encoder step scale/acceleration, volume-to-audio gain curve, persistence of saved volume across power cycles, music interruption policy, theme sound hardware, final I2C packet/address/retry encoding, and future fog actuation. Keep these configurable.

## Standalone simulation exception

The standalone sketch accepts debounced local toggle edges as simulated accepted state: theme drives a synthetic equalizer and vent runs the local bargraph animation. This is explicitly a test mode, with no audio, fog, or pack communication. Startup edge suppression and OFF-to-ON vent rearming follow the rules above.

Pin assignment decision (2026-10-01): toggle 1/theme uses Nano D5; toggle 2/vent uses Nano D6. Both use `INPUT_PULLUP`, with ON closing to shared GND. Physical wiring verification remains pending; see README and PINOUTS.
