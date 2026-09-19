# Pack Controller Firmware Architecture

This document describes the behavior implemented by
`SpiritMinimal_Activate_Fire_Sound.ino`. It records firmware facts separately
from physical wiring that has not yet been verified on the pack.

## Scope and dependencies

The sketch targets an Arduino Nano and uses:

- Adafruit NeoPixel
- SoftwareSerial
- Adafruit Soundboard
- Wire
- Adafruit INA219

The main loop is cooperative: controls, voltage button, current sensing,
lighting, state transitions, diagnostics, and queued audio commands are
serviced in sequence. Animation timing uses `millis()` rather than deliberate
runtime `delay()` calls. Library calls during startup and Audio FX command
acknowledgement can still block briefly.

## Arduino pin assignments

| Nano pin | Direction / mode | Firmware use | Electrical assumption |
| --- | --- | --- | --- |
| D2 | Output | 16-pixel power-cell NeoPixel data | 5 V NeoPixel chain; 800 kHz GRB |
| D3 | Output | Cyclotron NeoPixel data | Four 7-pixel jewels in one 28-pixel chain in the current build configuration; 800 kHz GRB |
| D4 | Input | HasLab Activate control | External pull-down; interpreted together with D5 |
| D5 | Input | HasLab Fire control | External pull-down; interpreted together with D4 |
| D6 | `INPUT_PULLUP` | Voltage-check momentary button | Button closes to common ground |
| D9 | Output via library | Audio FX `RST` | Reset connected directly to sound board |
| D10 | Software serial TX | Nano TX to Audio FX RX | 9600 baud |
| D11 | Software serial RX | Nano RX from Audio FX TX | 9600 baud |
| D12 | `INPUT_PULLUP` | Audio FX `ACT` | LOW while a track is playing |
| D13 / `LED_BUILTIN` | Output | Heartbeat LED | Toggles every 500 ms |
| A0 | Analog input | Battery-divider midpoint | Default nominal 5 V ADC reference |
| A4 | I2C SDA | INA219 current sensor | Default INA219 address `0x40` |
| A5 | I2C SCL | INA219 current sensor | Common ground with Nano and wand |

USB serial runs at 115200 baud for diagnostics and voltage results. D4 and D5
are deliberately plain `INPUT` pins; the firmware does not enable internal
pull-ups or pull-downs on them.

## Inputs

### Wand controls

D4 and D5 are active-low at the pin-reading layer, but neither pin alone maps
directly to a pack state. With INA219 current confirming that the wand is
powered, the combined stable patterns are:

| D4 Activate | D5 Fire | Firmware meaning |
| --- | --- | --- |
| LOW | LOW | Ready/armed pattern in `HOLDING` |
| HIGH | LOW | Activate/active pattern |
| HIGH | HIGH | Fire pattern while `ACTIVE` |
| LOW | HIGH | Invalid/release pattern; can request shutdown after qualification |

Both controls use a 3 ms debounce filter. The ready pattern must be observed
before the following active pattern may start a boot. This prevents a wand
power transition by itself from counting as Activate.

The INA219 reading is authoritative for whether the wand is powered. If the
sensor is absent, initialization fails, or qualified current falls below the
off threshold, the controller will not accept wand commands and requests
shutdown from any non-holding state.

### Voltage button and battery divider

A press on D6 is recognized on its HIGH-to-LOW edge with 40 ms debounce. The
firmware averages 16 A0 samples and converts them using these compile-time
values:

- R1, battery positive to A0: 100 kΩ
- R2, A0 to ground: 33 kΩ
- ADC reference: 5.0 V
- calibration multiplier: 1.0

The result is only printed to USB serial. `onVoltageMeasured()` is an extension
stub; the reading does not change pack state, lighting, or audio and there is
no automatic low-battery protection.

## Outputs

- Power-cell NeoPixels on D2
- Cyclotron NeoPixels on D3
- Audio FX UART commands and reset on D9-D11
- Built-in heartbeat LED on D13
- USB serial diagnostics at 115200 baud

## NeoPixel circuits and animations

Both NeoPixel objects use GRB ordering at 800 kHz and global brightness 80.

### Power cell

The D2 chain is configured for 16 pixels. Only pixels 0-14 are animated;
pixel 15 is always forced off. `BOTTOM_POWER_CELL` is currently pixel 0.

- `HOLDING`: one bottom blue pixel breathes from brightness 20 to 150 and
  back, changing by 2 every 25 ms (about 3.25 seconds for a full cycle).
- `BOOTING`: a blue sweep repeatedly travels from the upper end toward the
  next cell to lock on. All 15 animated cells require 120 frames at 40 ms,
  approximately 4.8 seconds.
- `ACTIVE`: a blue fill advances one cell every 55 ms and wraps after 15
  cells (825 ms per cycle).
- `FIRING`: the fill advances every 28 ms (420 ms per cycle) while blue and
  blue/white coloration changes according to a separate 45 ms flash counter.
