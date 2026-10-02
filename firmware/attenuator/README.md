# Standalone attenuator volume prototype

Arduino Nano ATmega328P / 5 V, yellow BL28Z-3005SA04Y, and Adafruit
HT16K33 breakout #1427. This sketch extends the verified bargraph mapping;
`firmware/POC/Attenuator_Bargraph/` remains the original diagnostic POC.
There is no pack I²C protocol implementation or main pack firmware change yet. V1 architecture now treats the attenuator Nano as the authoritative source for user-selected master volume; future pack communication will send absolute effective volume only.

## Volume, mute, and vent

Startup briefly lights all segments, clears them, then displays simulated
volume 50. `volume N` accepts integers 0–100. The bar uses all 28 positions,
rounding to the nearest segment: `(volume * 28 + 50) / 100`.
Thus 0 is empty, 50 is 14 segments, and 100 is all 28. Very low values can
round to zero. This is a local simulation; it does not change actual audio.

`mute` toggles simulated confirmed mute; `unmute` explicitly clears it.
The saved volume never changes as a side effect of mute. On mute, the filled
bar drains from high to low beneath two fixed adjacent saved-volume markers over
250 ms. The pair stays on the top two currently illuminated volume segments:
its first index is `max(0, litSegmentCount - 2)`. Volume 50 gives (12,13),
and 100 gives (26,27). When fewer than two segments are lit, use (0,1).
Volume adjustments while muted reposition the pair without unmuting.

Both markers breathe together using the HT16K33 global brightness, with a
2.5-second smoothstep cycle between 3 and configured brightness. Values 1–3
stay constant; **brightness 0 disables the display**, including diagnostics.
The configured brightness is always the ceiling. Default brightness is 2,
so use `brightness 12` to see breathing. The chip provides 16 discrete levels.
On unmute, the bar refills from low to high toward the fixed pair over 250 ms,
then shows exactly the saved volume at configured brightness.

`vent` (alias `purge`) runs the deterministic V2 pressure-dump sequence:

| Phase | Duration | Behavior |
|---|---:|---|
| `VENT_BUILDUP` | 180 ms | Preserve the current image, then fill in four steps at 36 ms intervals |
| `VENT_CHATTER` | 250 ms | Near-full dropouts from alternating ends; frame durations 45/35/60/45/65 ms |
| `VENT_DUMP` | 700 ms | Uneven high-end collapse, with two brief kickbacks |
| `VENT_RESIDUAL` | 380 ms | Three low-end pulses over 300 ms, followed by an 80 ms dark gap |
| `VENT_COMPLETE` | At 1510 ms | Restore latest volume/mute state |

Purge remaining-segment counts are 28, 25, 23, **25**, 19, 18, 14, **15**,
10, 7, 3, 0. Frame durations are 60/45/65/40/55/75/45/80/60/70/55/50 ms.
The residual pattern uses segment 0, then pair 2–3, then segment 1, separated
by dark frames. There is no runtime randomness. `VentAnimation.h` keeps the
named phases and tuning tables separate from driver and volume/mute ownership.

Volume/mute changes during vent update saved state without interrupting it.
Completion restores the current filled volume or breathing pair immediately,
even if mute changed in the last millisecond. If the simulated theme is still
playing and unmuted, the existing theme meter resumes. Repeated Serial vent
requests restart buildup from the currently rendered image. Diagnostic commands
can cancel vent; `show` returns to the normal renderer. Toggle rearming remains
unchanged. All phases respect configured brightness, including 0=off, and use
the verified mapping and normal `invert` behavior. Diagnostic `speed` does not
alter the V2 timings or mute timing.

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

### Rotary encoder wiring

The attenuator encoder is now physically soldered. Use Nano internal pull-ups; the encoder itself does not receive 5 V.

| Nano | Encoder |
|---|---|
| D2 | Rotary phase A / top-left outer contact — **blue** |
| D3 | Rotary phase B / top-right outer contact — **white** |
| D4 | Integrated pushbutton / bottom-left contact — **yellow** |
| GND | Top-center rotary common, locally tied to bottom-right switch return — **black** |

