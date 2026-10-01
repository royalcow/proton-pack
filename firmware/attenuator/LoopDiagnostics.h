#pragma once
#include <stdint.h>

// Called at 100 Hz by the diagnostic timer, independently of loop()/millis().
// Byte fields are atomic on AVR. The loop sets progressed; the timer clears it.
class LoopDiagnostics {
 public:
  volatile uint8_t phase = 0; // 1 input, 2 recovery, 3 Serial, 4 animation
  volatile bool progressed = false;
  volatile uint8_t stalledPhase = 0;
  volatile uint8_t lastStall = 0;
  volatile uint8_t stalls = 0; // Saturating count
  void tick() {
    if (progressed) {
      progressed = false;
      quietTicks_ = 0;
      stalledPhase = 0;
      patternTick_ = 0;
      return;
    }
    if (quietTicks_ < 200) ++quietTicks_;
    if (quietTicks_ == 200 && stalledPhase == 0) {
      stalledPhase = phase ? phase : 5;
      lastStall = stalledPhase;
      if (stalls < 255) ++stalls;
      patternTick_ = 0;
      return;
    }
    if (stalledPhase) patternTick_ = (patternTick_ + 1) % 200;
  }
  bool faultLedOn() const {
    // 100 ms on / 100 ms off, followed by a pause; repeat every two seconds.
    return patternTick_ < uint16_t(stalledPhase) * 20 && patternTick_ % 20 < 10;
  }
 private:
  uint8_t quietTicks_ = 0;
  uint16_t patternTick_ = 0;
};
