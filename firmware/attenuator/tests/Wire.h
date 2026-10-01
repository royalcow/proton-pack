#pragma once
#include <stdint.h>
#include <vector>
struct TwoWire {
 bool getWireTimeoutFlag() { return false; }
 uint32_t timeoutUs=0;
 bool resetOnTimeout=false;
 void setWireTimeout(uint32_t us, bool reset) { timeoutUs=us; resetOnTimeout=reset; }
 bool connected=true;
 bool enabled=true;
 uint8_t address=0;
 std::vector<uint8_t> bytes, ram;
 void begin() {}
 void setClock(int) {}
 void beginTransmission(uint8_t a) { address=a; bytes.clear(); }
 void write(uint8_t b) { bytes.push_back(b); }
 uint8_t endTransmission() {
   if (!connected || address!=0x70) return 2;
   if (bytes.size()==1 && (bytes[0]==0x80 || bytes[0]==0x81)) enabled=bytes[0]==0x81;
   if (bytes.size()==17) ram=bytes;
   return 0;
 }
};
extern TwoWire Wire;
