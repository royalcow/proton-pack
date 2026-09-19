# SpiritMinimal: Activate + Fire, sound-ready copy

This sketch adds Adafruit Audio FX Sound Board support using the same serial-control approach and track naming as the original Neutrino Wand sketch.

| Input state | Result |
| --- | --- |
| Activate switched off | The power cell counts backward from its current displayed level while the cyclotron fades out; the holding pattern then starts with one slowly pulsing blue power-cell light and alternating yellow cyclotron pairs |
| Activate on | Original-style power-cell build while the red cyclotron breathing pulse accelerates, then blue power-cell animation, red cyclotron rotation, and continuous `T03.WAV` audio |
| Activate on + Fire held | Rapid blue/white power-cell sweep and a faster orange-red cyclotron fade/rotation |

## Sound cues

| Event | Audio FX file | Included file |
| --- | --- | --- |
| Activate engaged | `T00.WAV` | `sounds/T00.wav` |
| Boot completes / active idle loop | `T03.WAV` | `sounds/T03.wav` |
| Fire pressed | `T01.WAV` | `sounds/T01.wav` |
| Fire released | `T02.WAV` | `sounds/T02.wav` |
| Activate disengaged | `T04.WAV` | `sounds/T04.wav` (original) |

Copy the five included `.wav` files onto the root of the Audio FX Sound Board's USB drive. The files must retain the `T00.WAV` through `T04.WAV` names so the sketch can trigger them.

The reversed shutdown recording is retained separately as `sounds/T04_reversed.wav`; the sketch continues to use the original `T04.wav` unless its track name is changed.

## Wiring

Keep the existing SpiritMinimal connections:

| Arduino Nano pin | Connection |
| --- | --- |
| D2 | Power-cell NeoPixel data input |
| D3 | Cyclotron NeoPixel data input |

Add these controls:

| Arduino Nano pin | Switch |
| --- | --- |
| D4 | Active-low Activate signal with an external pull-down |
| D5 | Active-low Fire signal with an external pull-down |
| D6 | One terminal of a momentary voltage-check button; other terminal to GND |
| A0 | Midpoint of the battery-voltage divider |
| A4 | INA219 SDA |
| A5 | INA219 SCL |

Add the INA219 before the wand power switch so it measures wand current:

| INA219 connection | Connect to |
| --- | --- |
| VCC | Nano always-on 5V |
| GND | Nano/wand common GND |
| SDA | Nano A4 |
| SCL | Nano A5 |
| VIN+ | 5V supply from the buck converter |
| VIN- | Positive feed going to the wand power switch |

The wand's negative supply remains connected directly to common GND. Do not route the wand current through the INA219 `VCC` pin; the measured current path is `VIN+` to `VIN-` through the onboard shunt.

Add an Adafruit Audio FX Sound Board:

| Arduino Nano pin | Sound Board connection |
| --- | --- |
| D9 | RST |
| D10 | RX |
| D11 | TX |
| D12 | ACT (required for continuous active-mode audio) |
| 5V | VIN / 5V, matching your board variant |
| GND | GND |
| GND | UG (required to select UART mode) |

Connect the Sound Board's line output to a suitable audio amplifier and speaker. Verify the Sound Board model's power-input requirements before connecting it; some variants expect USB 5 V while others have additional options.

**Tie the Audio FX board's UG pin directly to GND before powering or resetting it.** UG is pulled high by default, which selects manual GPIO-trigger mode and causes the board to ignore RX/TX commands. UART and manual trigger modes cannot operate simultaneously.

The sketch resets the Sound Board during Arduino startup. Track changes send a non-blocking Stop command, then use the same acknowledged Adafruit Soundboard Play command as the diagnostic sketch. Audio is serviced only after each lighting/state update. Leave the Sound Board's `RST` pin connected to D9.

`SFX_ACT_CONNECTED` and `REPLAY_IDLE_TRACK` are enabled. ACT must be wired to D12; when `T03.WAV` finishes and ACT returns HIGH, the sketch starts it again.

