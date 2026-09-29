#pragma once
#include <Arduino.h>

struct SegmentPosition { uint8_t row; uint8_t bit; };
constexpr uint8_t SEGMENT_COUNT = 28;
constexpr bool SEGMENT_MAP_VERIFIED = true;

// User-reported physical sequence: cycle COM0..3, then advance ROW0..6.
// row = buffer word / COM; bit = electrical ROW output (anode).
// Physical sequence confirmed by user with test; ancillary wiring records remain pending.
constexpr SegmentPosition SEGMENT_MAP[SEGMENT_COUNT] = {
  {0,0}, {1,0}, {2,0}, {3,0},
  {0,1}, {1,1}, {2,1}, {3,1},
  {0,2}, {1,2}, {2,2}, {3,2},
  {0,3}, {1,3}, {2,3}, {3,3},
  {0,4}, {1,4}, {2,4}, {3,4},
  {0,5}, {1,5}, {2,5}, {3,5},
  {0,6}, {1,6}, {2,6}, {3,6}
};

inline bool validSegmentMap() {
  for (uint8_t i = 0; i < SEGMENT_COUNT; ++i) {
    if (SEGMENT_MAP[i].row >= 4 || SEGMENT_MAP[i].bit >= 7) return false;
    for (uint8_t j = 0; j < i; ++j)
      if (SEGMENT_MAP[i].row == SEGMENT_MAP[j].row &&
          SEGMENT_MAP[i].bit == SEGMENT_MAP[j].bit) return false;
  }
  return true;
}
