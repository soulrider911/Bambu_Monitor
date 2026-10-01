#pragma once
#include <Adafruit_GFX.h>
#define GxEPD_BLACK 0
#define GxEPD_WHITE 65535
#define GxEPD_RED 63488
struct GxEPD2_290_C90c {
    static const int HEIGHT = 296;
    GxEPD2_290_C90c(int, int, int, int) {}
};
template<class Driver, int Height> class GxEPD2_3C : public GFXcanvas16 {
public:
    GxEPD2_3C(Driver) : GFXcanvas16(296, 128) {}
};
