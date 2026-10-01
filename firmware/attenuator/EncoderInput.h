#pragma once
#include <stdint.h>

// Hardware-independent contact decoding; sample rotation on both phase edges.
class EncoderInput {
 public:
  void begin(uint8_t phases, bool pressed, uint32_t now) {
    previous_ = phases;
    quarters_ = 0;
    rawPressed_ = stablePressed_ = pressed;
    changed_ = now;
  }
  int8_t rotate(uint8_t phases) {
    static const int8_t steps[16] = {
      0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0
    };
    uint8_t old = previous_;
    previous_ = phases;
    if ((old ^ phases) == 3) { quarters_ = 0; return 0; }
    quarters_ += steps[(old << 2) | phases];
    // A complete cycle returns to both pull-ups high. Contact bounce cancels.
    if (phases != 3) return 0;
    int8_t result = quarters_ >= 4 ? 1 : quarters_ <= -4 ? -1 : 0;
    quarters_ = 0;
    return result;
  }
  bool button(bool pressed, uint32_t now) {
    if (pressed != rawPressed_) { rawPressed_ = pressed; changed_ = now; }
    if (pressed == stablePressed_ || uint32_t(now - changed_) < 25) return false;
    stablePressed_ = pressed;
    return pressed;
  }
 private:
  uint8_t previous_ = 3;
  int8_t quarters_ = 0;
  bool rawPressed_ = false, stablePressed_ = false;
  uint32_t changed_ = 0;
};
