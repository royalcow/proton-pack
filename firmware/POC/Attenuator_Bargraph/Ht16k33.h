#pragma once
#include <Arduino.h>
#include <Wire.h>

// Raw 8 x 16-bit buffer: word index = COM, bit index = electrical ROW.
// No display-specific ordering, Serial, animation, or Wire.begin() here.
class Ht16k33 {
 public:
  explicit Ht16k33(TwoWire &bus = Wire) : bus_(bus) {}
  bool begin(uint8_t address, uint8_t brightness) {
    address_ = address;
    clear();
    return probe(address_) == 0 && command(0x21) && command(0x80) &&
           write() && setBrightness(brightness) && command(0x81);
  }
  uint8_t probe(uint8_t address) {
    bus_.beginTransmission(address);
    return bus_.endTransmission();
  }
  void clear() {
    for (uint8_t i = 0; i < 8; ++i) buffer_[i] = 0;
  }
  void set(uint8_t row, uint8_t bit) {
    if (row < 8 && bit < 16) buffer_[row] |= uint16_t(1) << bit;
  }
  bool write() {
    bus_.beginTransmission(address_);
    bus_.write(uint8_t(0x00));
    for (uint8_t i = 0; i < 8; ++i) {
      bus_.write(uint8_t(buffer_[i]));
      bus_.write(uint8_t(buffer_[i] >> 8));
    }
    return bus_.endTransmission() == 0;
  }
  bool setBrightness(uint8_t value) {
    return value <= 15 && command(0xE0 | value);
  }
 private:
  bool command(uint8_t value) {
    bus_.beginTransmission(address_);
    bus_.write(value);
    return bus_.endTransmission() == 0;
  }
  TwoWire &bus_;
  uint8_t address_ = 0x70;
  uint16_t buffer_[8] = {};
};
