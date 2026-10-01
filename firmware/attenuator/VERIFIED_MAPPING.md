# Physical mapping verification record

Copied from the original POC; results below apply to its mapping and diagnostics.
New volume, V1 breathing mute/transitions, and vent restoration behavior: hardware verification pending.

Status: **User confirmed increasing physical order and a correct sequential `test`; remaining hardware checks pending.**

## Initial assumed mapping (reference only)

Logical 0–6 -> (COM0, ROW0–6); 7–13 -> (COM1, ROW0–6);
14–20 -> (COM2, ROW0–6); 21–27 -> (COM3, ROW0–6).
This is enumeration order, not a verified BL28Z physical order.

## User-reported lookup table

User reports COM cycling 0–3 before advancing ROW, implemented in SegmentMap.h.
Orientation and the checks marked pending below remain to be recorded.
Use the physical number as the SEGMENT_MAP index. Row is buffer COM index;
bit is the electrical ROW output number.

| Physical segment / logical index | Observed row (COM) | Observed bit (ROW) |
|---:|---|---|
| 0 | 0 | 0 |
| 1 | 1 | 0 |
| 2 | 2 | 0 |
| 3 | 3 | 0 |
| 4 | 0 | 1 |
| 5 | 1 | 1 |
| 6 | 2 | 1 |
| 7 | 3 | 1 |
| 8 | 0 | 2 |
| 9 | 1 | 2 |
| 10 | 2 | 2 |
| 11 | 3 | 2 |
| 12 | 0 | 3 |
| 13 | 1 | 3 |
| 14 | 2 | 3 |
| 15 | 3 | 3 |
| 16 | 0 | 4 |
| 17 | 1 | 4 |
| 18 | 2 | 4 |
| 19 | 3 | 4 |
| 20 | 0 | 5 |
| 21 | 1 | 5 |
| 22 | 2 | 5 |
| 23 | 3 | 5 |
| 24 | 0 | 6 |
| 25 | 1 | 6 |
| 26 | 2 | 6 |
| 27 | 3 | 6 |

## Encoder verification

- 2026-10-01: user reported that reset restored input after the earlier idle failure. With the updated input/display recovery code, the encoder remained responsive after 45 minutes idle. This confirms the observed idle test passed; the original failure cause is not isolated.

- 2026-09-30: user confirmed correct rotation direction with `ENCODER_DIRECTION = 1`.
- Detent scale, debounce/hold behavior, and fast rotation: not yet individually confirmed.

## Hardware record

- Date / observer: pending
- Display marking and pin-1 orientation/photo: pending
- Physical segment 0 end: pending
- HT board revision/photo: pending
- Printed A0 continuity destination: pending
- Printed A2 continuity destination: pending
- Wiring table: user believes the working build matches README; exact printed A0/A2 correspondence is not recorded
- Corrected wiring or software-only mapping: pending
- Scanned address: pending
- Nano / core / sketch revision: pending
- Discovery with Return to advance: passed (user reported correct operation)
- Explicit count of 28 distinct single-lit discovery positions: not individually recorded
- Sequential `test`: passed (user reported correct operation with the updated mapping)
- Increasing physical sequence: confirmed by user
- `all`, `off`, and `fill`: passed (user reported correct operation)
- Specific fill boundary values 0/1/14/28: not individually recorded
- `invert`: passed (user reported correct operation)
- `chase` and `bounce`: passed, including interruption with `off`
- `demo` fill/drain and interruption with `off`: passed (user reported correct operation)
- Brightness adjustment: passed (user confirmed levels change)
- Specific brightness endpoints 0/15: not individually recorded
- Missing-device error and scan recovery: pending

Compilation results belong in README/build notes, not this hardware record.
