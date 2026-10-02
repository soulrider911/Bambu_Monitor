#include "../../Bambu_Monitor_ESP32/filament_metadata.h"
#include <cassert>
#include <cstring>
int main() {
    const char* xml = "<root><plate><metadata key=\"index\" value=\"4\"/>"
        "<filament type=\"PLA\" used_g=\"12.5\"/><filament type=\"PLA\" used_g=\"11.5\"/>"
        "<filament type=\"PETG\" used_g=\"0\"/></plate>"
        "<plate><metadata key=\"index\" value=\"7\"/><filament type=\"TPU\" used_g=\"29.92\"/></plate></root>";
    FilamentEstimate e;
    assert(parseFilamentEstimate(xml, strlen(xml), 4, e));
    assert(e.count == 2 && e.material == "PLA" && e.grams == 24);
    assert(parseFilamentEstimate(xml, strlen(xml), 7, e));
    assert(e.count == 1 && e.material == "TPU" && fabs(e.grams - 29.92f) < .001f);
    assert(!parseFilamentEstimate(xml, strlen(xml), 1, e));
    assert(!parseFilamentEstimate(xml, 20, 4, e));
    assert(!parseFilamentEstimate(xml, 25000, 4, e));
    const char* mixed = "<plate><metadata key='index' value='1'/><filament type='PLA' used_g='10'/><filament type='PETG' used_g='2'/></plate>";
    assert(parseFilamentEstimate(mixed, strlen(mixed), 1, e) && e.material == "Mix" && e.grams == 12);
}
