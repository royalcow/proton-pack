# Attenuator bargraph hardware proof of concept

EXPERIMENTAL: standalone Arduino Nano ATmega328P / 5 V test for the yellow
BL28Z-3005SA04Y and Adafruit HT16K33 breakout #1427. Physical wiring and segment
order are **not yet hardware verified**. No pack-controller code is involved.
The requested `firmware/attenuator/README.md` and `CONTROLS_SPEC.md` were absent
from this checkout when this POC was created; scope follows the supplied test
requirements and PROJECT.md.

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
following the printed header labels. This is the user-supplied GPStar wiring
reference; it does not establish physical segment order.

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

Open `Attenuator_Bargraph.ino` in Arduino IDE, select Arduino Nano,
ATmega328P, and your actual serial port. Or from the repository root:

```sh
arduino-cli core install arduino:avr
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 firmware/POC/Attenuator_Bargraph
arduino-cli board list
arduino-cli upload --fqbn arduino:avr:nano:cpu=atmega328 --port /dev/cu.YOUR_PORT firmware/POC/Attenuator_Bargraph
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
a recovered display off; `demo` restarts animation. Change HT_ADDRESS if the
scanner finds the breakout at another address, then rebuild.

Default startup: all 28 wired positions on for 250 ms, off for 250 ms, then
repeating fill/drain. This is a visual self-test, not an automatic hardware
pass. Set `DISCOVERY_AT_BOOT = true` for an initial bring-up build that instead
steps through raw positions with manual confirmation. Either way, `discover` is always available.

| Command | Behavior |
|---|---|
| `scan` | Scan bus and report/reinitialize configured device if needed |
| `discover` | Light the first raw position of 4 COM x 7 ROW positions; print index, row, bit and RAM byte; hold until Return |
| `test` | One walk of logical segments 0–27 using lookup table; prints positions; ends off |
| `all` / `off` | Stop animation, set all 28 mapped segments on/off |
| `fill N` | Stop animation, fill N logical segments (0–28) |
| `chase` | Repeating single-segment chase |
| `bounce` | Repeating single segment 0 to 27 and back, without duplicated endpoints |
| `brightness N` | Global HT brightness 0–15; 0 is minimum brightness, not off |
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
7. Save all 28 final row/bit pairs, orientation, board/wiring notes, date,
   observer, and results. Only then set SEGMENT_MAP_VERIFIED to true.

A successful compile does not validate wiring, optical order, brightness, or
hardware operation. The sketch has no sensor feedback to establish these.

## Software validation (2026-09-28)

Before the manual discovery change, Arduino CLI 1.5.1, Arduino AVR Boards
1.8.8, bundled Wire:
`arduino:avr:nano:cpu=atmega328` compilation passed. Flash: 7,886 / 30,720
bytes; static RAM: 630 / 2,048 bytes. No upload or physical test performed.

A host test with mocked Wire/Serial passed raw RAM serialization, all 28
single-position discovery frames, command bounds, static inversion, animation
endpoints, millis wraparound, line overflow handling, and I2C failure/recovery.
Run from the repository root (requires a C++ compiler):

```sh
c++ -std=c++11 -Wall -Wextra -Ifirmware/POC/Attenuator_Bargraph/tests firmware/POC/Attenuator_Bargraph/tests/test.cpp -o /tmp/attenuator-bargraph-test-run
/tmp/attenuator-bargraph-test-run
```

These mocks check program behavior, not electrical timing or physical mapping.
