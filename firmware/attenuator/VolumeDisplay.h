#pragma once
#include <stdint.h>

// Pure display-state renderer: no Serial, Wire, mapping, or audio ownership.
// Call setters only with confirmed state; the POC simulates it via Serial.
class VolumeDisplay {
 public:
  struct Frame { uint32_t image; uint8_t brightness; };
  uint8_t volume() const { return volume_; }
  bool muted() const { return muted_; }
  void setVolume(uint8_t value) { volume_ = value > 100 ? 100 : value; }
  void setMuted(bool value, uint32_t now) {
    if (value == muted_) return;
    muted_ = value;
    transition_ = true;
    started_ = now;
    breathStarted_ = now + 250;
  }
  static uint32_t fill(uint8_t count) { return (uint32_t(1) << count) - 1; }
  uint8_t count() const { return (uint16_t(volume_) * 28 + 50) / 100; }
  uint8_t markerStart() const {
    // Keep the pair inside the existing filled bar, at its upper edge.
    uint8_t lit = count();
    return lit >= 2 ? lit - 2 : 0;
  }
  Frame render(uint32_t now, uint8_t cap) {
    uint8_t countNow = count();
    uint32_t bar = fill(countNow);
    uint32_t pair = uint32_t(3) << markerStart();
    uint32_t elapsed = uint32_t(now - started_);
    if (transition_ && elapsed >= 250) transition_ = false;
    uint32_t image = muted_ ? pair : bar;
    uint8_t level = cap;
    if (transition_) {
      uint8_t span = markerStart();
      uint8_t step = (elapsed * span) / 250;
      // Keep the saved-volume pair fixed. Drain high-to-low beneath it;
      // refill low-to-high on unmute.
      image = pair | fill(muted_ ? span - step : step);
    } else if (muted_ && cap > 3) {
      uint16_t phase = uint32_t(now - breathStarted_) % 2500;
      uint16_t ramp = phase <= 1250 ? phase : 2500 - phase;
      // Smoothstep easing, fixed point. HT16K33 has 16 global levels.
      uint32_t x = uint32_t(ramp) * 1000 / 1250;
      uint32_t ease = x * x * (3000 - 2 * x) / 1000000;
      level = 3 + ((cap - 3) * ease + 500) / 1000;
    }
    return {image, level};
  }
 private:
  uint8_t volume_ = 50;
  bool muted_ = false;
  bool transition_ = false;
  uint32_t started_ = 0;
  uint32_t breathStarted_ = 0;
};
