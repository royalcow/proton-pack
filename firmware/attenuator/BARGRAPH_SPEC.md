# BL28Z bargraph behavior — V1

**Status:** Agreed display behavior. Mute behavior is implemented in the standalone prototype; V2 vent animation below is implemented in the standalone prototype and needs hardware testing.
**Hardware:** yellow 28-segment BL28Z driven by HT16K33, controlled by attenuator Nano.

## Behavior

- Normal operation displays the attenuator-owned selected master volume (logical 0–100) as a filled bar.
- On local mute, drain the filled bar from high to low beneath **two adjacent lit segments** at the saved-volume position. Local mute must not change the stored selected volume; pack-facing effective volume becomes 0.
- Both segments breathe **in sync** over an approximately 2.5-second cycle. HT16K33 brightness varies smoothly between **3 and the configured display brightness**, inclusive.
- The configured brightness is a hard ceiling. If configured brightness is 1–3, hold the marker pair at that value instead of exceeding the cap. If configured brightness is 0, honor off.
- Use the verified physical-to-logical segment mapping from the POC; logical indices are 0–27. Anchor the pair to the top two segments of the normal filled-volume bar (`max(0, litSegmentCount - 2)` through the next index), so muting does not move the indication upward. When fewer than two segments are lit, use (0,1). Clamp the two-segment window at both ends: saved volume 0 maps to pair (0,1); 100 maps to (26,27).
- Adjustments during local mute reposition the pair to the saved-volume setting without unmuting. These adjustments do not make the pack audible; effective volume remains 0 until local unmute.
- On local unmute, refill from low to high beneath the fixed pair to the saved selected volume over approximately 250 ms, restore configured normal brightness, and send that absolute volume to the pack.
- A higher-priority temporary display sequence may override the marker. When it finishes, return to the breathing pair if still muted; otherwise show confirmed volume.


## Vent / purge animation — V2

The existing straight full-to-empty drain is deprecated for the attenuator experience. Replace it with a staged **pressure-dump** animation that feels irregular and mechanical rather than like a progress bar.

### Phase 1 — pressure buildup

- Duration target: approximately **180 ms**.
- Start from whatever bargraph state is currently visible.
- Rapidly fill toward all 28 segments in roughly 4–5 nonblocking steps.
- End at or very near a full bar before release begins.

### Phase 2 — overpressure chatter

- Duration target: approximately **250 ms**.
- Hold near full while producing short irregular dropouts and recoveries.
- Use deliberately uneven frame timing, approximately **35–65 ms** per frame.
- Prefer a fixed, hand-tuned frame sequence over runtime randomness so the effect is repeatable and visually intentional.
- Example visual vocabulary: full bar, lose 2–4 segments from one end, recover to full, lose a small group from the opposite end, recover again.

### Phase 3 — main purge

- Duration target: approximately **700 ms**.
- Collapse the bar from the high end in **uneven chunks**, not one segment per frame.
- Suggested remaining-segment progression as a starting point: **28, 25, 23, 19, 18, 14, 10, 7, 3, 0**.
- Include one or two brief pressure kickbacks where the lit count increases by 1–2 segments before continuing downward.
- Timing should remain irregular enough to read as a pressure release rather than a timer.

### Phase 4 — residual pressure

- Duration target: approximately **300 ms** plus a short dark gap.
- After the main bar reaches empty, emit 2–3 short isolated low-end pulses using one or two adjacent segments.
- Residual pulses should be separated spatially and/or temporally, then end with approximately **80 ms dark**.
- After the dark gap, restore the correct current steady state:
  - selected filled volume if locally unmuted;
  - the two-segment breathing saved-volume marker if locally muted.

### Vent animation rules

- Overall target duration: approximately **1.4–1.6 seconds**. Exact phase timings are tuning values, not protocol timing guarantees.
- Keep the animation fully nonblocking with `millis()`; no `delay()` or waiting loops.
- Preserve attenuator-owned selected volume and local mute state underneath the temporary effect. Changes received during vent update the saved state but do not have to interrupt the visual sequence.
- A repeated vent request may restart the visual sequence from buildup.
- Diagnostic commands may cancel the vent effect as they do today.
- The main purge should use the configured display brightness ceiling. Slight brightness flutter during chatter is allowed, but never exceed the configured cap and honor brightness 0 as off.
- Use the verified logical segment mapping and keep `invert` behavior consistent with the rest of the display API.

### Vent implementation structure

Prefer explicit named phases rather than an opaque frame timer, for example:

```cpp
VENT_BUILDUP
VENT_CHATTER
VENT_DUMP
VENT_RESIDUAL
VENT_COMPLETE
```

Keep frame/timing tables separate from state ownership so the animation can be tuned without changing the bargraph driver or confirmed volume/mute logic.

## Implementation notes

Use nonblocking millis()-based animation. Preserve the attenuator-owned selected volume and local mute state separately from the rendered LED pattern. The pack does not require a separate mute field; effective volume 0 is the pack-facing result of local mute. HT16K33 brightness is global to this one display, so both lit segments breathe together.

## Acceptance checks

1. At volume 0 and 100, both markers remain in range and adjacent.
2. Breathing reaches brightness 3 and the configured cap; never exceeds the cap.
3. At brightness 0–3, the brightness cap is respected.
4. Volume changes while muted reposition markers without modifying mute state.
5. Unmute restores the saved volume. A temporary override returns to the correct current state.
6. Vent starts from empty, partial volume, full volume, and muted marker states without corrupting saved state.
7. Vent visibly passes through buildup, chatter, uneven purge, residual pulses, and dark gap; it must not look like a uniform one-segment countdown.
8. Volume/mute changes during vent are reflected when the animation completes.
9. Brightness 0–15 is respected throughout vent, including chatter; no phase exceeds the configured cap.