D4 and D5 remain plain `INPUT` pins with external pull-down resistors. The physical mapping remains D4 = Activate and D5 = Fire. INA219 current detection is authoritative for whether the wand is armed at all. Once current is above threshold, LOW/LOW is the armed holding pattern; the following HIGH/LOW transition begins `BOOTING` and its 2.6-second control-transition block. After the block, HIGH/LOW means active and HIGH/HIGH means firing; returning to HIGH/LOW ends firing. Dropping below the current threshold disarms the wand and requests shutdown from any state, regardless of D4/D5. Inputs are debounced for 3 ms, and the first boot-animation frame is rendered in the same loop that accepts Activate. D6 and D12 retain their internal pull-ups.

Because no sound is expected while the current-qualified wand is armed in `HOLDING`, activation sends the startup Play command directly. It does not send a redundant Stop command or wait through the normal 35 ms audio command gap. Other sound transitions retain the gap.

Current detection is configured for ON at 10 mA and OFF below 1 mA, with hysteresis. Arming retains a conservative 50 ms qualification and normal operation samples every 25 ms. While firing, current is sampled every 10 ms and a reading below the OFF threshold is accepted immediately, so cutting wand power normally begins shutdown within about 10 ms. Other disarming paths retain 25 ms qualification. The reverse count also removes its first power-cell light immediately instead of redrawing the unchanged firing frame or waiting for its 55 ms animation timer. These thresholds separate the observed wand-off current below 1 mA from wand-on current above 20 mA. `DEBUG_WAND_CURRENT` is disabled; enable it to print the measured current every 500 ms at 115200 baud. If the INA219 is not detected at its default `0x40` address, the sketch deliberately holds the wand state off.

When `ACTIVE` first sees the `HIGH/HIGH` Fire pattern, it forces fresh INA219 readings and requires the command to remain valid with wand current present for 10 ms before entering `FIRING`. This distinguishes a genuine Fire command from the brief `HIGH/HIGH` control transient that can occur as wand power is switched off. If current falls below the OFF threshold, the pack goes directly to shutdown and never starts the firing state or blast track.

`DEBUG_CONTROL_INPUTS` is currently enabled. Open the Arduino Serial Monitor at 115200 baud to see every raw and debounced Activate/Fire change together with its assigned physical pin. State numbers are `0=HOLDING`, `1=BOOTING`, `2=ACTIVE`, `3=FIRING`, and `4=SHUTTING_DOWN`. Disable the constant after the wiring sequence has been confirmed.

## Voltage-measurement stub

The voltage-check button on D6 takes 16 averaged samples from A0 whenever it is pressed and prints the result to USB Serial at 115200 baud. `onVoltageMeasured()` is the stub callback for adding a display, bar graph, or spoken-voltage response later.

For the included 100 kΩ / 33 kΩ divider:

- Connect 100 kΩ from battery positive to A0.
- Connect 33 kΩ from A0 to GND.
- Ensure the battery, Nano, and Audio FX board share GND.
- Never connect an 11.1 V battery directly to A0.

Update `VOLTAGE_DIVIDER_R1`, `VOLTAGE_DIVIDER_R2`, `ADC_REFERENCE_VOLTS`, and `VOLTAGE_CALIBRATION` to match the installed hardware and a trusted multimeter reading.

The sketch is currently configured for two connected 7-pixel NeoPixel jewels (14 pixels total): jewel 1 is pixels 0–6 and jewel 2 is 7–13. After adding the other two jewels, change `CYCLOTRON_JEWEL_COUNT` to `4`; jewels 3 and 4 will then be pixels 14–20 and 21–27.

## Upload

Open `SpiritMinimal_Activate_Fire_Sound.ino` in Arduino IDE, select your Nano board and port, then upload. Install the Adafruit NeoPixel, Adafruit Soundboard, and Adafruit INA219 libraries if they are not already available.

Fire is accepted only after the boot sequence reaches `ACTIVE`.

The sketch treats NeoPixel index `0` as the bottom power-cell light. If that is the top of your installed strip, change `BOTTOM_POWER_CELL` near the top of the sketch to `14`.
