#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "Ht16k33.h"
#include "SegmentMap.h"
#include "VolumeDisplay.h"
#include "VentAnimation.h"
#include "EncoderInput.h"
#include "ToggleInput.h"
#include "LoopDiagnostics.h"
#if defined(__AVR_ATmega328P__)
#include <avr/interrupt.h>
#include <util/atomic.h>
#endif

LoopDiagnostics loopDiagnostics;
volatile uint8_t encoderInterrupts = 0;

// Assigned standalone toggle pins; physical verification pending.
constexpr uint8_t THEME_SWITCH_PIN = 5;
constexpr uint8_t VENT_SWITCH_PIN = 6;
constexpr uint32_t THEME_DURATION_MS = 180000; // Simulated track, not audio timing.
ToggleInput themeSwitch, ventSwitch;
bool themePlaying = false;
uint32_t themeStarted = 0;

constexpr uint8_t ENCODER_A_PIN = 2;
constexpr uint8_t ENCODER_B_PIN = 3;
constexpr uint8_t ENCODER_BUTTON_PIN = 4;
constexpr int8_t ENCODER_DIRECTION = 1; // Set -1 if clockwise decreases volume.
constexpr uint8_t ENCODER_VOLUME_STEP = 4; // About one visible segment per cycle.
EncoderInput encoderInput;
volatile int16_t encoderDelta = 0;
uint32_t lastDisplayRetry = 0;
uint32_t displayFailures = 0;
uint32_t lastHeartbeat = 0;
bool heartbeatOn = false;

constexpr uint8_t HT_ADDRESS = 0x70;
constexpr uint8_t DEFAULT_BRIGHTNESS = 2;
constexpr uint16_t DEFAULT_STEP_MS = 100;
// Enable for first wiring bring-up: discovery replaces the startup self-test and volume display.
constexpr bool DISCOVERY_AT_BOOT = false;

enum Mode { IDLE, SELF_TEST, DEMO, TEST, DISCOVER, CHASE, BOUNCE, VOLUME, VENT };
Ht16k33 display;
VolumeDisplay volumeDisplay;
VentAnimation ventAnimation;
uint8_t appliedBrightness = 255;
Mode mode = IDLE;
bool ready = false;
bool reversed = false;
uint8_t brightness = DEFAULT_BRIGHTNESS;
uint16_t stepMs = DEFAULT_STEP_MS;
uint32_t lastStep = 0;
uint8_t frame = 0;
int8_t direction = 1;
// Logical image retained so invert also redraws a static fill immediately.
uint32_t logicalImage = 0;
char input[48];
uint8_t inputLength = 0;
bool inputOverflow = false;
bool previousWasCR = false;

void help() {
  Serial.println(F("theme | stoptheme (simulate theme play/stop)"));
  Serial.println(F("volume 0..100 | mute (toggle) | unmute | show | vent | purge"));
  Serial.println(F("scan | discover | test | all | off | fill 0..28"));
  Serial.println(F("Return on an empty line advances discovery"));
  Serial.println(F("chase | bounce | demo | brightness 0..15 | invert"));
  Serial.println(F("speed 20..5000 (animation ms) | status | diag | help"));
}

bool checked(bool success) {
  if (!success) {
    ++displayFailures;
    ready = false;
    mode = IDLE;
    Serial.println(F("ERROR: HT16K33 I2C failed; animation stopped. Check wiring; run scan."));
  }
  return success;
}

bool applyBrightness(uint8_t level) {
  if (appliedBrightness == level) return true;
  if (!checked(display.setEnabled(level != 0))) return false;
  if (!checked(display.setBrightness(level))) return false;
  appliedBrightness = level;
  return true;
}

void renderLogical() {
  display.clear();
  for (uint8_t i = 0; i < SEGMENT_COUNT; ++i) {
    if (logicalImage & (uint32_t(1) << i)) {
      const SegmentPosition &p = SEGMENT_MAP[reversed ? 27 - i : i];
      display.set(p.row, p.bit);
    }
  }
  checked(display.write());
}

void fillSegments(uint8_t count) {
  logicalImage = (uint32_t(1) << count) - 1;
  renderLogical();
}