Rear-view orientation as installed: top-left blue, top-center GND, top-right white, bottom-left yellow, bottom-right black. The top-center rotary common is locally tied to the bottom-right pushbutton return, so the black lead is the single ground conductor leaving the encoder.

Configure D2, D3 and D4 as `INPUT_PULLUP`. Rotation and button closures are therefore active-low contact events. If clockwise is decoded backwards, reverse A/B in firmware or swap D2/D3; leave the shared ground unchanged.

### Encoder operation and checks

Hardware update (2026-10-01): the user confirmed encoder response after 45 minutes
idle with the updated code. Reset had restored operation with the earlier failure;
the exact cause remains unconfirmed.

Rotation changes simulated volume by four points per complete four-edge cycle,
clamped to 0–100. `ENCODER_VOLUME_STEP` configures the scale; there is no
acceleration. Each cycle now changes the bar by roughly one segment.
`ENCODER_DIRECTION = 1` treats phase sequence 11→01→00→10→11 as increasing;
set it to `-1` if clockwise decreases on the installed encoder. Encoders with
two detents per electrical cycle will require two clicks per four-point adjustment.
The user confirmed the installed encoder direction is correct on 2026-09-30
with `ENCODER_DIRECTION = 1`. Detent behavior remains to be checked.

D2/D3 CHANGE interrupts accumulate complete cycles while display updates run.
The decoder cancels backtracking bounce and rejects invalid two-bit jumps.
On Nano, both phase pins are sampled together from PORTD. The main loop also
reconciles missed edges under the interrupt lock. Unchanged volume frames avoid
I²C rewrites, and optional encoder Serial telemetry is dropped when the transmit
buffer is full rather than blocking input handling.
`EncoderInput.h` keeps contact decoding separate from display and simulation.
The D4 button uses a nonblocking 25 ms debounce: press toggles mute once,
holding does not repeat, and a button held at boot must be released first.

Rotation while muted moves the saved-volume pair without unmuting. Pressing
again uses the existing 250 ms unmute refill. Encoder input returns diagnostic
screens to volume; vent retains priority and restores the latest input state
when done. Input still updates simulated state if the display is unavailable;
Automatic retries once per second restore it after recovery; `scan` remains available. Serial volume/mute commands remain available.

After upload, turn slowly in both directions and check 0/100 limits. Turn quickly
while muted and during vent. Press, hold, release, and press again; verify one
toggle per press. Set `brightness 12` to see the breathing animation (default 2
holds steady as specified). Also reset with the button held and verify no toggle.
Direction is hardware-confirmed; contact behavior and fast rotation still require testing.

### Latching theme and vent toggles

Chosen pin assignments (2026-10-01), implemented in the standalone sketch:

| Toggle | Nano signal | Other contact |
|---|---|---|
| Theme | D5 (`THEME_SWITCH_PIN`) | GND |
| Vent | D6 (`VENT_SWITCH_PIN`) | GND |

Both use internal pull-ups; ON means the contact closes to GND. For SPDT
switches, use common and the selected ON contact, leaving the other unused.
Confirm these pins against the installed harness before upload.

Both switches debounce for 25 ms without blocking. Their startup positions are
sampled without generating events: if already ON, move OFF then ON to trigger.
Theme ON starts a synthetic, smoothly bouncing full-height bargraph meter;
OFF stops it. This simulates theme playback visually—there is no audio or
music analysis. A simulated track lasts 180 seconds (`THEME_DURATION_MS`),
then stops without replaying while the switch stays ON. Serial `theme` and
`stoptheme` also exercise this behavior.

Vent triggers once on OFF→ON and uses the existing vent animation. OFF rearms
without aborting vent; a fresh ON while vent is active is ignored. Priority is
vent, then muted markers/transitions, then theme meter, then normal volume.
Muting hides the theme meter while its simulated playback clock continues.
Unmuting completes the refill before resuming the meter. Volume remains saved
while the meter runs. Vent completion restores whichever state currently applies.
Diagnostics can temporarily replace the meter; `show` returns to normal rendering.

