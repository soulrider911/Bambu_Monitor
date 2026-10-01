#pragma once
#include "Arduino.h"
class Print {
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t c) = 0;
    size_t print(const String& s) { for (auto c : s) write(c); return s.length(); }
    size_t print(const char* s) { return print(String(s)); }
};