- `SHUTTING_DOWN`: lit cells are removed from the highest displayed index
  downward, one every 55 ms. The first removal is immediate; a full 15-cell
  shutdown takes about 770 ms after that first frame.

### Cyclotron

The D3 chain is compiled for four 7-pixel jewels (28 pixels), with each jewel
occupying a contiguous group. This conflicts with older documentation that
says two jewels are installed; firmware configuration alone does not prove
the physical jewel count, so the hardware must be checked.

- `HOLDING`: opposing yellow-orange jewel pairs alternate (0+2, then 1+3).
  Each pair fades up and down over a 750 ms step.
- `BOOTING`: every cyclotron pixel breathes red. The update interval
  accelerates from 14 ms toward 3 ms as power-cell pixels lock on.
- `ACTIVE`: one red jewel at a time fades up/down over a 770 ms step.
- `FIRING`: one orange-red jewel at a time fades up/down over a 105 ms step.
- `SHUTTING_DOWN`: the captured cyclotron frame is dimmed in proportion to
  the reverse power-cell count.

Fade frames are refreshed no more often than every 10 ms. NeoPixel `show()`
calls are synchronous, but there are no explicit animation delays.

## Wand state machine

| State | Entry / behavior | Exit conditions |
| --- | --- | --- |
| `HOLDING` (0) | Default state; holding lights run whether the wand is powered or not. Current-qualified LOW/LOW arms Activate. | Armed LOW/LOW followed by HIGH/LOW starts `BOOTING`. |
| `BOOTING` (1) | Plays startup track and runs the ~4.8 s boot lights. A 2.6 s transition block ignores the wand's known control transient. Fire cannot enter `FIRING` during boot. | Completed light sequence enters `ACTIVE`; after the transition block, a qualified invalid/release pattern or loss of wand current starts shutdown. |
| `ACTIVE` (2) | Runs normal lights and idle audio. | HIGH/HIGH plus current, stable for 10 ms, enters `FIRING`; qualified release or loss of current starts shutdown. |
| `FIRING` (3) | Plays blast track and runs firing lights. | HIGH/LOW plays fire-end track and returns to `ACTIVE`; qualified release or loss of current starts shutdown. |
| `SHUTTING_DOWN` (4) | Plays shutdown track, reverse-counts the power cell, and fades the captured cyclotron frame. Reactivation is ignored. | When the reverse count reaches zero, clears both chains and returns to `HOLDING`. |

Release through an unexpected control pattern is qualified for 50 ms. Loss of
current normally uses separate current qualification described below and can
request shutdown independently of D4/D5.

## Audio control

The sketch controls an Adafruit Audio FX Sound Board in UART mode. The board's
UG pin must be tied to ground; UART and manual trigger modes are mutually
exclusive. Audio FX output is expected to feed a separate amplifier and pack
speaker.

| Event | Track name sent | Repository asset |
| --- | --- | --- |
| Boot starts | `T00.WAV` | `sounds/T00.wav` |
| Fire starts | `T01.WAV` | `sounds/T01.wav` |
| Fire ends | `T02.WAV` | `sounds/T02.wav` |
| Boot completes / active idle | `T03.WAV` | `sounds/T03.wav` |
| Shutdown starts | `T04.WAV` | `sounds/T04.wav` |

The bundled files are mono PCM WAV assets. `T00` is 16-bit/44.1 kHz (6.53 s),
`T01` is 16-bit/32 kHz (22.04 s), `T02` is 16-bit/44.1 kHz (1.75 s), `T03`
is 16-bit/32 kHz (30.00 s), and the three `T04` variants are 8-bit/32 kHz
(3.10 s). `T04.wav` and `T04_original.wav` are byte-identical;
`T04_reversed.wav` is the retained alternate and is not referenced by the
sketch.

The newest queued cue replaces any pending cue. Normal transitions send the
single-byte Stop command (`q`), wait 35 ms, then call `playTrack()`. Play is
retried once after another 35 ms when it is not acknowledged. Boot omits Stop
because `HOLDING` is expected to be silent. SoftwareSerial has a 150 ms read
timeout.

During `ACTIVE`, D12 tracks Audio FX ACT. Once the idle track has been seen
playing and ACT later returns HIGH, the idle track is queued again without a
Stop command, providing continuous active-mode sound. Audio servicing occurs
after input, state, and lighting work on each loop pass.

## Current and power handling

The INA219 is expected before the wand power switch: VIN+ receives bucked 5 V
and VIN- feeds the positive side of the wand switch. Its VCC is powered by the
Nano's always-on 5 V; wand negative remains on common ground.

- Wand ON threshold: magnitude at least 10 mA
- Wand OFF threshold: magnitude at most 1 mA (implemented as ON only while
  strictly greater than 1 mA once powered)
- ON qualification: 50 ms
- Normal OFF qualification: 25 ms
- Normal sample interval: 25 ms
- Firing or candidate-Fire sample interval: 10 ms
- OFF acceptance while firing or validating Fire: immediate after a below-
  threshold reading

