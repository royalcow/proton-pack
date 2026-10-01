#pragma once
#include <stdint.h>

class ToggleInput {
 public:
  void begin(bool on, uint32_t now) { raw_ = stable_ = on; changed_ = now; }
  // -1: no event, 0: switched OFF, 1: switched ON.
  int8_t update(bool on, uint32_t now) {
    if (on != raw_) { raw_ = on; changed_ = now; }
    if (on == stable_ || uint32_t(now - changed_) < 25) return -1;
    stable_ = on;
    return on ? 1 : 0;
  }
 private:
  bool raw_ = false, stable_ = false;
  uint32_t changed_ = 0;
};