Physical acceptance: boot with toggles ON (no effects), cycle theme OFF/ON/OFF,
then trigger vent and leave it ON beyond completion (no repeat). Try vent while
theme plays, then mute and change volume during vent; verify correct restoration.
Toggle wiring and these new effects still await hardware confirmation.

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
errors and stop animations. The configured device is retried once per second;
`scan` can also recover after fixing the bus and leaves
a recovered display showing saved volume; `show` also restores this display. Change HT_ADDRESS if the
scanner finds the breakout at another address, then rebuild.

Default startup: all 28 wired positions on for 250 ms, off for 250 ms, then
saved volume (50 at boot). This is a visual self-test, not an automatic hardware
pass. Set `DISCOVERY_AT_BOOT = true` for an initial bring-up build that instead
steps through raw positions with manual confirmation. Either way, `discover` is always available.

| Command | Behavior |
|---|---|
| `theme` / `stoptheme` | Start/stop the synthetic theme equalizer (no audio) |
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
| `diag` | Report loop phase, current/last stall, stall count, encoder interrupt activity, and sticky Wire timeout flag |
| `status` | Report uptime, display readiness, volume/mute, raw encoder/button pins, and display failure count |
| `help` | Print commands |

Default brightness is 2, frame interval 100 ms. Edit constants or use commands.
Settings are RAM-only and reset on restart. Animations use elapsed `millis()`
subtraction (including wraparound); no delay-based animation or dynamic String.
Wire transactions and the on-demand bus scan are synchronous; the AVR Wire
25 ms timeout with peripheral reset is explicitly enabled. This sketch requires
the Wire timeout API (provided by AVR Boards 1.8.8); it must not be guarded by
`WIRE_HAS_TIMEOUT`, which that core does not define.
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
14,508 bytes flash and 968 bytes static RAM. No upload performed for this extension.

The original POC mapping and diagnostic commands were tested on hardware by the
user; see VERIFIED_MAPPING.md. New V2 vent behavior still needs a
hardware run. Host tests cover all 101 volume values and mapped RAM outputs,
quadrature decoding, encoder bounce/clamping, button debounce/hold/boot behavior,
long idle and timer wraparound, full Serial buffers, automatic display recovery,
V2 phase boundaries, kickbacks, restart, all brightness caps, final-millisecond
state restoration, and inverted output mapping,
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

### V1 volume ownership

The attenuator Nano is authoritative for the user-selected master-volume value (0–100). Encoder rotation updates that value locally and the BL28Z responds immediately. The pack/audio subsystem should receive only the resulting **absolute effective volume**.

The encoder pushbutton remains a local mute convenience so the attenuator can preserve a saved volume and render the breathing marker. Pack-facing mute is not a separate protocol state: while locally muted, send effective volume 0. Encoder changes while muted update the saved value/marker but keep effective volume at 0; unmute sends the latest saved absolute volume.

This makes volume updates idempotent and removes any need to replay encoder deltas or mute events after reconnect. For V1, no other device should write master volume without a new synchronization/ownership rule.

## Full attenuator integration (planned)

This standalone sketch implements the bargraph behavior using simulated local
state controlled by the physical encoder/button and Serial commands. The encoder
is wired and integrated (D2/D3 rotation, D4 pushbutton, shared GND). The two toggles now simulate local theme/vent requests. NeoPixels,
authoritative pack state, and pack communication remain planned. Integration references:

- [CONTROLS_SPEC.md](CONTROLS_SPEC.md) — encoder, mute, theme and vent controls.
- [BARGRAPH_SPEC.md](BARGRAPH_SPEC.md) — V1 bargraph behavior implemented here; hardware acceptance pending.
- [LIGHTING_SPEC.md](LIGHTING_SPEC.md) — connection lamp, radiation lens and lower dome.

### Planned controller-board connectors

| Ref | Connector | Pin order |
|---|---|---|
| J1 | 2-pin bench/test power | +5V, GND |
| J2 | 4-pin pack loom | GND, +5V, SDA, SCL |
| J3 | 4-pin local bargraph controller | GND, +5V, SDA_LOCAL, SCL_LOCAL |
| J4 | 4-pin encoder/button | GND, D2/A, D3/B, D4/SW |
| J5 | 3-pin NeoPixels | GND, +5V, DATA |
| J6 | 3-pin theme/vent switches | GND, D5/THEME, D6/VENT |

