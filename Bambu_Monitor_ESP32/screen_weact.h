#pragma once
#ifdef WEACT_HOST_PREVIEW
#include "weact_display.h"
#else
#include <GxEPD2_3C.h>
#include <epd3c/GxEPD2_290_C90c.h>
#endif
#include "font_weact_chicago_small.h"
#include "font_weact_chicago_label.h"
#include "font_weact_chicago_layer.h"
#include "font_weact_chicago_title.h"
#include "font_weact_chicago_progress.h"
#include "printer_status.h"
#include "preview_weact.h"

// EDIT DISPLAY STYLES HERE.
// font: ChicagoSmall, ChicagoLabel, ChicagoTitle, ChicagoProgress.
// Capitals are approximately 7, 10, 13, and 26 pixels high respectively.
// scale: 1 = native size, 2 = double size (not a point size).
// color: GxEPD_BLACK, GxEPD_RED, or GxEPD_WHITE.
// Larger fonts may need position/spacing changes in drawWeactScreen below.
struct WeactTextStyle {
    const GFXfont* font;
    uint8_t scale;
    uint16_t color;
};
namespace WeactStyle {
    const WeactTextStyle TITLE = {&ChicagoTitle, 1, GxEPD_BLACK};
    const WeactTextStyle REMAINING_LABEL = {&ChicagoLabel, 1, GxEPD_BLACK};
    const WeactTextStyle REMAINING_VALUE = {&ChicagoTitle, 1, GxEPD_BLACK};
    const WeactTextStyle SENSOR_LABEL = {&ChicagoSmall, 1, GxEPD_BLACK};
    const WeactTextStyle SENSOR_VALUE = {&ChicagoSmall, 1, GxEPD_BLACK};
    const uint8_t SENSOR_VALUE_EXTRA_WEIGHT = 1; // Extra stroke pixels; 0 uses the original weight.
    const WeactTextStyle JOB_NAME = {nullptr, 1, GxEPD_BLACK};
    const WeactTextStyle PROGRESS = {&ChicagoProgress, 1, GxEPD_BLACK};
    const WeactTextStyle PROGRESS_UNIT = {&ChicagoLabel, 1, GxEPD_BLACK};
    const WeactTextStyle LAYERS = {&Chicago7pt, 1, GxEPD_BLACK};
    const WeactTextStyle STATUS = {&ChicagoSmall, 1, GxEPD_WHITE};
    const uint16_t CURRENT_LAYER_COLOR = GxEPD_RED;
    const uint16_t TITLE_ACCENT = GxEPD_RED;
    const uint16_t TIME_NUMBER_COLOR = GxEPD_RED;
    const uint16_t ALERT_COLOR = GxEPD_RED;
    const uint16_t STATUS_BACKGROUND = GxEPD_BLACK;
    const uint16_t BACKGROUND = GxEPD_WHITE;
    const uint16_t LINE_COLOR = GxEPD_BLACK;
    const uint16_t PROGRESS_COLOR = GxEPD_BLACK;
    const uint16_t PREVIEW_COLOR = GxEPD_BLACK;
}

GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT> weact(GxEPD2_290_C90c(5, 17, 16, 4));

// Sanitize text for the ASCII fonts.
String compactText(const String& input, unsigned cells) {
    String text;
    for (unsigned i = 0; i < input.length(); ++i) {
        unsigned char c = input[i];
        if (c >= 32 && c < 127) text += (char)c;
        else if (c >= 192) text += '?';
    }
    if (text.length() > cells) return text.substring(0, cells - 1) + "~";
    return text;
}
// y is the top for the bitmap font, and the baseline for custom fonts.
void weactText(int x, int y, const String& text, const WeactTextStyle& style) {
    weact.setFont(style.font);
    weact.setTextSize(style.scale);
    weact.setTextColor(style.color);
    weact.setCursor(x, y);
    weact.print(text);
}
String compactTemp(float t) { return isnan(t) ? String("--") : String((int)roundf(t)); }

