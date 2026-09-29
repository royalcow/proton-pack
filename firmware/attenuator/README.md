# Standalone attenuator volume prototype

Arduino Nano ATmega328P / 5 V, yellow BL28Z-3005SA04Y, and Adafruit
HT16K33 breakout #1427. This sketch extends the verified bargraph mapping;
`firmware/POC/Attenuator_Bargraph/` remains the original diagnostic POC.
There is no pack I²C protocol, audio control, or main pack firmware change.

## Volume, mute, and vent

Startup briefly lights all segments, clears them, then displays simulated
volume 50. `volume N` accepts integers 0–100. The bar uses all 28 positions,
rounding to the nearest segment: `(volume * 28 + 50) / 100`.
Thus 0 is empty, 50 is 14 segments, and 100 is all 28. Very low values can
round to zero. This is a local simulation; it does not change actual audio.

`mute` toggles simulated confirmed mute; `unmute` explicitly clears it.
The saved volume never changes as a side effect of mute. On mute, the filled
bar drains from the low end toward two adjacent saved-volume markers over
250 ms. The pair's first index is `min(26, round(volume * 27 / 100))`:
volume 0 gives (0,1), 50 gives (14,15), and 100 gives (26,27).
Volume adjustments while muted reposition the pair without unmuting.

Both markers breathe together using the HT16K33 global brightness, with a
2.5-second smoothstep cycle between 3 and configured brightness. Values 1–3
stay constant; **brightness 0 disables the display**, including diagnostics.
The configured brightness is always the ceiling. Default brightness is 2,
so use `brightness 12` to see breathing. The chip provides 16 discrete levels.
On unmute, the bar refills from the pair toward its low end over 250 ms,
then shows exactly the saved volume at configured brightness.

`vent` (alias `purge`) drains a full bar over 28 steps of 40 ms, holds empty
for 40 ms, and restores the current volume/mute state at 1160 ms. While muted,
it returns to the breathing pair. Volume/mute changes during vent update saved
state without interrupting vent. Repeating vent restarts it. Diagnostic
commands cancel vent; `show` returns immediately to the volume renderer.
Brightness/inversion changes preserve the active sequence. Diagnostic `speed`
does not alter the mute transition, breathing cycle, or vent duration.

All animation scheduling uses unsigned elapsed `millis()` comparisons, without
`delay()` or waiting loops. Serial stays responsive throughout effects.

## Wiring

Disconnect power before changing wiring. Use the display's package drawing to
identify physical pin 1 and viewing direction; do not infer pin numbering from
the way it happens to sit on a breadboard.

| Nano | HT16K33 |
|---|---|
| A4 | SDA |
| A5 | SCL |
| 5V | VDD |
| GND | GND |

HT output names below mean **electrical anode ROWn / cathode COMn**, not blindly
following the printed header labels. The user believes the working build matches this wiring table. Display
sequence and diagnostic operation were confirmed on hardware. Exact printed
A0/A2 pad correspondence, continuity measurements, and physical segment-zero
orientation have not been recorded; the table names electrical signals.

| HT electrical output | BL28Z physical pin | Display designation |
|---|---:|---|
| A0 / ROW0 | 22 | C1 |
| A1 / ROW1 | 1 | C2 |
| A2 / ROW2 | 19 | C3 |
| A3 / ROW3 | 18 | C4 |
| A4 / ROW4 | 7 | C5 |
| A5 / ROW5 | 10 | C6 |
| A6 / ROW6 | 11 | C7 |
| C0 / COM0 | 21 | L1 |
| C1 / COM1 | 15 | L2 |
| C2 / COM2 | 13 | L3 |
| C3 / COM3 | 16 | L4 |

Nano A4/A5 and HT output A4/A5 are different connections. Unused HT outputs
remain unconnected. Keep this POC on a standalone hardware-I2C bus. It does
not implement a Nano slave, ESP32 link, software I2C, or other attenuator I/O.

