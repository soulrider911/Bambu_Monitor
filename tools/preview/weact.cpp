#include "../../Bambu_Monitor_ESP32/weact_completion.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#include <cassert>
#include "../../Bambu_Monitor_ESP32/screen_weact.h"

void save(const char* name) {
    std::ofstream out(std::string("out/weact/") + name + ".ppm", std::ios::binary);
    out << "P6\n296 128\n255\n";
    for (int i = 0; i < 296 * 128; ++i) {
        uint16_t c = weact.getBuffer()[i];
        const int x = i % 296, y = i / 296;
        if (x >= 4 && x < 4 + PREVIEW_W && y >= 23 && y < 23 + PREVIEW_H)
            assert(c != GxEPD_RED); // Thumbnails must never use the accent plane.
        unsigned char rgb[] = {(unsigned char)(c == GxEPD_BLACK ? 0 : 255),
            (unsigned char)(c == GxEPD_WHITE ? 255 : 0), (unsigned char)(c == GxEPD_WHITE ? 255 : 0)};
        out.write((char*)rgb, 3);
    }
}
int main(int argc, char** argv) {
    assert(argc == 2);
    // Brighter gamma lifts midtones without reversing the shade ordering.
    assert(previewTone(80, 0, 239, 2.2f, 1.0f) > previewTone(80, 0, 239, 1.0f, 1.0f));
    assert(previewTone(255, 0, 239) == 255);
    for (int value = 1; value < 245; ++value)
        assert(previewTone(value, 0, 239) >= previewTone(value - 1, 0, 239));
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<uint8_t> png((std::istreambuf_iterator<char>(in)), {});
    assert(decodeWeactPreview(png.data(), png.size()));
    auto valid = png;
    assert(!decodeWeactPreview(png.data(), 20));
    assert(!decodeWeactPreview(png.data(), png.size() - 12));
    png[16] = 127;
    assert(!decodeWeactPreview(png.data(), png.size()));
    png = valid; png[0] = 0;
    assert(!decodeWeactPreview(png.data(), png.size()));
    assert(decodeWeactPreview(valid.data(), valid.size()));
    WeactCompletion completion;
    PrinterStatus live;
    live.state = "PRINTING"; completion.observe(live);
    live.state = "FINISHED"; completion.observe(live);
    assert(completion.displayStatus(live).state == "FINISHED");
    assert(!completion.tick(90000, 60000)); // no timeout before rendering
    live.state = "IDLE"; completion.observe(live);
    assert(completion.displayStatus(live).state == "FINISHED");
    completion.displayed(90000);
    assert(!completion.tick(149999, 60000));
    assert(completion.tick(150000, 60000));
    assert(completion.displayStatus(live).state == "IDLE");
    live.state = "FINISHED"; completion.observe(live);
    assert(completion.displayStatus(live).state == "IDLE"); // repeated reports cannot restart hold
    live.state = "PRINTING"; completion.observe(live);
    assert(completion.displayStatus(live).state == "PRINTING");
    live.state = "FINISHED"; completion.observe(live);
    completion.displayed(160000);
    live.state = "PREPARING"; completion.observe(live);
    assert(!completion.tick(230000, 60000));
    assert(completion.displayStatus(live).state == "PREPARING");
    PrinterStatus s;
    s.printerName = "Bambu Lab X2D"; s.state = "PRINTING"; s.jobName = "Mini Turtle";
    s.progress = 67; s.layer = 142; s.totalLayers = 380; s.remainingMinutes = 134;
    s.chamberTemp = 42; s.bedTemp = 60; s.leftNozzleTemp = 220; s.rightNozzleTemp = 38;
    s.amsTemp = 28; s.amsHumidityRaw = 18;
    s.filamentMaterial = "PLA"; s.filamentGrams = 24; s.filamentCount = 1;
    String signature = weactStatusSignature(s);
    s.stage = 42; s.lastMessage = 12345; s.amsHumidity = 3;
    assert(weactStatusSignature(s) == signature); // not visible while raw humidity is available
    s.bedTemp = 60.2f;
    assert(weactStatusSignature(s) == signature);
    s.bedTemp = 60.8f;
    assert(weactStatusSignature(s) != signature);
    s.bedTemp = 60;
    drawWeactScreen(s, true, false); save("printing");
    assert(decodeWeactPreview(valid.data(), valid.size(), PreviewDither::FloydSteinberg));
    drawWeactScreen(s, true, false); save("dither_floyd_steinberg");
    assert(decodeWeactPreview(valid.data(), valid.size(), PreviewDither::Atkinson));
    drawWeactScreen(s, true, false); save("dither_atkinson");
    assert(decodeWeactPreview(valid.data(), valid.size()));
    drawWeactScreen(s, false, true); save("no_preview");
    s.jobName = "An exceptionally long job name that should be truncated";
    s.filamentCount = 2;
    s.state = "PAUSED"; drawWeactScreen(s, true, false); save("paused");
    s.state = "FINISHED"; s.progress = 100; s.remainingMinutes = 0;
    drawWeactScreen(s, true, false); save("finished");
    s.state = "FAILED"; drawWeactScreen(s, false, true); save("failed");
    s = PrinterStatus(); drawWeactScreen(s, false, false); save("connecting");
    s.state = "IDLE"; s.bedTemp = 23; s.rightNozzleTemp = 25;
    drawWeactScreen(s, false, false); save("idle");
    s.state = "PREPARING"; drawWeactScreen(s, false, false); save("preparing");
    puts("WeAct decoder checks passed; rendered 8 scenarios and both dither modes.");
}
