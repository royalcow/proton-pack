#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include <Adafruit_Soundboard.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

// Uses the Adafruit Audio FX Sound Board wiring and track naming from the
// original Neutrino Wand sketch.

// Existing SpiritMinimal LED wiring.
constexpr uint8_t NEO_POWER_PIN = 2;
constexpr uint8_t NEO_CYCLOTRON_PIN = 3;
constexpr uint8_t POWER_CELL_COUNT = 16;
constexpr uint8_t ANIMATED_POWER_CELLS = 15;
// Set this to 4 after the remaining two jewels are connected.
constexpr uint8_t CYCLOTRON_JEWEL_COUNT = 4;
constexpr uint8_t PIXELS_PER_CYCLOTRON_JEWEL = 7;
constexpr uint8_t CYCLOTRON_COUNT = CYCLOTRON_JEWEL_COUNT * PIXELS_PER_CYCLOTRON_JEWEL;
// Change this to 14 if the opposite end of your installed strip is the bottom.
constexpr uint8_t BOTTOM_POWER_CELL = 0;

// External pull-downs make an unpowered wand read LOW/LOW. Keep the physical
// wiring names fixed: Activate is D4 and Fire is D5. The state machine uses
// the combined D4/D5 sequence observed from that wiring instead of assigning
// state directly from either pin's level.
constexpr uint8_t ACTIVATE_SWITCH_PIN = 4;
constexpr uint8_t FIRE_SWITCH_PIN = 5;
constexpr uint8_t ACTIVATE_ACTIVE_LEVEL = LOW;
constexpr uint8_t FIRE_ACTIVE_LEVEL = LOW;
constexpr uint8_t VOLTAGE_BUTTON_PIN = 6;  // momentary button to GND
constexpr uint8_t VOLTAGE_SENSE_PIN = A0;  // midpoint of the voltage divider

// INA219 current sensing. The sensor is installed before the wand power
// switch, so bus voltage remains present and current draw determines on/off.
constexpr float WAND_ON_CURRENT_MA = 10.0;
constexpr float WAND_OFF_CURRENT_MA = 1.0;
constexpr unsigned long WAND_CURRENT_SAMPLE_INTERVAL = 25;
constexpr unsigned long WAND_FIRING_CURRENT_SAMPLE_INTERVAL = 10;
constexpr unsigned long WAND_POWER_ON_QUALIFY_INTERVAL = 50;
constexpr unsigned long WAND_POWER_OFF_QUALIFY_INTERVAL = 25;
constexpr unsigned long WAND_CURRENT_LOG_INTERVAL = 500;
constexpr bool DEBUG_WAND_CURRENT = false;

// Example divider for measuring an 11.1 V (12.6 V full) LiPo. Adjust these
// constants to match the resistors actually installed.
constexpr float VOLTAGE_DIVIDER_R1 = 100000.0; // battery positive to A0
constexpr float VOLTAGE_DIVIDER_R2 = 33000.0;  // A0 to GND
constexpr float ADC_REFERENCE_VOLTS = 5.0;
constexpr float VOLTAGE_CALIBRATION = 1.0;
constexpr uint8_t VOLTAGE_SAMPLE_COUNT = 16;
constexpr unsigned long VOLTAGE_BUTTON_DEBOUNCE = 40;

// Adafruit Audio FX Sound Board connections.
constexpr uint8_t SFX_RESET_PIN = 9;
constexpr uint8_t SFX_RX_PIN = 10; // Nano TX -> Sound Board RX
constexpr uint8_t SFX_TX_PIN = 11; // Nano RX <- Sound Board TX
constexpr uint8_t SFX_ACT_PIN = 12; // used only if idle replay is enabled below
constexpr bool SFX_ACT_CONNECTED = true;
constexpr bool REPLAY_IDLE_TRACK = true;

