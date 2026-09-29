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
 void begin(int) {}
 int available() { return incoming.size(); }
 int read() { char c=incoming.front(); incoming.erase(0,1); return c; }
 template<class T> void print(T) {}
 template<class T> void println(T) {}
 template<class T> void println(T,int) {}
};
extern SerialMock Serial;
