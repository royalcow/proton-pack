#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>
#include <string.h>
#include "Ht16k33.h"
#include "SegmentMap.h"

constexpr uint8_t HT_ADDRESS = 0x70;
constexpr uint8_t DEFAULT_BRIGHTNESS = 2;
constexpr uint16_t DEFAULT_STEP_MS = 100;
// Enable for first wiring bring-up: discovery replaces the startup demo.
constexpr bool DISCOVERY_AT_BOOT = false;

enum Mode { IDLE, SELF_TEST, DEMO, TEST, DISCOVER, CHASE, BOUNCE };
Ht16k33 display;
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
  Serial.println(F("scan | discover | test | all | off | fill 0..28"));
  Serial.println(F("Return on an empty line advances discovery"));
  Serial.println(F("chase | bounce | demo | brightness 0..15 | invert"));
  Serial.println(F("speed 20..5000 (animation ms) | help"));
}

bool checked(bool success) {
  if (!success) {
    ready = false;
    mode = IDLE;
    Serial.println(F("ERROR: HT16K33 I2C failed; animation stopped. Check wiring; run scan."));
  }
  return success;
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
      logicalImage = 0;
      Serial.println(F("HT16K33 initialized at configured address; display off."));
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

void execute(char *line) {
  char *cmd = strtok(line, " \t");
  char *arg = strtok(nullptr, " \t");
  char *extra = strtok(nullptr, " \t");
  if (!cmd) {
    if (ready && mode == DISCOVER) advanceDiscovery();
    return;
  }
  const bool takesNumber = !strcmp(cmd, "fill") || !strcmp(cmd, "brightness") || !strcmp(cmd, "speed");
  if (extra || (arg && !takesNumber)) {
    Serial.println(F("ERROR: unexpected argument.")); return;
  }
  if (!strcmp(cmd, "help")) { help(); return; }
  if (!strcmp(cmd, "scan")) { scanBus(); return; }
  uint16_t value = 0;
  uint16_t maximum = !strcmp(cmd, "fill") ? 28 : !strcmp(cmd, "brightness") ? 15 : 5000;
  if (takesNumber && (!number(arg, maximum, value) || (!strcmp(cmd, "speed") && value < 20))) {
    Serial.println(F("ERROR: invalid number/range. See help.")); return;
  }
  if (!ready) { Serial.println(F("ERROR: display unavailable; run scan.")); return; }
  if (!strcmp(cmd, "off") || !strcmp(cmd, "all") || !strcmp(cmd, "fill")) {
    mode = IDLE;
    fillSegments(!strcmp(cmd, "all") ? 28 : !strcmp(cmd, "off") ? 0 : value);
  } else if (!strcmp(cmd, "brightness")) {
    if (checked(display.setBrightness(value))) brightness = value;
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
  uint16_t interval = stepMs;
  if (mode == SELF_TEST) interval = 250;
  uint32_t now = millis();
  if (uint32_t(now - lastStep) < interval) return;
  lastStep = now;
  if (mode == SELF_TEST) {
    if (frame == 0) { fillSegments(28); ++frame; }
    else if (frame == 1) { fillSegments(0); ++frame; }
    else startMode(DEMO);
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
  Wire.begin();
  Wire.setClock(100000);
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif
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
}

void loop() {
  readSerial();
  animate();
}