uint8_t themeHeight(uint32_t elapsed) {
  // Deterministic beat-like peaks; simulated meter, not audio analysis.
  static const uint8_t peaks[] = {8,24,13,28,10,21,16,26,7,19,12,28,15,23,9,25};
  uint8_t index = (elapsed / 120) % 16;
  uint8_t next = (index + 1) % 16;
  int16_t height = peaks[index] + (int16_t(peaks[next]) - peaks[index]) *
                   int16_t(elapsed % 120) / 120;
  return uint8_t(height);
}

void renderVolume() {
  VolumeDisplay::Frame state = volumeDisplay.render(millis(), brightness);
  if (themePlaying && !volumeDisplay.muted() && !volumeDisplay.transitioning())
    state.image = VolumeDisplay::fill(themeHeight(uint32_t(millis() - themeStarted)));
  if (!applyBrightness(state.brightness)) return;
  if (logicalImage != state.image) {
    logicalImage = state.image;
    renderLogical();
  }
}

void showVolume() {
  mode = VOLUME;
  // Force a refresh only when returning from another display mode.
  logicalImage = 0xffffffffUL;
  renderVolume();
}

uint8_t encoderPhases() {
#if defined(__AVR_ATmega328P__)
  // D2/D3 share PORTD: capture both contacts at the same instant.
  uint8_t pins = PIND;
  return ((pins & _BV(PD2)) ? 2 : 0) | ((pins & _BV(PD3)) ? 1 : 0);
#else
  return (digitalRead(ENCODER_A_PIN) == HIGH ? 2 : 0) |
         (digitalRead(ENCODER_B_PIN) == HIGH ? 1 : 0);
#endif
}

void encoderEdge() {
  int8_t delta = encoderInput.rotate(encoderPhases());
  // Keep ISR work bounded: no display, Wire, Serial, or millis calls here.
  if (delta > 0 && encoderDelta < 1000) ++encoderDelta;
  else if (delta < 0 && encoderDelta > -1000) --encoderDelta;
}

void encoderInterrupt() {
  ++encoderInterrupts; // Low-byte activity counter, intentionally wraps.
  encoderEdge();
}

#if defined(__AVR_ATmega328P__)
ISR(TIMER1_COMPA_vect) {
  loopDiagnostics.tick();
  if (loopDiagnostics.stalledPhase) {
    if (loopDiagnostics.faultLedOn()) PORTB |= _BV(PB5);
    else PORTB &= ~_BV(PB5);
  }
}
#endif

void startLoopDiagnostics() {
#if defined(__AVR_ATmega328P__)
  // Diagnostic-only Timer1 use: no Servo or PWM on D9/D10 in this sketch.
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = F_CPU / 64 / 100 - 1;
    TIFR1 = _BV(OCF1A);
    TIMSK1 = _BV(OCIE1A);
    TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);
  }
#endif
}

void readEncoder() {
  noInterrupts();
  // Reconcile a missed edge; unchanged samples do not generate movement.
  encoderEdge();
  int16_t delta = encoderDelta;
  encoderDelta = 0;
  interrupts();
  bool press = encoderInput.button(digitalRead(ENCODER_BUTTON_PIN) == LOW, millis());
  if (!delta && !press) return;
  int32_t next = int32_t(volumeDisplay.volume()) +
                 int32_t(delta) * ENCODER_DIRECTION * ENCODER_VOLUME_STEP;
  volumeDisplay.setVolume(next < 0 ? 0 : next > 100 ? 100 : next);
  if (press) volumeDisplay.setMuted(!volumeDisplay.muted(), millis());
  // A vent retains priority; inputs still update the state restored afterward.
  if (ready && mode != VENT) {
    if (mode == VOLUME) renderVolume();
    else showVolume();
  }
  // Drop optional telemetry rather than block input/display work on Serial.
  if (Serial.availableForWrite() >= 32) {
    Serial.print(F("Volume: ")); Serial.print(volumeDisplay.volume());
    Serial.println(volumeDisplay.muted() ? F(" (muted)") : F(" (unmuted)"));
  }
}

void printPosition(uint8_t index, uint8_t row, uint8_t bit) {
  Serial.print(index);
  Serial.print(F(": row(COM)=")); Serial.print(row);
  Serial.print(F(" bit(ROW/anode)=")); Serial.print(bit);
  Serial.print(F(" RAM byte=0x")); Serial.println(2 * row + bit / 8, HEX);
}