J2 is the normal operating power source. J1 is retained only for convenient bench/service power and connects to the same board rails.

## Proposed integration

- Pack-facing link: Nano as an I2C peripheral on the planned shared pack bus; ESP32 is the master. For volume, the Nano sends absolute effective volume 0–100; it does not send encoder deltas or a separate mute flag.
- The Nano must control its own outputs, not rely on the ESP32 to stream LED frames or drive bargraph segments.
- Local HT16K33 driver beside the BL28Z bargraph; a **separate local software I2C bus** is proposed if the Nano hardware I2C interface is used in peripheral mode. Verify library compatibility and timing experimentally.
- Physical packaging: Nano and power/distribution on the removable base plate; bargraph/driver near the shell window; detachable internal harnesses.
- Current HasLab wand remains supported. The eventual printed wand may reuse the same *pattern*, subject to bus and timing validation.
- The exterior loom supplies **+5V, GND, SDA and SCL** to the attenuator; the existing 5-pin GX12 can retain one spare conductor. The board-facing pack JST is 4-pin in the order GND, +5V, SDA, SCL.
- A separate 2-pin JST remains on the attenuator board as a **bench/test power input** tied to the same +5V/GND rails. Do not power the attenuator from the pack connector and bench connector at the same time unless an isolation/OR-ing scheme is intentionally added.
- No pack-bus level shifter is fitted on the attenuator board. With the current 5 V pack Nano, SDA/SCL connect directly. When the pack controller becomes a 3.3 V ESP32, bidirectional level translation for **both SDA and SCL** belongs on the main-controller side so the external accessory bus remains 5 V compatible.

See the top-level [PROJECT.md](../../PROJECT.md) and [PINOUTS.md](../../PINOUTS.md) for the current-versus-planned distinction and verified wiring. Do not reuse the current main pack Nano pin map as the attenuator Nano pin map.

## Diagnosing idle failures

The Nano built-in LED toggles every 500 ms from the main loop, even when the
volume bar is static. Serial remains silent at idle. Send `status` at 115200 baud:
`up` is uptime in seconds, `ready` is display availability, `vol`/`mute` are saved
state, `AB` is the raw 2-bit encoder state, `SW` is the raw button (0 pressed),
and `err` counts checked display-operation failures. Status skips printing if
the transmit buffer is full. It performs no I²C operations.

If the problem recurs, record whether the built-in LED still blinks and whether
`status` responds before resetting. A blinking LED with no Serial response
suggests investigating the serial connection; a stopped LED suggests a loop
stall or reset/power issue. Neither observation alone establishes the cause.
The 45-minute successful test was followed by another reported idle failure;
long-duration hardware validation must be repeated with the explicit timeout fix.

### Freeze localization build

The user confirmed that the heartbeat and Serial both stopped even after uploading
the explicit Wire-timeout build. The cause is still unresolved. This version
adds a Timer1 diagnostic interrupt at 100 Hz, independent of loop and millis.
After two seconds without another loop iteration, it emits repeating groups on
the built-in LED (100 ms on/off, with a pause between groups):

- 1 flash: input handling, including any display update triggered by input.
- 2 flashes: display recovery.
- 3 flashes: Serial command handling, which may itself invoke display operations.
- 4 flashes: display animation/rendering.
- 5 flashes: between tracked operations.
- 6 flashes: toggle handling, including display changes from a switch event.

Normal operation keeps the existing 500 ms heartbeat. A solid on/off LED rather
than grouped flashes means this diagnostic interrupt is not progressing either;
interrupt blocking/starvation or a board/power fault remains possible. A phase
code identifies a caller, not proof of the underlying cause. Record the pattern
before reset. No automatic reset is enabled.

`diag` reports the current phase, current and last detected stalls, a saturating
stall count, encoder interrupt count modulo 256 (`irq8`), and the sticky Wire
timeout flag. Counters are RAM-only and reset on reboot. Timer1 is reserved by
this standalone diagnostic build: do not combine it with Servo or Timer1 PWM on
D9/D10. The original POC and main pack firmware are unaffected.