### A0/A2 silkscreen correction

Adafruit's [support discussion of #1427](https://forums.adafruit.com/viewtopic.php?t=210123)
records the swapped A0/A2 output-header labels. On an affected board, the pad
printed **A2 is electrical ROW0**, and the pad printed **A0 is electrical ROW2**.
Thus use printed A2 -> BL pin 22 and printed A0 -> BL pin 19 *after confirming
your board*. Do not change the I2C address solder jumpers to correct LED routing.

With power removed, check continuity from each header pad to the chip's ROW0
and ROW2 pins using the package-specific pin diagram in the
[HT16K33 datasheet](https://cdn-shop.adafruit.com/datasheets/ht16K33v110.pdf).
Record the board revision and measured correspondence. The chip's alternate
pin names ROW0/A2 and ROW2/A0 include **address input functions**; this document
uses ROW numbers to avoid that ambiguity.

Then use `discover`: bit 0 always drives electrical ROW0; bit 2 drives ROW2.
Compare their four observed positions with the display pin diagram and your
continuity notes. If those groups are exchanged, correct the two wires while
unpowered, or map the actual observed row/bit pairs in SegmentMap.h. Do not
both swap the wires and apply a second software swap. `invert` only reverses
logical orientation; it cannot fix an A0/A2 group swap.

## Dependencies, compile, upload

Only the Arduino AVR Boards core and its bundled **Wire** library are required.
No Adafruit LED Backpack/GFX library is needed. This uses an equivalent small
raw-buffer implementation; the 24-bargraph helper is not used.

Open `attenuator.ino` in Arduino IDE, select Arduino Nano,
ATmega328P, and your actual serial port. Or from the repository root:

```sh
arduino-cli core install arduino:avr
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 firmware/attenuator
arduino-cli board list
arduino-cli upload --fqbn arduino:avr:nano:cpu=atmega328 --port /dev/cu.YOUR_PORT firmware/attenuator
arduino-cli monitor --port /dev/cu.YOUR_PORT --config baudrate=115200
```

For a Nano requiring the old bootloader, use `cpu=atmega328old` for both compile
and upload. Close other serial monitors before upload. On macOS the IDE may
bundle the CLI at `/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli`.

## Startup and commands

Serial: **115200 baud**, newline or carriage-return termination (CRLF works).
Commands are lowercase. Startup scans 0x08–0x77 and initializes the configured
address (default `HT_ADDRESS = 0x70`). An ACK verifies an address responds,
not the chip identity. Missing hardware or failed writes produce explicit
errors and stop animations. `scan` can recover after fixing the bus and leaves
a recovered display showing saved volume; `show` also restores this display. Change HT_ADDRESS if the
scanner finds the breakout at another address, then rebuild.

Default startup: all 28 wired positions on for 250 ms, off for 250 ms, then
saved volume (50 at boot). This is a visual self-test, not an automatic hardware
pass. Set `DISCOVERY_AT_BOOT = true` for an initial bring-up build that instead
steps through raw positions with manual confirmation. Either way, `discover` is always available.

| Command | Behavior |
|---|---|
| `volume N` | Set saved simulated volume 0–100; preserve mute; reposition muted markers; defer display during vent |
| `mute` / `unmute` | Toggle mute / explicitly unmute with V1 transitions |
| `vent` / `purge` | Temporary full-to-empty drain, then restore volume or muted markers |
| `show` | Cancel animation and display saved volume or muted markers |
| `scan` | Scan bus and report/reinitialize configured device if needed |
| `discover` | Light the first raw position of 4 COM x 7 ROW positions; print index, row, bit and RAM byte; hold until Return |
| `test` | One walk of logical segments 0–27 using lookup table; prints positions; ends off |
| `all` / `off` | Stop animation, set all 28 mapped segments on/off |
| `fill N` | Stop animation, fill N logical segments (0–28) |
| `chase` | Repeating single-segment chase |
| `bounce` | Repeating single segment 0 to 27 and back, without duplicated endpoints |
| `brightness N` | Global HT brightness 0–15; 0 disables display; muted breathing respects the cap |
| Return (empty line) | Advance discovery after recording the lit segment; after position 27, finish and turn off |
| `invert` | Toggle logical reversal, including current static image; discovery stays raw |
| `speed N` | Animation frame interval 20–5000 ms; discovery advances only with Return |
| `demo` | Restart repeating fill/drain |
| `help` | Print commands |

Default brightness is 2, frame interval 100 ms. Edit constants or use commands.
Settings are RAM-only and reset on restart. Animations use elapsed `millis()`
subtraction (including wraparound); no delay-based animation or dynamic String.
Wire transactions and the on-demand bus scan are synchronous; the AVR Wire
25 ms timeout bounds a stuck transaction when supported by the installed core.
The Serial parser rejects oversized lines, extra arguments, and invalid ranges.

## Driver and mapping

`Ht16k33.h` is independent of demo, Serial, segment order, and bus initialization.
It owns eight 16-bit words. Word `row` means **COM index** (the same convention
as Adafruit's raw display buffer); `bit` means electrical **ROW/anode index**.
This terminology differs from the datasheet's ROW output name.

RAM byte address = `2 * row + bit / 8`, byte bit = `bit % 8`. The driver sends
address pointer 0x00 followed by all 16 RAM bytes, low byte first. Initialization
uses oscillator on (0x21), display off (0x80), cleared RAM, brightness (0xE0|N),
and display on/no blink (0x81). See the datasheet's RAM structure and command
summary. Every transaction checks the Wire result.

`SegmentMap.h` contains the user-reported physical sequence: cycle COM0–3,
then advance ROW0–6. The user confirmed increasing order and sequential `test`. The map validator rejects
duplicates or positions outside COM0–3/ROW0–6. Discovery ignores this table and
logical inversion. `VERIFIED_MAPPING.md` separately holds the physical results;
it records passing checks and the remaining hardware verification items.

## Verify all 28 physical segments

1. Check display orientation/pin numbering and A0/A2 continuity as above.
   Open Serial, reset Nano, run `scan`; record actual address.
2. Define physical segment 0 at a clearly photographed/labeled end, then number
   toward 27. Run `off`, then `discover`.
3. For each printed row/bit, record the single physical segment illuminated in
   VERIFIED_MAPPING.md, then press Return to advance. Each position stays lit
   until you press Return; after the last position, Return turns the display off.
   Use `off` to stop early or `discover` to restart. The 28 steps must light 28 distinct segments exactly
   once. Repeat as needed. Missing, duplicate, or multiple lit segments require
   wiring/pin-orientation investigation before accepting the map.
4. Reorder the entries of SEGMENT_MAP so entry N is the observed row/bit for
   physical segment N. Rebuild/upload. Keep the original assumed mapping
   recorded separately in VERIFIED_MAPPING.md.
5. Run `test`: verify contiguous motion 0–27. Run `all` and check all 28,
   `off` and check darkness, then `fill 0`, `fill 1`, `fill 14`, `fill 28`.
   Verify `invert` fills from the opposite end and `invert` again restores it.
6. Verify `chase`, `bounce`, `demo`, and brightness 0 and 15. Confirm Serial
   `off` interrupts every animation. Power down, disconnect the breakout,
   restart, and verify an explicit missing-device error. Power down again
   before reconnecting; `scan` should then recover after power restoration.
7. The copied lookup table is marked sequence-verified based on user tests.
   Save orientation, wiring notes, and remaining check results separately; this
   flag does not assert that every electrical or recovery check has passed.

A successful compile does not validate wiring, optical order, brightness, or
hardware operation. The sketch has no sensor feedback to establish these.

## Validation

Arduino Nano ATmega328P compilation with Arduino AVR Boards 1.8.8 passed:
9,438 bytes flash and 677 bytes static RAM. No upload performed for this extension.

The original POC mapping and diagnostic commands were tested on hardware by the
user; see VERIFIED_MAPPING.md. New volume/mute/vent behavior still needs a
hardware run. Host tests cover all 101 volume values and mapped RAM outputs,
invalid commands, saved state, mute transitions, all brightness caps, effect
interruption and restoration, timer wraparound,
discovery CRLF handling, and driver failure/recovery.

```sh
c++ -std=c++11 -Wall -Wextra -Ifirmware/attenuator/tests firmware/attenuator/tests/test.cpp -o /tmp/attenuator-volume-test
/tmp/attenuator-volume-test
```

Hardware acceptance (new V1 behavior remains untested on hardware):

1. Send `brightness 12`, `volume 50`, then `mute`. Confirm a short drain to
   two adjacent markers which breathe together every 2.5 seconds.
2. While muted, send `volume 0`, then `volume 100`; confirm adjacent endpoint
   pairs, with no unmute. Try intermediate values.
3. Check `brightness 0` (off), 1, 2, 3 (steady), and 15 (breathing capped at 15).
4. Send `unmute`; confirm a roughly 250 ms refill and normal brightness.
5. Mute again, run `vent`, and change volume during vent. Confirm restoration
   to the new pair. Repeat with `unmute` during vent; confirm a volume bar.
6. Recheck `discover`, Return stepping, `test`, `invert`, `chase`, `bounce`,
   `demo`, and `off` using the unchanged physical lookup table.

`VolumeDisplay.h` owns confirmed volume/mute and renders logical bitmasks and
brightness using caller-supplied time. It has no Serial, Wire, or pack coupling;
a future integration can call its setters with confirmed master state.
`Ht16k33.h` remains the reusable raw driver; `setEnabled()` implements true off
at brightness zero. The sketch handles Serial, diagnostics, and temporary vent
priority. `SegmentMap.h` and its verified lookup table are unchanged.
Behavior reference: [BARGRAPH_SPEC.md](BARGRAPH_SPEC.md).

## Full attenuator integration (planned)

This standalone sketch implements the bargraph behavior using simulated Serial
state. Encoder, switches, NeoPixels, authoritative pack state, and pack communication
remain planned. Integration references:

- [CONTROLS_SPEC.md](CONTROLS_SPEC.md) — encoder, mute, theme and vent controls.
- [BARGRAPH_SPEC.md](BARGRAPH_SPEC.md) — V1 bargraph behavior implemented here; hardware acceptance pending.
- [LIGHTING_SPEC.md](LIGHTING_SPEC.md) — connection lamp, radiation lens and lower dome.

## Proposed integration

- Pack-facing link: Nano as an I2C peripheral on the planned shared pack bus; ESP32 is the master and polls inputs/sends state.
- The Nano must control its own outputs, not rely on the ESP32 to stream LED frames or drive bargraph segments.
- Local HT16K33 driver beside the BL28Z bargraph; a **separate local software I2C bus** is proposed if the Nano hardware I2C interface is used in peripheral mode. Verify library compatibility and timing experimentally.
- Physical packaging: Nano and power/distribution on the removable base plate; bargraph/driver near the shell window; detachable internal harnesses.
- Current HasLab wand remains supported. The eventual printed wand may reuse the same *pattern*, subject to bus and timing validation.
- The proposed 5-pin GX12 loom assigns 5V, GND, SDA, SCL and one spare. **Pin numbering, voltage interface, address and bus pull-ups are not finalized**; do not connect the 5 V Nano I2C interface directly to 3.3 V ESP32 lines without confirming level shifting.

See the top-level [PROJECT.md](../../PROJECT.md) and [PINOUTS.md](../../PINOUTS.md) for the current-versus-planned distinction and verified wiring. Do not reuse the current main pack Nano pin map as the attenuator Nano pin map.