void advanceDiscovery() {
  if (frame == SEGMENT_COUNT) {
    mode = IDLE;
    fillSegments(0);
    if (ready) Serial.println(F("Discovery complete; record observations in VERIFIED_MAPPING.md."));
    return;
  }
  display.clear();
  display.set(frame / 7, frame % 7);
  if (checked(display.write())) {
    printPosition(frame, frame / 7, frame % 7);
    Serial.println(F("Record the lit segment, then press Return to continue; off to stop."));
    ++frame;
  }
}

void startMode(Mode next) {
  if (!applyBrightness(brightness)) return;
  mode = next;
  frame = 0;
  direction = 1;
  lastStep = millis();
  // First frame runs on the next loop, without blocking Serial.
  lastStep -= 5000;
  if (next == DISCOVER) advanceDiscovery();
}

void scanBus() {
  bool found = false;
  Serial.println(F("Scanning I2C addresses 0x08..0x77 (ACK does not identify chip)..."));
  for (uint8_t address = 8; address < 0x78; ++address) {
    uint8_t status = display.probe(address);
    if (status == 0) {
      Serial.print(F("ACK 0x")); Serial.println(address, HEX);
      if (address == HT_ADDRESS) found = true;
    } else if (status != 2) {
      Serial.print(F("Bus error ")); Serial.print(status);
      Serial.print(F(" at 0x")); Serial.println(address, HEX);
      break;
    }
  }
  if (!found) {
    ready = false; mode = IDLE;
    Serial.println(F("ERROR: configured HT16K33 address absent. Check address/power/wiring."));
  } else if (!ready) {
    ready = checked(display.begin(HT_ADDRESS, brightness));
    if (ready) {
      appliedBrightness = 255;
      applyBrightness(brightness);
      logicalImage = 0;
      showVolume();
      Serial.println(F("HT16K33 initialized; saved volume restored."));
    }
  } else Serial.println(F("Configured HT16K33 address ACK OK."));
}

bool number(const char *s, uint16_t maximum, uint16_t &value) {
  if (!s || !*s) return false;
  uint32_t result = 0;
  for (; *s; ++s) {
    if (*s < '0' || *s > '9') return false;
    result = result * 10 + (*s - '0');
    if (result > maximum) return false;
  }
  value = uint16_t(result);
  return true;
}

void reportStatus() {
  char line[64];
  snprintf(line, sizeof(line), "up=%lu ready=%u vol=%u mute=%u AB=%u SW=%u err=%lu\r\n",
           (unsigned long)(millis() / 1000), unsigned(ready),
           unsigned(volumeDisplay.volume()), unsigned(volumeDisplay.muted()),
           unsigned(encoderPhases()), unsigned(digitalRead(ENCODER_BUTTON_PIN)),
           (unsigned long)displayFailures);
  if (Serial.availableForWrite() >= int(strlen(line))) Serial.print(line);
}

void reportDiagnostics() {
  char line[64];
  snprintf(line, sizeof(line), "phase=%u stall=%u last=%u count=%u irq8=%u twiTimeout=%u\r\n",
           unsigned(loopDiagnostics.phase), unsigned(loopDiagnostics.stalledPhase),
           unsigned(loopDiagnostics.lastStall), unsigned(loopDiagnostics.stalls),
           unsigned(encoderInterrupts), unsigned(Wire.getWireTimeoutFlag()));
  if (Serial.availableForWrite() >= int(strlen(line))) Serial.print(line);
}

void heartbeat() {
  if (uint32_t(millis() - lastHeartbeat) < 500) return;
  lastHeartbeat = millis();
  heartbeatOn = !heartbeatOn;
  digitalWrite(LED_BUILTIN, heartbeatOn ? HIGH : LOW);
}

void setTheme(bool playing);
void startVent();

