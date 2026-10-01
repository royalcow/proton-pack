#pragma once
#include <stdint.h>

// Pure logical-frame renderer. No driver, input, volume, or mute ownership.
class VentAnimation {
 public:
  enum Phase { VENT_BUILDUP, VENT_CHATTER, VENT_DUMP, VENT_RESIDUAL, VENT_COMPLETE };
  struct Frame { Phase phase; uint32_t image; };
  static constexpr uint16_t DURATION_MS = 1510;
  void begin(uint32_t now, uint32_t image) { started_ = now; initial_ = image & fill(28); }
  Frame render(uint32_t now) const {
    uint32_t elapsed = uint32_t(now - started_);
    if (elapsed < 180) {
      uint8_t step = elapsed / 36; // Original image, then four fills to full.
      return {VENT_BUILDUP, initial_ | fill(step * 7)};
    }
    elapsed -= 180;
    if (elapsed < 250) {
      static const uint8_t duration[] = {45,35,60,45,65};
      static const uint32_t image[] = {fill(28),fill(25),fill(28),fill(28)&~fill(3),fill(28)};
      return {VENT_CHATTER, image[index(elapsed,duration,5)]};
    }
    elapsed -= 250;
    if (elapsed < 700) {
      static const uint8_t duration[] = {60,45,65,40,55,75,45,80,60,70,55,50};
      static const uint8_t remaining[] = {28,25,23,25,19,18,14,15,10,7,3,0};
      return {VENT_DUMP, fill(remaining[index(elapsed,duration,12)])};
    }
    elapsed -= 700;
    if (elapsed < 380) {
      // Three isolated low-end pulses over 300 ms, then 80 ms explicitly dark.
      static const uint8_t duration[] = {45,55,40,65,35,60,80};
      static const uint8_t image[] = {1,0,12,0,2,0,0};
      return {VENT_RESIDUAL, image[index(elapsed,duration,7)]};
    }
    return {VENT_COMPLETE,0};
  }
 private:
  static constexpr uint32_t fill(uint8_t n) { return (uint32_t(1)<<n)-1; }
  static uint8_t index(uint16_t elapsed, const uint8_t *durations, uint8_t count) {
    for (uint8_t i=0;i<count-1;++i) {
      if (elapsed<durations[i]) return i;
      elapsed-=durations[i];
    }
    return count-1;
  }
  uint32_t started_=0, initial_=0;
};