// right is the exclusive pixel edge, matching drawRect(x, y, width, height).
int weactRightAlignedX(int right, const String& text, const WeactTextStyle& style) {
    weact.setFont(style.font);
    weact.setTextSize(style.scale);
    int16_t left, top;
    uint16_t width, height;
    weact.getTextBounds(text.c_str(), 0, 0, &left, &top, &width, &height);
    return right - left - width;
}

String weactStatusSignature(const PrinterStatus& s) {
    return compactText(s.printerName.length() ? s.printerName : "Bambu Monitor", 29) + "|" +
        compactText(s.state, 13) + "|" + compactText(s.jobName.length() ? s.jobName : "No active job", 32) + "|" +
        String(constrain(s.progress, -1, 100)) + "|" + String(s.remainingMinutes) + "|" +
        String(s.layer) + "|" + String(s.totalLayers) + "|" + compactTemp(s.chamberTemp) + "|" +
        compactTemp(s.bedTemp) + "|" + compactTemp(s.leftNozzleTemp) + "|" + compactTemp(s.rightNozzleTemp) + "|" +
        String(s.leftNozzleTemp >= 180) + "|" + String(s.rightNozzleTemp >= 180) + "|" + compactTemp(s.amsTemp) + "|" +
        (s.amsHumidityRaw >= 0 ? String(s.amsHumidityRaw) + "%" : String("L") + String(s.amsHumidity));
}

