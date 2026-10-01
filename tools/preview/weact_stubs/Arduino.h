#pragma once
#include "../stubs/Arduino.h"
#include <cstdlib>
#define PROGMEM
using boolean = bool;
class __FlashStringHelper;
inline void yield() {}
inline void delay(unsigned long) {}

inline float radians(float degrees) { return degrees * 0.017453292519943295f; }