void execute(char *line) {
  char *cmd = strtok(line, " \t");
  char *arg = strtok(nullptr, " \t");
  char *extra = strtok(nullptr, " \t");
  if (!cmd) {
    if (ready && mode == DISCOVER) advanceDiscovery();
    return;
  }
  const bool takesNumber = !strcmp(cmd, "fill") || !strcmp(cmd, "brightness") || !strcmp(cmd, "speed") || !strcmp(cmd, "volume");
  if (extra || (arg && !takesNumber)) {
    Serial.println(F("ERROR: unexpected argument.")); return;
  }
  if (!strcmp(cmd, "help")) { help(); return; }
  if (!strcmp(cmd, "diag")) { reportDiagnostics(); return; }
  if (!strcmp(cmd, "status")) { reportStatus(); return; }
  if (!strcmp(cmd, "scan")) { scanBus(); return; }
  uint16_t value = 0;
  uint16_t maximum = !strcmp(cmd, "fill") ? 28 : !strcmp(cmd, "brightness") ? 15 : !strcmp(cmd, "volume") ? 100 : 5000;
  if (takesNumber && (!number(arg, maximum, value) || (!strcmp(cmd, "speed") && value < 20))) {
    Serial.println(F("ERROR: invalid number/range. See help.")); return;
  }
  if (!ready) { Serial.println(F("ERROR: display unavailable; run scan.")); return; }
  if (!strcmp(cmd, "theme")) setTheme(true);
  else if (!strcmp(cmd, "stoptheme")) setTheme(false);
  else if (!strcmp(cmd, "volume")) {
    volumeDisplay.setVolume(value);
    if (mode != VENT) showVolume();
  } else if (!strcmp(cmd, "mute") || !strcmp(cmd, "unmute")) {
    volumeDisplay.setMuted(!strcmp(cmd, "mute") ? !volumeDisplay.muted() : false, millis());
    if (mode != VENT) showVolume();
    Serial.println(volumeDisplay.muted() ? F("Muted") : F("Unmuted"));
  } else if (!strcmp(cmd, "show")) showVolume();
  else if (!strcmp(cmd, "vent") || !strcmp(cmd, "purge")) {
    startVent();
  } else if (!strcmp(cmd, "off") || !strcmp(cmd, "all") || !strcmp(cmd, "fill")) {
    if (!applyBrightness(brightness)) return;
    mode = IDLE;
    fillSegments(!strcmp(cmd, "all") ? 28 : !strcmp(cmd, "off") ? 0 : value);
  } else if (!strcmp(cmd, "brightness")) {
    brightness = value;
    if (mode == VOLUME) renderVolume();
    else applyBrightness(brightness);
  } else if (!strcmp(cmd, "speed")) stepMs = value;
  else if (!strcmp(cmd, "invert")) {
    reversed = !reversed;
    Serial.println(reversed ? F("Order reversed") : F("Order normal"));
    if (mode != DISCOVER) renderLogical();
  } else if (!strcmp(cmd, "discover")) startMode(DISCOVER);
  else if (!strcmp(cmd, "test")) startMode(TEST);
  else if (!strcmp(cmd, "chase")) startMode(CHASE);
  else if (!strcmp(cmd, "bounce")) startMode(BOUNCE);
  else if (!strcmp(cmd, "demo")) startMode(DEMO);
  else { Serial.println(F("ERROR: unknown command. See help.")); return; }
  if (ready) Serial.println(F("OK"));
}

void readSerial() {
  // Bound work per loop even during a continuous incoming stream.
  for (uint8_t budget = 0; budget < 32 && Serial.available(); ++budget) {
    char c = Serial.read();
    // Treat CRLF as one Return, even across separate loop iterations.
    if (c == '\n' && previousWasCR) {
      previousWasCR = false;
      continue;
    }
    previousWasCR = c == '\r';
    if (c == '\r' || c == '\n') {
      if (inputOverflow) Serial.println(F("ERROR: command too long; discarded."));
      else { input[inputLength] = '\0'; execute(input); }
      inputLength = 0; inputOverflow = false;
    } else if (!inputOverflow) {
      if (inputLength < sizeof(input) - 1) input[inputLength++] = c;
      else inputOverflow = true;
    }
  }
}