void drawWeactScreen(const PrinterStatus& s, bool withPreview, bool attempted) {
    weact.fillScreen(WeactStyle::BACKGROUND);
    weact.setTextWrap(false);
    weactText(4, 15, "Bambu ", WeactStyle::TITLE);
    weact.setTextColor(WeactStyle::TITLE_ACCENT);
    weact.print("H2D");
    weact.setTextColor(WeactStyle::TITLE.color);
    weact.print(" Status");
    bool alert = s.state == "PAUSED" || s.state == "FAILED" || s.state == "ERROR";
    String state = compactText(s.state, 13);
    int badgeW = state.length() * 6 + 8;
    weact.fillRoundRect(292 - badgeW, 2, badgeW, 14, 3, alert ? WeactStyle::ALERT_COLOR : WeactStyle::STATUS_BACKGROUND);
    weactText(296 - badgeW, 12, state, WeactStyle::STATUS);
    weact.drawFastHLine(4, 19, 288, WeactStyle::LINE_COLOR);

    if (withPreview) weact.drawBitmap(4, 23, weactPreview, PREVIEW_W, PREVIEW_H, WeactStyle::PREVIEW_COLOR);
    else {
        // Bambu Studio logo mark, scaled to 49 x 64 pixels and centered.
        // Source: https://github.com/bambulab/BambuStudio/blob/master/resources/images/splash_logo.svg
        const int points[4][8] = {
            {26, 24, 26, 64, 49, 64, 49, 33},
            {26, 0, 26, 20, 49, 29, 49, 0},
            {0, 40, 0, 0, 23, 0, 23, 31},
            {0, 64, 0, 44, 23, 35, 23, 64}
        };
        for (const auto& p : points) {
            weact.fillTriangle(24 + p[0], 29 + p[1], 24 + p[2], 29 + p[3],
                               24 + p[4], 29 + p[5], WeactStyle::PREVIEW_COLOR);
            weact.fillTriangle(24 + p[0], 29 + p[1], 24 + p[4], 29 + p[5],
                               24 + p[6], 29 + p[7], WeactStyle::PREVIEW_COLOR);
        }
    }
    weactText(100, 24, compactText(s.jobName.length() ? s.jobName : "No active job", 32), WeactStyle::JOB_NAME);
    String percent = s.progress < 0 ? String("--") : String(constrain(s.progress, 0, 100));
    weactText(100, 66, percent, WeactStyle::PROGRESS);
    weactText(weact.getCursorX() + 2, 66, "%", WeactStyle::PROGRESS_UNIT);
    const int timeRight = 292; // Same right edge as the progress bar.
    weactText(weactRightAlignedX(timeRight, "Remaining", WeactStyle::REMAINING_LABEL),
              50, "Remaining", WeactStyle::REMAINING_LABEL);
    if (s.remainingMinutes >= 0) {
        WeactTextStyle numberStyle = WeactStyle::REMAINING_VALUE;
        numberStyle.color = WeactStyle::TIME_NUMBER_COLOR;
        String hours = String(s.remainingMinutes / 60);
        String minutes = String(s.remainingMinutes % 60);
        String remaining = hours + "h " + minutes + "m";
        weactText(weactRightAlignedX(timeRight, remaining, WeactStyle::REMAINING_VALUE),
                  66, hours, numberStyle);
        weact.setTextColor(WeactStyle::REMAINING_VALUE.color);
        weact.print("h ");
        weact.setTextColor(WeactStyle::TIME_NUMBER_COLOR);
        weact.print(minutes);
        weact.setTextColor(WeactStyle::REMAINING_VALUE.color);
        weact.print("m");
    } else {
        weactText(weactRightAlignedX(timeRight, "--", WeactStyle::REMAINING_VALUE),
                  66, "--", WeactStyle::REMAINING_VALUE);
    }
    weact.drawRect(100, 72, 192, 9, WeactStyle::LINE_COLOR);
    // Staggered black dots give the empty track a retro 25% gray appearance.
    // One pixel in each group of four; the completed portion paints over it.
    for (int y = 0; y < 5; ++y)
        for (int x = (y % 2) * 2; x < 188; x += 4)
            weact.drawPixel(102 + x, 74 + y, WeactStyle::PROGRESS_COLOR);
    if (s.progress > 0) weact.fillRect(102, 74, 188 * constrain(s.progress, 0, 100) / 100, 5, WeactStyle::PROGRESS_COLOR);
    // Baseline 95 leaves room between the progress bar and sensor separator.
    weactText(100, 95, "Layer ", WeactStyle::LAYERS);
    weact.setTextColor(s.layer < 0 ? WeactStyle::LAYERS.color : WeactStyle::CURRENT_LAYER_COLOR);
    weact.print(s.layer < 0 ? String("--") : String(s.layer));
    weact.setTextColor(WeactStyle::LAYERS.color);
    weact.print("/");
    weact.print(s.totalLayers < 0 ? String("--") : String(s.totalLayers));
    weact.drawFastHLine(4, 102, 288, WeactStyle::LINE_COLOR);
    const char* labels[] = {"CHAMB", "BED", "NOZ L", "NOZ R", "AMS", "HUM"};
    String values[] = {compactTemp(s.chamberTemp), compactTemp(s.bedTemp), compactTemp(s.leftNozzleTemp),
        compactTemp(s.rightNozzleTemp), compactTemp(s.amsTemp),
        s.amsHumidityRaw >= 0 ? String(s.amsHumidityRaw) + "%" :
        (s.amsHumidity >= 0 ? String("L") + String(s.amsHumidity) : String("--"))};
    for (int i = 0; i < 6; ++i) {
        int x = 4 + i * 49;
        weactText(x, 113, labels[i], WeactStyle::SENSOR_LABEL);
        WeactTextStyle valueStyle = WeactStyle::SENSOR_VALUE;
        if ((i == 2 && s.leftNozzleTemp >= 180) || (i == 3 && s.rightNozzleTemp >= 180))
            valueStyle.color = WeactStyle::ALERT_COLOR;
        weactText(x, 124, compactText(values[i], 7), valueStyle);
        const int degreeX = weact.getCursorX() + 1;
        // Keep the humidity percent sign at its native weight for readability.
        String boldValue = compactText(values[i], 7);
        if (boldValue.endsWith("%")) boldValue = boldValue.substring(0, boldValue.length() - 1);
        // Overprint to strengthen the tiny numerals without increasing their height.
        for (int offset = 1; offset <= WeactStyle::SENSOR_VALUE_EXTRA_WEIGHT; ++offset)
            weactText(x + offset, 124, boldValue, valueStyle);
        // The display fonts contain ASCII only; draw a small raised degree ring.
        // Humidity and unavailable temperature readings do not get a degree mark.
        if (i < 5 && values[i] != "--")
            weact.drawRoundRect(degreeX, 117, 4, 4, 1, valueStyle.color);
    }
}
