#include "../../Bambu_Monitor_ESP32/thumbnail_identity.h"
#include <cassert>
int main() {
    auto matches = [](const char* xml, const char* job) {
        return thumbnailMetadataMatches(xml, strlen(xml), job);
    };
    assert(matches("<metadata name=\"ProfileTitle\">Includes optimised plates for PLA PETG ABS ASA and PC</metadata>", "Includes optimised plates for PLA PETG ABS ASA and PC"));
    assert(matches("<metadata name=\"ProfileTitle\">1 set 4 Pieces (New version) </metadata>", "1 set 4 Pieces (New version)"));
    assert(matches("<metadata name='Title'>Clips &amp; Hooks</metadata>", "Clips & Hooks"));
    assert(!matches("<metadata name=\"Description\">Clips</metadata>", "Clips"));
    assert(!matches("<metadata name=\"ProfileTitle\">Clips XL</metadata>", "Clips"));
    assert(!matches("<metadata name=\"Title\">Clips", "Clips"));
    assert(!matches("<metadata name=\"Title\"></metadata>", ""));
    const char xml[] = "<metadata name=\"Title\">Clips</metadata>";
    for (size_t n = 0; n < strlen(xml); ++n)
        assert(!thumbnailMetadataMatches(xml, n, "Clips"));
}