The large threshold separation supplies hysteresis. Signed readings are
converted to magnitude. The code comments record observed wand-off current
below 1 mA and wand-on current above 20 mA, but these observations should be
revalidated if power wiring or the wand changes.

The controller itself is assumed to remain powered while it observes and
switches wand state. The sketch does not control the 12 V battery rail, the
buck converter, charging, fusing, or a main power latch.

## Timing summary

| Function | Timing |
| --- | ---: |
| D4/D5 debounce | 3 ms |
| Activation transient block | 2600 ms |
| Fire qualification | 10 ms |
| Unexpected control release qualification | 50 ms |
| INA219 normal / firing sampling | 25 / 10 ms |
| INA219 ON / normal OFF qualification | 50 / 25 ms |
| Voltage button debounce | 40 ms |
| Holding power-cell fade update | 25 ms |
| Boot power-cell frame | 40 ms |
| Cyclotron fade frame | 10 ms |
| Active power-cell frame | 55 ms |
| Firing power-cell / flash frame | 28 / 45 ms |
| Shutdown frame | 55 ms |
| Built-in LED heartbeat toggle | 500 ms |
| Audio Stop-to-Play gap | 35 ms |
| Audio serial timeout | 150 ms |

## Global state

The sketch keeps all state in file-scope globals:

- Pack mode: `packState`
- Audio queue: pending track, command phase, retry count, and due time
- Wand controls: raw/debounced candidates, arming flag, activation block,
  Fire/release qualification flags, and debug snapshots
- Wand current: sensor availability, measured current, detected/candidate
  power states, and sample/qualification timestamps
- Animation: power-cell/cyclotron steps, boot sweep and brightness, firing
  flash step, holding fade, and shutdown snapshots
- Timing: one `previous...Millis` value per independently scheduled activity
- Voltage button edge/debounce state and idle-track ACT history

There is no persistence across reset and no dynamic allocation in application
code. On reset the state returns to `HOLDING`, the Audio FX board is reset, and
both NeoPixel chains are cleared.

## Hardware assumptions and open verification

Implemented assumptions:

- Arduino Nano uses nominal 5 V logic and default 5 V ADC reference.
- All modules share common ground.
- D4/D5 have working external pull-down resistors and are logic signals from
  the wand, not bare contacts.
- NeoPixels accept the Nano's data level and have adequate separate 5 V power.
- INA219 is present at its library-default `0x40` address.
- Audio FX UG is grounded, RST and ACT are wired, and its power-input variant
  is compatible with the supplied 5 V.
- Audio FX line output feeds an amplifier; the CH358D may fill this role, but
  that exact signal connection is not expressed in firmware.

Still requiring physical verification:

- Whether two or four cyclotron jewels are actually installed and connected.
- HasLab 4-pin JST pin order, wire colors, voltage levels, and whether every
  pin is used by the pack interface.
- Installed divider resistor values and calibrated ADC reference.
- INA219 placement, shunt orientation, thresholds, and measured currents.
- NeoPixel power injection, data resistor/capacitor, connector pin order, and
  current budget.
- Audio FX board model, power input, amplifier interconnect, and speaker path.

## Comparison with top-level documentation

- `PROJECT.md` previously mentioned only two 7-pixel Jewel circuits and did
  not mention the 16-pixel power-cell chain, INA219, voltage button/divider,
  Audio FX controller, or implemented state machine.
- `PINOUTS.md` previously omitted all Nano pin assignments and the INA219
  address. It also left HasLab control behavior unspecified.
- The firmware README described a two-jewel configuration and instructed a
  future change to four, while the sketch currently sets the jewel count to
  four. The README has been corrected to expose this discrepancy rather than
  imply that the source still uses two.
- Top-level references to a 12 V LiPo and buck converter are compatible with
  the sketch, but only the 5 V wand feed, common ground, and optional battery
  divider are visible to firmware.


## ESP32-S3 migration plan

Development hardware selected on 2026-09-19:

- Adafruit ESP32-S3 Reverse TFT Feather #5691 for prototyping/development.
- SN74AHCT125N powered from 5 V for 3.3 V -> 5 V NeoPixel data translation.
- Two independent level-shifted NeoPixel outputs: power cell and cyclotron.
- 0.1 uF ceramic decoupling at the AHCT125; series data resistors targeted around 330 ohms.
- Existing 5 V NeoPixel power distribution remains separate from data-level translation.

Migration should first reproduce the Nano's five-state behavior before adding new functionality. The S3 implementation should favor event-driven FreeRTOS tasks so critical input/state handling is isolated from lighting, audio, I2C, telemetry, Wi-Fi, and OTA work. Core affinity and task priorities should be chosen from measurement rather than assumed prematurely.

The #5691 integrated TFT is available for development diagnostics (state, wand inputs, lighting state, I2C status, battery telemetry, Wi-Fi/OTA). Final firmware must not require the TFT so a standard ESP32-S3 Feather can be substituted later.

Exact ESP32-S3 GPIO assignments remain open until the #5691 reserved/on-board pins are reviewed. Do not copy Nano pin numbers directly.
