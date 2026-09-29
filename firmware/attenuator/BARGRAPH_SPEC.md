# BL28Z mute indicator — V1

**Status:** Agreed display behavior, pending firmware implementation and hardware testing.
**Hardware:** yellow 28-segment BL28Z driven by HT16K33, controlled by attenuator Nano.

## Behavior

- Normal operation displays confirmed master volume (logical 0–100) as a filled bar.
- On confirmed mute, drain the filled bar into **two adjacent lit segments** at the saved-volume position. Mute must not change the stored volume.
- Both segments breathe **in sync** over an approximately 2.5-second cycle. HT16K33 brightness varies smoothly between **3 and the configured display brightness**, inclusive.
- The configured brightness is a hard ceiling. If configured brightness is 1–3, hold the marker pair at that value instead of exceeding the cap. If configured brightness is 0, honor off.
- Use the verified physical-to-logical segment mapping from the POC; logical indices are 0–27. Clamp the two-segment window at both ends: saved volume 0 maps to pair (0,1); 100 maps to (26,27).
- Adjustments during mute reposition the pair to the **confirmed** saved-volume setting without unmuting.
- On unmute, refill from the pair to confirmed master volume over approximately 250 ms and restore configured normal brightness.
- A higher-priority temporary display sequence may override the marker. When it finishes, return to the breathing pair if still muted; otherwise show confirmed volume.

## Implementation notes

Use nonblocking millis()-based animation. Preserve the confirmed volume and mute state separately from the rendered LED pattern. HT16K33 brightness is global to this one display, so both lit segments breathe together.

## Acceptance checks

1. At volume 0 and 100, both markers remain in range and adjacent.
2. Breathing reaches brightness 3 and the configured cap; never exceeds the cap.
3. At brightness 0–3, the brightness cap is respected.
4. Volume changes while muted reposition markers without modifying mute state.
5. Unmute restores the saved volume. A temporary override returns to the correct current state.