Adafruit_NeoPixel powerCell(POWER_CELL_COUNT, NEO_POWER_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel cyclotron(CYCLOTRON_COUNT, NEO_CYCLOTRON_PIN, NEO_GRB + NEO_KHZ800);
SoftwareSerial soundSerial(SFX_TX_PIN, SFX_RX_PIN);
Adafruit_Soundboard soundBoard(&soundSerial, NULL, SFX_RESET_PIN);
Adafruit_INA219 wandCurrentSensor;

// Copy the matching files from the included sounds/ folder to the Audio FX board.
char startupTrack[] =  "T00     WAV";
char blastTrack[] =    "T01     WAV";
char fireEndTrack[] =  "T02     WAV";
char idleTrack[] =     "T03     WAV";
char shutdownTrack[] = "T04     WAV";
enum AudioCommandState { AUDIO_READY, AUDIO_SEND_STOP, AUDIO_SEND_PLAY };
char *pendingAudioTrack = NULL;
AudioCommandState audioCommandState = AUDIO_READY;
uint8_t pendingAudioAttempts = 0;
unsigned long pendingAudioMillis = 0;

enum PackState { HOLDING, BOOTING, ACTIVE, FIRING, SHUTTING_DOWN };
PackState packState = HOLDING;

enum ControlPattern : uint8_t {
  CONTROLS_RELEASED = 0,
  CONTROL_ACTIVATE = 1,
  CONTROL_FIRE = 2,
  CONTROLS_BOTH = CONTROL_ACTIVATE | CONTROL_FIRE
};
// INA219 current distinguishes wand power from control state. With current
// above threshold, LOW/LOW is armed, HIGH/LOW is active, and HIGH/HIGH fires.
constexpr uint8_t WAND_READY_PATTERN = CONTROLS_BOTH;      // D4 LOW,  D5 LOW
constexpr uint8_t WAND_ACTIVE_PATTERN = CONTROL_FIRE;      // D4 HIGH, D5 LOW
constexpr uint8_t WAND_FIRE_PATTERN = CONTROLS_RELEASED;   // D4 HIGH, D5 HIGH
// D4/D5 are logic outputs from the wand rather than bare mechanical contacts,
// so they only need a short noise filter. The 2.6-second activation transition
// block still absorbs the wand's intentional HIGH/LOW -> HIGH/HIGH -> HIGH/LOW
// sequence after this first edge is accepted.
constexpr unsigned long CONTROL_DEBOUNCE_INTERVAL = 3;
constexpr unsigned long FIRE_COMMAND_QUALIFY_INTERVAL = 10;
constexpr unsigned long ACTIVATION_TRANSITION_BLOCK_INTERVAL = 2600;
constexpr unsigned long DEACTIVATE_QUALIFY_INTERVAL = 50;
constexpr bool DEBUG_CONTROL_INPUTS = true;
uint8_t controlCandidate = CONTROLS_BOTH;
uint8_t stableControls = CONTROLS_BOTH;
unsigned long controlCandidateSince = 0;
unsigned long activationTransitionBlockStarted = 0;
bool activationTransitionBlockActive = false;
bool fireCommandPending = false;
unsigned long fireCommandSince = 0;
unsigned long deactivateSince = 0;
bool deactivatePending = false;
bool wandCurrentSensorReady = false;
bool wandPowered = false;
bool wandPowerCandidate = false;
bool wandControlsArmed = false;
float wandCurrentMa = 0.0;
unsigned long wandPowerCandidateSince = 0;
unsigned long previousWandCurrentSampleMillis = 0;
unsigned long previousWandCurrentLogMillis = 0;
uint8_t lastReportedRawControls = 0xFF;
uint8_t lastReportedStableControls = 0xFF;
PackState lastReportedPackState = HOLDING;

uint8_t bootPowerCell = 0;
int8_t bootSweepCell = ANIMATED_POWER_CELLS - 1;
int16_t bootCyclotronBrightness = 0;
int8_t bootCyclotronDirection = 5;
uint8_t powerCellStep = 0;
uint8_t cyclotronStep = 0;
uint8_t fireFlashStep = 0;
bool holdingPowerCellDrawn = false;
uint8_t holdingPowerCellBrightness = 20;
int8_t holdingPowerCellDirection = 2;
uint8_t shutdownPowerCellCount = 0;
uint8_t shutdownInitialPowerCellCount = 0;
uint32_t shutdownPowerCells[POWER_CELL_COUNT];
uint32_t shutdownCyclotronCells[CYCLOTRON_COUNT];

unsigned long previousPowerCellMillis = 0;
unsigned long previousCyclotronMillis = 0;
unsigned long previousCyclotronFadeFrameMillis = 0;
unsigned long previousBootMillis = 0;
unsigned long previousFireFlashMillis = 0;
unsigned long previousShutdownMillis = 0;
unsigned long previousHeartbeatMillis = 0;
unsigned long previousHoldingPowerCellMillis = 0;

constexpr unsigned long BOOT_POWER_CELL_INTERVAL = 40;
constexpr unsigned long BOOT_CYCLOTRON_SLOW_INTERVAL = 14;
constexpr unsigned long BOOT_CYCLOTRON_FAST_INTERVAL = 3;
constexpr unsigned long IDLE_CYCLOTRON_INTERVAL = 750;
constexpr unsigned long FIRE_POWER_CELL_INTERVAL = 28;
constexpr unsigned long FIRE_CYCLOTRON_INTERVAL = 105;
constexpr unsigned long FIRE_FLASH_INTERVAL = 45;
constexpr unsigned long SHUTDOWN_FADE_INTERVAL = 55;
constexpr unsigned long CYCLOTRON_FADE_FRAME_INTERVAL = 10;
constexpr unsigned long HEARTBEAT_INTERVAL = 500;
constexpr unsigned long HOLDING_POWER_CELL_FADE_INTERVAL = 25;
constexpr uint8_t HOLDING_POWER_CELL_MIN = 20;
constexpr uint8_t HOLDING_POWER_CELL_MAX = 150;
constexpr unsigned long AUDIO_COMMAND_GAP = 35;
bool idleTrackWasPlaying = false;
bool previousVoltageButtonState = HIGH;
unsigned long previousVoltageButtonMillis = 0;

bool timeElapsed(unsigned long now, unsigned long &then, unsigned long interval) {
  if (now - then < interval) return false;
  then = now;
  return true;
}

uint8_t readControlPattern() {
  uint8_t controls = CONTROLS_RELEASED;
  if (digitalRead(ACTIVATE_SWITCH_PIN) == ACTIVATE_ACTIVE_LEVEL) {
    controls |= CONTROL_ACTIVATE;
  }
  if (digitalRead(FIRE_SWITCH_PIN) == FIRE_ACTIVE_LEVEL) {
    controls |= CONTROL_FIRE;
  }
  return controls;
}

void printControlPattern(uint8_t controls) {
  // Print the assigned control, physical pin, and actual voltage logic level.
  Serial.print(F("Activate(D"));
  Serial.print(ACTIVATE_SWITCH_PIN);
  Serial.print(F(")="));
  Serial.print((controls & CONTROL_ACTIVATE) ? F("LOW") : F("HIGH"));
  Serial.print(F(" Fire(D"));
  Serial.print(FIRE_SWITCH_PIN);
  Serial.print(F(")="));
  Serial.print((controls & CONTROL_FIRE) ? F("LOW") : F("HIGH"));
}

uint8_t updateControlPattern(unsigned long now) {
  const uint8_t currentControls = readControlPattern();
  if (DEBUG_CONTROL_INPUTS && currentControls != lastReportedRawControls) {
    Serial.print(now);
    Serial.print(F(" ms RAW    "));
    printControlPattern(currentControls);
    Serial.print(F(" state="));
    Serial.println((uint8_t)packState);
    lastReportedRawControls = currentControls;
  }

  if (currentControls != controlCandidate) {
    controlCandidate = currentControls;
    controlCandidateSince = now;
  } else if (stableControls != controlCandidate &&
             now - controlCandidateSince >= CONTROL_DEBOUNCE_INTERVAL) {
    stableControls = controlCandidate;
  }

  if (DEBUG_CONTROL_INPUTS && stableControls != lastReportedStableControls) {
    Serial.print(now);
    Serial.print(F(" ms STABLE "));
    printControlPattern(stableControls);
    Serial.print(F(" state="));
    Serial.println((uint8_t)packState);
    lastReportedStableControls = stableControls;
  }
  return stableControls;
}

void reportPackState(unsigned long now) {
  if (!DEBUG_CONTROL_INPUTS || packState == lastReportedPackState) return;
  Serial.print(now);
  Serial.print(F(" ms STATE  "));
  Serial.println((uint8_t)packState);
  lastReportedPackState = packState;
}

bool activationReleaseConfirmed(unsigned long now, uint8_t controls) {
  if (controls == WAND_ACTIVE_PATTERN || controls == WAND_FIRE_PATTERN) {
    deactivatePending = false;
    return false;
  }
  if (!deactivatePending) {
    deactivatePending = true;
    deactivateSince = now;
    return false;
  }
  return now - deactivateSince >= DEACTIVATE_QUALIFY_INTERVAL;
}

void beginActivationTransitionBlock(unsigned long now) {
  activationTransitionBlockStarted = now;
  activationTransitionBlockActive = true;
  if (DEBUG_CONTROL_INPUTS) {
    Serial.print(now);
    Serial.println(F(" ms INPUT  activation transition block started"));
  }
}

bool activationTransitionBlocked(unsigned long now) {
  if (!activationTransitionBlockActive) return false;
  if (now - activationTransitionBlockStarted <
      ACTIVATION_TRANSITION_BLOCK_INTERVAL) {
    return true;
  }
  activationTransitionBlockActive = false;
  if (DEBUG_CONTROL_INPUTS) {
    Serial.print(now);
    Serial.println(F(" ms INPUT  activation transition block ended"));
  }
  return false;
}

float currentMagnitude(float currentMa) {
  return currentMa < 0.0 ? -currentMa : currentMa;
}

void updateWandPower(unsigned long now) {
  // HIGH/HIGH can be either a real Fire command or a brief power-off transient.
  // Force a fresh current reading before ACTIVE is allowed to enter FIRING.
  const bool verifyingFireCommand =
      packState == ACTIVE && stableControls == WAND_FIRE_PATTERN;
  const bool fastPowerMonitoring = packState == FIRING || verifyingFireCommand;

  // Poll more often while firing so cutting wand power can stop the pack with
  // minimal delay. Other states retain the lower-overhead 25 ms polling rate.
  const unsigned long sampleInterval = fastPowerMonitoring
      ? WAND_FIRING_CURRENT_SAMPLE_INTERVAL
      : WAND_CURRENT_SAMPLE_INTERVAL;
  if (!wandCurrentSensorReady ||
      (!verifyingFireCommand &&
       now - previousWandCurrentSampleMillis < sampleInterval)) {
    return;
  }
  previousWandCurrentSampleMillis = now;
  wandCurrentMa = wandCurrentSensor.getCurrent_mA();
  const float magnitudeMa = currentMagnitude(wandCurrentMa);

  // Hysteresis prevents current noise near one threshold from rapidly
  // switching the detected power state.
  const bool proposedPowerState = wandPowered
      ? magnitudeMa > WAND_OFF_CURRENT_MA
      : magnitudeMa >= WAND_ON_CURRENT_MA;

  if (proposedPowerState != wandPowerCandidate) {
    wandPowerCandidate = proposedPowerState;
    wandPowerCandidateSince = now;
  }

  // While firing, or while validating a Fire transition, a reading below the
  // OFF threshold is accepted immediately.
  // The thresholds are widely separated (OFF < 1 mA, ON >= 10 mA), so this
  // does not weaken the conservative qualification used to arm the wand.
  const bool fastPowerOff = fastPowerMonitoring && !wandPowerCandidate;
  const unsigned long qualifyInterval = wandPowerCandidate
      ? WAND_POWER_ON_QUALIFY_INTERVAL
      : (fastPowerOff ? 0 : WAND_POWER_OFF_QUALIFY_INTERVAL);
  if (wandPowerCandidate != wandPowered &&
      now - wandPowerCandidateSince >= qualifyInterval) {
    wandPowered = wandPowerCandidate;
    // A power transition never counts as Activate. Require the powered wand's
    // LOW/LOW armed pattern followed by HIGH/LOW before beginning boot.
    wandControlsArmed = false;
    if (DEBUG_CONTROL_INPUTS) {
      Serial.print(now);
      Serial.print(F(" ms INPUT  wand "));
      Serial.println(wandPowered ? F("armed by current") : F("disarmed by current"));
    }
    if (DEBUG_WAND_CURRENT) {
      Serial.print(now);
      Serial.print(F(" ms WAND   "));
      Serial.print(wandPowered ? F("ON") : F("OFF"));
      Serial.print(F(" current="));
      Serial.print(wandCurrentMa, 2);
      Serial.println(F(" mA"));
    }
  }

  if (DEBUG_WAND_CURRENT &&
      now - previousWandCurrentLogMillis >= WAND_CURRENT_LOG_INTERVAL) {
    previousWandCurrentLogMillis = now;
    Serial.print(now);
    Serial.print(F(" ms INA219 current="));
    Serial.print(wandCurrentMa, 2);
    Serial.print(F(" mA detected="));
    Serial.println(wandPowered ? F("ON") : F("OFF"));
  }
}

void updateHeartbeat(unsigned long now) {
  if (timeElapsed(now, previousHeartbeatMillis, HEARTBEAT_INTERVAL)) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

float readSupplyVoltage() {
  uint32_t sampleTotal = 0;
  for (uint8_t sample = 0; sample < VOLTAGE_SAMPLE_COUNT; ++sample) {
    sampleTotal += analogRead(VOLTAGE_SENSE_PIN);
  }

  const float averageAdc = (float)sampleTotal / VOLTAGE_SAMPLE_COUNT;
  const float senseVoltage = averageAdc * ADC_REFERENCE_VOLTS / 1023.0;
  const float dividerRatio = (VOLTAGE_DIVIDER_R1 + VOLTAGE_DIVIDER_R2) /
                             VOLTAGE_DIVIDER_R2;
  return senseVoltage * dividerRatio * VOLTAGE_CALIBRATION;
}

void onVoltageMeasured(float volts) {
  // Stub point for a future display, bar graph, spoken-voltage track, etc.
  Serial.print(F("Measured supply voltage: "));
  Serial.print(volts, 2);
  Serial.println(F(" V"));
}

void updateVoltageButton(unsigned long now) {
  const bool buttonState = digitalRead(VOLTAGE_BUTTON_PIN);
  const bool newPress = buttonState == LOW && previousVoltageButtonState == HIGH;

  if (newPress && now - previousVoltageButtonMillis >= VOLTAGE_BUTTON_DEBOUNCE) {
    previousVoltageButtonMillis = now;
    onVoltageMeasured(readSupplyVoltage());
  }
  previousVoltageButtonState = buttonState;
}

void queueAudio(char *track, bool stopCurrent = true) {
  // The newest state-transition cue supersedes any older pending cue.
  pendingAudioTrack = track;
  audioCommandState = stopCurrent ? AUDIO_SEND_STOP : AUDIO_SEND_PLAY;
  pendingAudioAttempts = 0;
  pendingAudioMillis = millis();
}

void serviceAudio(unsigned long now) {
  if (pendingAudioTrack == NULL || (long)(now - pendingAudioMillis) < 0) return;

  switch (audioCommandState) {
    case AUDIO_SEND_STOP:
      // Stop is a single-byte command. Do not wait for its acknowledgement;
      // the timed gap below lets the board return to idle while lights run.
      soundSerial.print('q');
      audioCommandState = AUDIO_SEND_PLAY;
      pendingAudioMillis = now + AUDIO_COMMAND_GAP;
      break;

    case AUDIO_SEND_PLAY:
      if (soundBoard.playTrack(pendingAudioTrack)) {
        pendingAudioTrack = NULL;
        audioCommandState = AUDIO_READY;
      } else if (pendingAudioAttempts < 1) {
        // Retry once after a timed gap if the board did not acknowledge Play.
        ++pendingAudioAttempts;
        pendingAudioMillis = millis() + AUDIO_COMMAND_GAP;
      } else {
        pendingAudioTrack = NULL;
        audioCommandState = AUDIO_READY;
      }
      break;

    case AUDIO_READY:
      pendingAudioTrack = NULL;
      break;
  }
}

void clearPixels() {
  powerCell.clear();
  cyclotron.clear();
  powerCell.show();
  cyclotron.show();
}

uint32_t dimColor(uint32_t color, uint8_t brightness) {
  const uint8_t red = (color >> 16) & 0xFF;
  const uint8_t green = (color >> 8) & 0xFF;
  const uint8_t blue = color & 0xFF;
  return powerCell.Color((uint16_t)red * brightness / 255,
                         (uint16_t)green * brightness / 255,
                         (uint16_t)blue * brightness / 255);
}

void beginShutdown(unsigned long now) {
  queueAudio(shutdownTrack);
  for (uint8_t i = 0; i < POWER_CELL_COUNT; ++i) {
    shutdownPowerCells[i] = powerCell.getPixelColor(i);
  }
  for (uint8_t i = 0; i < CYCLOTRON_COUNT; ++i) {
    shutdownCyclotronCells[i] = cyclotron.getPixelColor(i);
  }
  // The active power-cell pattern is contiguous from index 0. Capture its
  // displayed position so shutdown can count backward from that exact point.
  shutdownPowerCellCount = 0;
  for (uint8_t i = 0; i < ANIMATED_POWER_CELLS; ++i) {
    if (shutdownPowerCells[i] != 0) shutdownPowerCellCount = i + 1;
  }
  shutdownInitialPowerCellCount = shutdownPowerCellCount;
  // Let runShutdown() draw its first reverse-count frame immediately instead
  // of waiting for the next 55 ms shutdown interval.
  previousShutdownMillis = now - SHUTDOWN_FADE_INTERVAL;
}

void runShutdown(unsigned long now) {
  if (!timeElapsed(now, previousShutdownMillis, SHUTDOWN_FADE_INTERVAL)) return;

  // Step backward before drawing so the first shutdown frame is visibly
  // different from the last active/firing frame.
  if (shutdownPowerCellCount > 0) {
    --shutdownPowerCellCount;
  }

  for (uint8_t i = 0; i < ANIMATED_POWER_CELLS; ++i) {
    powerCell.setPixelColor(i, i < shutdownPowerCellCount ? shutdownPowerCells[i] : 0);
  }
  powerCell.setPixelColor(POWER_CELL_COUNT - 1, 0);

  // Fade the cyclotron over the same duration as the reverse power-cell count.
  const uint8_t cyclotronBrightness = shutdownInitialPowerCellCount == 0
      ? 0
      : (uint16_t)shutdownPowerCellCount * 255 / shutdownInitialPowerCellCount;
  for (uint8_t i = 0; i < CYCLOTRON_COUNT; ++i) {
    cyclotron.setPixelColor(i, dimColor(shutdownCyclotronCells[i], cyclotronBrightness));
  }
  powerCell.show();
  cyclotron.show();

  if (shutdownPowerCellCount == 0) {
    clearPixels();
    holdingPowerCellDrawn = false;
    packState = HOLDING;
  }
}

void beginBoot(unsigned long now) {
  // The armed HOLDING state has no active track, so start the activation cue
  // directly instead of sending Stop and waiting through AUDIO_COMMAND_GAP.
  queueAudio(startupTrack, false);
  bootPowerCell = 0;
  bootSweepCell = ANIMATED_POWER_CELLS - 1;
  bootCyclotronBrightness = 0;
  bootCyclotronDirection = 5;
  powerCellStep = 0;
  cyclotronStep = 0;
  fireFlashStep = 0;
  holdingPowerCellDrawn = false;
  clearPixels();

  // Make both boot animations eligible to draw immediately. Without this,
  // their shared timers can add up to 40 ms after the Activate edge.
  previousPowerCellMillis = now - BOOT_POWER_CELL_INTERVAL;
  previousBootMillis = now - BOOT_CYCLOTRON_SLOW_INTERVAL;
}

void setCyclotronStep(uint8_t step, uint8_t red, uint8_t green, uint8_t blue) {
  cyclotron.clear();
  const uint8_t jewelStart = (step % CYCLOTRON_JEWEL_COUNT) * PIXELS_PER_CYCLOTRON_JEWEL;
  for (uint8_t pixel = jewelStart; pixel < jewelStart + PIXELS_PER_CYCLOTRON_JEWEL; ++pixel) {
    cyclotron.setPixelColor(pixel, cyclotron.Color(red, green, blue));
  }
  cyclotron.show();
}

void drawPowerCell(uint8_t litCells, uint8_t red, uint8_t green, uint8_t blue) {
  for (uint8_t i = 0; i < ANIMATED_POWER_CELLS; ++i) {
    powerCell.setPixelColor(i, i < litCells ? powerCell.Color(red, green, blue) : 0);
  }
  // The original SpiritMinimal chain has one unused 16th pixel.
  powerCell.setPixelColor(POWER_CELL_COUNT - 1, 0);
  powerCell.show();
}

void drawHoldingPowerCell(uint8_t brightness) {
  powerCell.clear();
  powerCell.setPixelColor(BOTTOM_POWER_CELL, powerCell.Color(0, 0, brightness));
  powerCell.show();
}

void drawHoldingCyclotronPair(uint8_t pair) {
  cyclotron.clear();
  // Alternate opposing pairs: cells 0 + 2, then cells 1 + 3.
  cyclotron.setPixelColor(pair == 0 ? 0 : 1, cyclotron.Color(255, 106, 0));
  cyclotron.setPixelColor(pair == 0 ? 2 : 3, cyclotron.Color(255, 106, 0));
  cyclotron.show();
}

void drawFadedCyclotron(uint8_t step, bool usePairs,
                         uint8_t red, uint8_t green, uint8_t blue,
                         uint8_t brightness) {
  cyclotron.clear();
  const uint8_t fadedRed = (uint16_t)red * brightness / 255;
  const uint8_t fadedGreen = (uint16_t)green * brightness / 255;
  const uint8_t fadedBlue = (uint16_t)blue * brightness / 255;
  const uint32_t color = cyclotron.Color(fadedRed, fadedGreen, fadedBlue);

  if (usePairs) {
    // With two installed jewels both pulse together. With four, alternate
    // opposing pairs: 0 + 2, then 1 + 3.
    const uint8_t firstJewel = CYCLOTRON_JEWEL_COUNT == 2 ? 0 : ((step % 2) == 0 ? 0 : 1);
    const uint8_t secondJewel = CYCLOTRON_JEWEL_COUNT == 2 ? 1 : ((step % 2) == 0 ? 2 : 3);
    const uint8_t jewelStride = CYCLOTRON_JEWEL_COUNT == 2 ? 1 : 2;
    for (uint8_t jewel = firstJewel; jewel <= secondJewel; jewel += jewelStride) {
      const uint8_t jewelStart = jewel * PIXELS_PER_CYCLOTRON_JEWEL;
      for (uint8_t pixel = jewelStart; pixel < jewelStart + PIXELS_PER_CYCLOTRON_JEWEL; ++pixel) {
        cyclotron.setPixelColor(pixel, color);
      }
    }
  } else {
    const uint8_t jewelStart = (step % CYCLOTRON_JEWEL_COUNT) * PIXELS_PER_CYCLOTRON_JEWEL;
    for (uint8_t pixel = jewelStart; pixel < jewelStart + PIXELS_PER_CYCLOTRON_JEWEL; ++pixel) {
      cyclotron.setPixelColor(pixel, color);
    }
  }
  cyclotron.show();
}

// Each sequence step fades its assigned light(s) up, then back down.
// A shorter step duration naturally produces a faster firing fade.
void runCyclotronFade(unsigned long now, unsigned long stepDuration,
                      bool usePairs, uint8_t red, uint8_t green, uint8_t blue) {
  unsigned long elapsed = now - previousCyclotronMillis;
  if (elapsed >= stepDuration) {
    previousCyclotronMillis = now;
    elapsed = 0;
    ++cyclotronStep;
  }

  if (!timeElapsed(now, previousCyclotronFadeFrameMillis, CYCLOTRON_FADE_FRAME_INTERVAL)) {
    return;
  }

  const unsigned long halfStep = stepDuration / 2;
  uint8_t brightness;
  if (elapsed <= halfStep) {
    brightness = (uint32_t)elapsed * 255 / halfStep;
  } else {
    brightness = (uint32_t)(stepDuration - elapsed) * 255 / halfStep;
  }
  drawFadedCyclotron(cyclotronStep, usePairs, red, green, blue, brightness);
}

void runBoot(unsigned long now) {
  if (bootPowerCell < ANIMATED_POWER_CELLS &&
      timeElapsed(now, previousPowerCellMillis, BOOT_POWER_CELL_INTERVAL)) {
    // Original SpiritMinimal boot: a single light sweeps down the strip,
    // then stays on as each successive cell is reached.
    if (bootPowerCell == bootSweepCell) {
      if (bootSweepCell + 1 < ANIMATED_POWER_CELLS) {
        powerCell.setPixelColor(bootSweepCell + 1, 0);
      }
      powerCell.setPixelColor(bootPowerCell, powerCell.Color(0, 0, 150));
      bootSweepCell = ANIMATED_POWER_CELLS - 1;
      ++bootPowerCell;
    } else {
      if (bootSweepCell + 1 < ANIMATED_POWER_CELLS) {
        powerCell.setPixelColor(bootSweepCell + 1, 0);
      }
      powerCell.setPixelColor(bootSweepCell, powerCell.Color(0, 0, 150));
      --bootSweepCell;
    }
    powerCell.setPixelColor(POWER_CELL_COUNT - 1, 0);
    powerCell.show();
  }

  // Start with a slow cyclotron pulse and accelerate it as cells lock in.
  const unsigned long bootCyclotronInterval = BOOT_CYCLOTRON_SLOW_INTERVAL
      - (unsigned long)bootPowerCell *
          (BOOT_CYCLOTRON_SLOW_INTERVAL - BOOT_CYCLOTRON_FAST_INTERVAL) /
          ANIMATED_POWER_CELLS;
  if (timeElapsed(now, previousBootMillis, bootCyclotronInterval)) {
    // The cyclotron breathes red for the entire power-cell charging sequence.
    bootCyclotronBrightness += bootCyclotronDirection;
    if (bootCyclotronBrightness >= 255) {
      bootCyclotronBrightness = 255;
      bootCyclotronDirection = -5;
    } else if (bootCyclotronBrightness <= 0) {
      bootCyclotronBrightness = 0;
      bootCyclotronDirection = 5;
    }
    for (uint8_t i = 0; i < CYCLOTRON_COUNT; ++i) {
      cyclotron.setPixelColor(i, cyclotron.Color(bootCyclotronBrightness, 0, 0));
    }
    cyclotron.show();
  }

  if (bootPowerCell == ANIMATED_POWER_CELLS) {
    // Let the active animation take over immediately after the final cell locks in.
    previousPowerCellMillis = 0;
    previousCyclotronMillis = 0;
    idleTrackWasPlaying = false;
    queueAudio(idleTrack);
    packState = ACTIVE;
  }
}

// The deactivated pack's holding pattern.
void runHoldingPattern(unsigned long now) {
  if (!holdingPowerCellDrawn) {
    holdingPowerCellBrightness = HOLDING_POWER_CELL_MIN;
    holdingPowerCellDirection = 2;
    previousHoldingPowerCellMillis = now;
    drawHoldingPowerCell(holdingPowerCellBrightness);
    holdingPowerCellDrawn = true;
  }
  if (timeElapsed(now, previousHoldingPowerCellMillis, HOLDING_POWER_CELL_FADE_INTERVAL)) {
    int16_t nextBrightness = holdingPowerCellBrightness + holdingPowerCellDirection;
    if (nextBrightness >= HOLDING_POWER_CELL_MAX) {
      nextBrightness = HOLDING_POWER_CELL_MAX;
      holdingPowerCellDirection = -2;
    } else if (nextBrightness <= HOLDING_POWER_CELL_MIN) {
      nextBrightness = HOLDING_POWER_CELL_MIN;
      holdingPowerCellDirection = 2;
    }
    holdingPowerCellBrightness = nextBrightness;
    drawHoldingPowerCell(holdingPowerCellBrightness);
  }
  runCyclotronFade(now, IDLE_CYCLOTRON_INTERVAL, true, 255, 106, 0);
}

// Normal animation after Activate is engaged and boot has completed.
void runActive(unsigned long now) {
  holdingPowerCellDrawn = false;
  // ACT is LOW during playback. Restart the idle track only after observing a
  // complete playing-to-stopped transition, avoiding rapid Play retriggers.
  if (SFX_ACT_CONNECTED && REPLAY_IDLE_TRACK) {
    const bool audioPlaying = digitalRead(SFX_ACT_PIN) == LOW;
    if (audioPlaying) {
      idleTrackWasPlaying = true;
    } else if (idleTrackWasPlaying && pendingAudioTrack == NULL) {
      idleTrackWasPlaying = false;
      queueAudio(idleTrack, false);
    }
  }
  if (timeElapsed(now, previousPowerCellMillis, 55)) {
    drawPowerCell(powerCellStep + 1, 0, 0, 150);
    powerCellStep = (powerCellStep + 1) % ANIMATED_POWER_CELLS;
  }
  runCyclotronFade(now, 770, false, 255, 0, 0);
}

void runFiring(unsigned long now) {
  holdingPowerCellDrawn = false;
  // Fast blue power-cell sweep, alternating with white: a visible firing effect.
  if (timeElapsed(now, previousPowerCellMillis, FIRE_POWER_CELL_INTERVAL)) {
    uint8_t blue = (fireFlashStep & 1) ? 255 : 120;
    uint8_t red = (fireFlashStep & 1) ? 90 : 0;
    uint8_t green = (fireFlashStep & 1) ? 90 : 0;
    drawPowerCell(powerCellStep + 1, red, green, blue);
    powerCellStep = (powerCellStep + 1) % ANIMATED_POWER_CELLS;
  }

  // Same fade shape as active, compressed to the firing sequence speed.
  runCyclotronFade(now, FIRE_CYCLOTRON_INTERVAL, false, 255, 35, 0);
  if (timeElapsed(now, previousFireFlashMillis, FIRE_FLASH_INTERVAL)) {
    ++fireFlashStep;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(ACTIVATE_SWITCH_PIN, INPUT);
  pinMode(FIRE_SWITCH_PIN, INPUT);
  controlCandidate = readControlPattern();
  stableControls = controlCandidate;
  controlCandidateSince = millis();
  pinMode(VOLTAGE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(VOLTAGE_SENSE_PIN, INPUT);
  analogReference(DEFAULT);
  Wire.begin();
  wandCurrentSensorReady = wandCurrentSensor.begin();
  if (wandCurrentSensorReady) {
    wandCurrentMa = wandCurrentSensor.getCurrent_mA();
    wandPowered = currentMagnitude(wandCurrentMa) >= WAND_ON_CURRENT_MA;
    wandPowerCandidate = wandPowered;
    wandPowerCandidateSince = millis();
    Serial.print(F("INA219 ready; initial current="));
    Serial.print(wandCurrentMa, 2);
    Serial.println(F(" mA"));
  } else {
    Serial.println(F("INA219 not detected at I2C address 0x40; wand held off"));
  }
  pinMode(SFX_ACT_PIN, INPUT_PULLUP);
  soundSerial.begin(9600);
  // Successful replies return immediately; this only caps a missing reply.
  soundSerial.setTimeout(150);
  // Ensure the Audio FX board has completed its serial boot before the first
  // activation event can request a track.
  soundBoard.reset();

  powerCell.begin();
  powerCell.setBrightness(80);
  cyclotron.begin();
  cyclotron.setBrightness(80);
  clearPixels();
}

void loop() {
  const unsigned long now = millis();
  updateHeartbeat(now);
  // Read and debounce the controls first so updateWandPower() can validate a
  // possible Fire command against a fresh INA219 reading in this same loop.
  const uint8_t controls = updateControlPattern(now);
  updateVoltageButton(now);
  updateWandPower(now);

  // Current sensing is authoritative for wand power. This removes the
  // LOW/LOW ambiguity from D4/D5 and also permits shutdown while firing.
  if (!wandCurrentSensorReady || !wandPowered) {
    activationTransitionBlockActive = false;
    fireCommandPending = false;
    deactivatePending = false;
    wandControlsArmed = false;
    if (packState == HOLDING) {
      runHoldingPattern(now);
    } else {
      if (packState != SHUTTING_DOWN) {
        beginShutdown(now);
        packState = SHUTTING_DOWN;
      }
      runShutdown(now);
    }
    reportPackState(now);
    serviceAudio(millis());
    return;
  }

  switch (packState) {
    case HOLDING:
      deactivatePending = false;
      runHoldingPattern(now);

      // Current means wand power is available. LOW/LOW arms the controls; the
      // following HIGH/LOW transition is the Activate command.
      if (controls == WAND_READY_PATTERN) {
        if (!wandControlsArmed && DEBUG_CONTROL_INPUTS) {
          Serial.print(now);
          Serial.println(F(" ms INPUT  activation control armed"));
        }
        wandControlsArmed = true;
      } else if (wandControlsArmed && controls == WAND_ACTIVE_PATTERN) {
        wandControlsArmed = false;
        beginActivationTransitionBlock(now);
        beginBoot(now);
        packState = BOOTING;
        // Render the first boot frame on the same pass that changes state.
        runBoot(now);
      }
      break;

    case BOOTING:
      if (activationTransitionBlocked(now)) {
        runBoot(now);
      } else if (controls == WAND_ACTIVE_PATTERN) {
        deactivatePending = false;
        runBoot(now);
      } else if (controls == WAND_FIRE_PATTERN) {
        // Ignore Fire until boot completes. If HIGH/HIGH remains held when
        // runBoot() enters ACTIVE, the next loop begins firing immediately.
        deactivatePending = false;
        runBoot(now);
      } else {
        // A mixed HIGH/LOW pattern is invalid after the transition block.
        if (activationReleaseConfirmed(now, controls)) {
          deactivatePending = false;
          beginShutdown(now);
          packState = SHUTTING_DOWN;
          runShutdown(now);
        } else {
          runBoot(now);
        }
      }
      break;

    case ACTIVE:
      if (activationTransitionBlocked(now)) {
        fireCommandPending = false;
        runActive(now);
      } else if (controls == WAND_ACTIVE_PATTERN) {
        fireCommandPending = false;
        deactivatePending = false;
        runActive(now);
      } else if (controls == WAND_FIRE_PATTERN) {
        deactivatePending = false;
        if (!fireCommandPending) {
          fireCommandPending = true;
          fireCommandSince = now;
          runActive(now);
        } else if (now - fireCommandSince >= FIRE_COMMAND_QUALIFY_INTERVAL) {
          fireCommandPending = false;
          queueAudio(blastTrack);
          packState = FIRING;
          runFiring(now);
        } else {
          runActive(now);
        }
      } else {
        fireCommandPending = false;
        if (activationReleaseConfirmed(now, controls)) {
          deactivatePending = false;
          beginShutdown(now);
          packState = SHUTTING_DOWN;
          runShutdown(now);
        } else {
          runActive(now);
        }
      }
      break;

    case FIRING:
      fireCommandPending = false;
      if (controls == WAND_FIRE_PATTERN) {
        deactivatePending = false;
        runFiring(now);
      } else if (controls == WAND_ACTIVE_PATTERN) {
        deactivatePending = false;
        queueAudio(fireEndTrack);
        packState = ACTIVE;
        runActive(now);
      } else {
        if (activationReleaseConfirmed(now, controls)) {
          deactivatePending = false;
          beginShutdown(now);
          packState = SHUTTING_DOWN;
          runShutdown(now);
        } else {
          runFiring(now);
        }
      }
      break;

    case SHUTTING_DOWN:
      // Finish shutdown before another activation is allowed.
      fireCommandPending = false;
      deactivatePending = false;
      activationTransitionBlockActive = false;
      wandControlsArmed = false;
      runShutdown(now);
      break;
  }

  // Audio is intentionally serviced last so it cannot delay switch detection
  // or the first lighting frame of a transition.
  reportPackState(now);
  serviceAudio(millis());
}
