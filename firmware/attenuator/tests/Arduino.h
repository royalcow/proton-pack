#pragma once
#include <stdint.h>
#include <stddef.h>
#include <algorithm>
#include <string>
using std::max;
#define F(x) x
#define HEX 16
extern uint32_t clockMs;
inline uint32_t millis() { return clockMs; }
struct SerialMock {
 std::string incoming;
 int txSpace=64;
 int availableForWrite() { return txSpace; }
 void begin(int) {}
 int available() { return incoming.size(); }
 int read() { char c=incoming.front(); incoming.erase(0,1); return c; }
 template<class T> void print(T) {}
 template<class T> void println(T) {}
 template<class T> void println(T,int) {}
};
extern SerialMock Serial;

#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 2
#define CHANGE 3
static int inputPins[5] = {1,1,1,1,1};
inline int digitalRead(int pin) { return inputPins[pin]; }
inline void pinMode(int, int) {}
inline int digitalPinToInterrupt(int pin) { return pin; }
inline void attachInterrupt(int, void (*)(), int) {}
inline void noInterrupts() {}
inline void interrupts() {}