void animate() {
  if (!ready || mode == IDLE || mode == DISCOVER) return;
  if (mode == VOLUME) { renderVolume(); return; }
  if (mode == VENT) {
    VentAnimation::Frame state = ventAnimation.render(millis());
    if (state.phase == VentAnimation::VENT_COMPLETE) {
      volumeDisplay.settle(millis());
      showVolume();
    }
    else if (logicalImage != state.image) {
      logicalImage = state.image;
      renderLogical();
    }
    return;
  }
  uint16_t interval = stepMs;
  if (mode == SELF_TEST) interval = 250;
  uint32_t now = millis();
  if (uint32_t(now - lastStep) < interval) return;
  lastStep = now;
  if (mode == SELF_TEST) {
    if (frame == 0) { fillSegments(28); ++frame; }
    else if (frame == 1) { fillSegments(0); ++frame; }
    else showVolume();
  } else if (mode == DEMO) {
    fillSegments(frame);
    if (frame == 28) direction = -1;
    else if (frame == 0) direction = 1;
    frame += direction;
  } else {
    if (mode == TEST && frame == 28) {
      mode = IDLE; fillSegments(0);
      Serial.println(F("Test sequence complete; visual verification required.")); return;
    }
    logicalImage = uint32_t(1) << frame;
    renderLogical();
    if (mode == TEST) {
      const SegmentPosition &p = SEGMENT_MAP[reversed ? 27 - frame : frame];
      printPosition(frame, p.row, p.bit);
      ++frame;
    } else if (mode == BOUNCE) {
      if (frame == 27) direction = -1;
      else if (frame == 0) direction = 1;
      frame += direction;
    } else frame = (frame + 1) % 28;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);
  encoderInput.begin(encoderPhases(), digitalRead(ENCODER_BUTTON_PIN) == LOW, millis());
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderInterrupt, CHANGE);
  pinMode(THEME_SWITCH_PIN, INPUT_PULLUP);
  pinMode(VENT_SWITCH_PIN, INPUT_PULLUP);
  themeSwitch.begin(digitalRead(THEME_SWITCH_PIN) == LOW, millis());
  ventSwitch.begin(digitalRead(VENT_SWITCH_PIN) == LOW, millis());
  Wire.begin();
  Wire.setClock(100000);
// Required by this Nano sketch. AVR Boards 1.8.8 provides this API but
  // does not define WIRE_HAS_TIMEOUT; a feature-macro guard silently disables it.
  Wire.setWireTimeout(25000, true);
  Serial.println(F("BL28Z / HT16K33 standalone display test"));
  if (!validSegmentMap()) {
    Serial.println(F("ERROR: duplicate/out-of-range map. Fix SegmentMap.h."));
    // Invalid mapping must never reach display rendering.
    for (;;) {}
  }
  Serial.println(SEGMENT_MAP_VERIFIED ? F("Map marked hardware-verified; see record.") : F("WARNING: initial map UNVERIFIED. Run discover first."));
  help();
  scanBus();
  if (ready) startMode(DISCOVERY_AT_BOOT ? DISCOVER : SELF_TEST);
  startLoopDiagnostics();
}

void setTheme(bool playing) {
  themePlaying = playing;
  if (playing) themeStarted = millis();
  if (ready && mode != VENT) showVolume();
}

void startVent() {
  if (!ready) return;
  // Read the actual buffer, including raw mapping diagnostics.
  uint32_t initial = 0;
  for (uint8_t i=0;i<SEGMENT_COUNT;++i) {
    const SegmentPosition &p = SEGMENT_MAP[reversed ? 27-i : i];
    if (display.isSet(p.row,p.bit)) initial |= uint32_t(1)<<i;
  }
  startMode(VENT);
  if (!ready) return;
  logicalImage = initial;
  ventAnimation.begin(millis(),initial);
}

void readToggles() {
  uint32_t now = millis();
  int8_t themeEvent = themeSwitch.update(digitalRead(THEME_SWITCH_PIN) == LOW, now);
  int8_t ventEvent = ventSwitch.update(digitalRead(VENT_SWITCH_PIN) == LOW, now);
  if (themeEvent >= 0) setTheme(themeEvent == 1);
  // OFF only rearms the edge detector; it does not abort an active vent.
  if (ventEvent == 1 && mode != VENT) startVent();
  if (themePlaying && uint32_t(now - themeStarted) >= THEME_DURATION_MS)
    setTheme(false);
}

void recoverDisplay() {
  if (ready || uint32_t(millis() - lastDisplayRetry) < 1000) return;
  lastDisplayRetry = millis();
  // A transient bus error must not leave the encoder apparently dead forever.
  // Retry only the configured device, without a full scan or repeated logging.
  if (!display.begin(HT_ADDRESS, brightness)) return;
  ready = true;
  appliedBrightness = 255;
  showVolume();
}

void loop() {
  loopDiagnostics.progressed = true;
  heartbeat();
  loopDiagnostics.phase = 6;
  readToggles();
  loopDiagnostics.phase = 1;
  readEncoder();
  loopDiagnostics.phase = 2;
  recoverDisplay();
  loopDiagnostics.phase = 3;
  readSerial();
  loopDiagnostics.phase = 4;
  animate();
  loopDiagnostics.phase = 0;
}
