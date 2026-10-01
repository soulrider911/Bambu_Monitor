#pragma once
#include <Arduino.h>
#include <pngle.h>
#include <new>

const int PREVIEW_W = 88;
const int PREVIEW_H = 76;
enum class PreviewDither { FloydSteinberg, Atkinson };
namespace WeactPreviewStyle {
    constexpr int THUMB_BRIGHTNESS = 6; // Grayscale offset: positive lightens, 0 preserves baseline.
    constexpr int THUMB_HIGHLIGHT_CUTOFF = 248; // Values above this become pure white; 255 disables.
    // Change to PreviewDither::Atkinson for the classic Macintosh appearance.
    constexpr PreviewDither DITHER = PreviewDither::FloydSteinberg;
    constexpr float GAMMA = 2.2f; // >1 brightens midtones; 1 is neutral.
    constexpr float CONTRAST = 0.90f; // 1 is neutral; lower softens dark shadows.
    constexpr int SHARPEN_PERCENT = 25; // 0 disables; 25 is a subtle edge boost.
    constexpr int BLACK_THRESHOLD = 120; // 1..254; lower favors white pixels.
    static_assert(GAMMA > 0 && GAMMA <= 5, "Preview gamma must be in (0, 5]");
    static_assert(CONTRAST > 0 && CONTRAST <= 2, "Preview contrast must be in (0, 2]");
    static_assert(SHARPEN_PERCENT >= 0 && SHARPEN_PERCENT <= 100, "Sharpen must be 0..100");
    static_assert(BLACK_THRESHOLD > 0 && BLACK_THRESHOLD < 255, "Threshold must be 1..254");
}
static uint8_t weactPreview[PREVIEW_W * PREVIEW_H / 8];

// Map cropped model levels into printable shades. Lift the shadow floor to
// preserve dots within dark surfaces; keep white background completely white.
inline uint8_t previewTone(uint8_t value, int lo, int hi,
                           float gamma = WeactPreviewStyle::GAMMA,
                           float contrast = WeactPreviewStyle::CONTRAST) {
    if (value >= 245) return 255;
    float level = constrain((value - lo) / float(max(1, hi - lo)), 0.0f, 1.0f);
    level = powf(level, 1.0f / gamma);
    level = constrain((level - 0.5f) * contrast + 0.5f, 0.0f, 1.0f);
    return (uint8_t)(64 + roundf(181 * level));
}

// Two decoder passes: find the content bounds, then sample directly into a
// fixed-size destination. Never allocate a full-resolution source image.
struct SmallPreviewContext {
    uint32_t width = 0, height = 0;
    int minX = 1024, minY = 1024, maxX = -1, maxY = -1;
    int cropW = 0, cropH = 0, drawW = 0, drawH = 0;
    bool sample = false, done = false, valid = false;
    uint8_t gray[PREVIEW_W * PREVIEW_H];
};
inline void smallPreviewInit(pngle_t* png, uint32_t w, uint32_t h) {
    auto* c = (SmallPreviewContext*)pngle_get_user_data(png);
    c->width = w; c->height = h;
    c->valid = w > 0 && h > 0 && w <= 1024 && h <= 1024;
}
inline void smallPreviewDone(pngle_t* png) {
    ((SmallPreviewContext*)pngle_get_user_data(png))->done = true;
}
inline void smallPreviewPixel(pngle_t* png, uint32_t x, uint32_t y,
                              uint32_t w, uint32_t h, const uint8_t rgba[4]) {
    auto* c = (SmallPreviewContext*)pngle_get_user_data(png);
    if (!c->valid) return;
    int lum = (30 * rgba[0] + 59 * rgba[1] + 11 * rgba[2]) / 100;
    int gray = 255 - rgba[3] * (255 - lum) / 255;
    if (!c->sample) {
        if (gray < 240) {
            c->minX = min(c->minX, (int)x); c->minY = min(c->minY, (int)y);
            c->maxX = max(c->maxX, (int)min(x + w, c->width) - 1);
            c->maxY = max(c->maxY, (int)min(y + h, c->height) - 1);
        }
        return;
    }
    // Only destination pixels whose source centers fall in this callback.
    int dx0 = max(0, ((int)x - c->minX) * c->drawW / c->cropW - 1);
    int dx1 = min(c->drawW, ((int)(x + w) - c->minX) * c->drawW / c->cropW + 1);
    int dy0 = max(0, ((int)y - c->minY) * c->drawH / c->cropH - 1);
    int dy1 = min(c->drawH, ((int)(y + h) - c->minY) * c->drawH / c->cropH + 1);
    for (int dy = dy0; dy < dy1; ++dy) {
        int sy = c->minY + (2 * dy + 1) * c->cropH / (2 * c->drawH);
        if (sy < (int)y || sy >= (int)(y + h)) continue;
        for (int dx = dx0; dx < dx1; ++dx) {
            int sx = c->minX + (2 * dx + 1) * c->cropW / (2 * c->drawW);
            if (sx < (int)x || sx >= (int)(x + w)) continue;
            int ox = (PREVIEW_W - c->drawW) / 2, oy = (PREVIEW_H - c->drawH) / 2;
            c->gray[(dy + oy) * PREVIEW_W + dx + ox] = gray;
        }
    }
}
inline bool decodeWeactPreview(const uint8_t* data, size_t size,
                               PreviewDither dither = WeactPreviewStyle::DITHER) {
    // Validate IHDR before pngle can allocate scanlines from untrusted dimensions.
    const uint8_t signature[] = {137,80,78,71,13,10,26,10};
    if (!data || size < 33 || size > 24 * 1024 || memcmp(data, signature, 8) ||
        memcmp(data + 12, "IHDR", 4)) return false;
    auto be32 = [](const uint8_t* p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; };
    if (!be32(data + 16) || !be32(data + 20) || be32(data + 16) > 1024 || be32(data + 20) > 1024) return false;
#ifndef WEACT_HOST_PREVIEW
    if (ESP.getFreeHeap() < 80 * 1024) return false;
#endif
    // pngle needs one contiguous ~44 KiB block. Allocate it before the 7 KiB
    // sampling context, which could otherwise split the only suitable block.
    pngle_t* png = pngle_new();
    if (!png) {
#ifndef WEACT_HOST_PREVIEW
        Serial.println("Thumbnail decode: cannot allocate PNG decoder");
#endif
        return false;
    }
    auto* c = new (std::nothrow) SmallPreviewContext;
    if (!c) { pngle_destroy(png); return false; }
    memset(c->gray, 255, sizeof(c->gray));
    pngle_set_user_data(png, c);
    pngle_set_init_callback(png, smallPreviewInit);
    pngle_set_draw_callback(png, smallPreviewPixel);
    pngle_set_done_callback(png, smallPreviewDone);
    bool ok = true;
    for (int pass = 0; pass < 2 && ok; ++pass) {
        c->done = false;
        size_t offset = 0;
        while (offset < size && !c->done) {
            int fed = pngle_feed(png, data + offset, size - offset);
            if (fed <= 0) {
#ifndef WEACT_HOST_PREVIEW
                Serial.printf("Thumbnail decode: %s\n", pngle_error(png));
#endif
                ok = false;
                break;
            }
            offset += fed;
            yield();
        }
        ok = ok && c->done && c->valid && c->maxX >= c->minX && c->maxY >= c->minY;
        if (pass == 0 && ok) {
            c->minX = max(0, c->minX - 4); c->minY = max(0, c->minY - 4);
            c->cropW = min((int)c->width, c->maxX + 5) - c->minX;
            c->cropH = min((int)c->height, c->maxY + 5) - c->minY;
            c->drawW = PREVIEW_W; c->drawH = max(1, c->cropH * PREVIEW_W / c->cropW);
            if (c->drawH > PREVIEW_H) {
                c->drawH = PREVIEW_H; c->drawW = max(1, c->cropW * PREVIEW_H / c->cropH);
            }
            c->sample = true;
            pngle_reset(png);
        }
    }
    if (ok) {
        // Stretch model contrast before monochrome error diffusion; retain white
        // margins so a dark source model does not become a solid silhouette.
        uint32_t histogram[256] = {}, count = 0;
        for (uint8_t value : c->gray) if (value < 240) { histogram[value]++; count++; }
        int lo = 0, hi = 239;
        uint32_t accumulated = 0;
        bool haveLo = false;
        for (int value = 0; value < 240 && count; ++value) {
            accumulated += histogram[value];
            if (!haveLo && accumulated * 100 >= count) { lo = value; haveLo = true; }
            if (accumulated * 100 >= count * 99) { hi = value; break; }
        }
        lo = min(lo, 215); hi = max(hi, lo + 24);
        // Apply levels before sharpening or distributing quantization error.
        for (uint8_t& value : c->gray) {
            value = previewTone(value, lo, hi);
        }
        // Rolling source rows keep sharpening independent of already-written pixels.
        // This uses 264 bytes rather than a second full grayscale image.
        uint8_t rows[3][PREVIEW_W];
        memcpy(rows[0], c->gray, PREVIEW_W);
        for (int y = 0; y < PREVIEW_H; ++y) {
            memcpy(rows[y % 3], c->gray + y * PREVIEW_W, PREVIEW_W);
            if (y + 1 < PREVIEW_H)
                memcpy(rows[(y + 1) % 3], c->gray + (y + 1) * PREVIEW_W, PREVIEW_W);
            for (int x = 0; x < PREVIEW_W; ++x) {
                int center = rows[y % 3][x];
                int left = rows[y % 3][max(0, x - 1)];
                int right = rows[y % 3][min(PREVIEW_W - 1, x + 1)];
                int above = rows[max(0, y - 1) % 3][x];
                int below = rows[min(PREVIEW_H - 1, y + 1) % 3][x];
                c->gray[y * PREVIEW_W + x] = center == 255 ? 255 : constrain(center +
                    (4 * center - left - right - above - below) * WeactPreviewStyle::SHARPEN_PERCENT / 400, 48, 254);
            }
        }
        // Grayscale is 0=black, 255=white. Adjust only the final dither input.
        for (uint8_t& value : c->gray) {
            value = constrain(int(value) + WeactPreviewStyle::THUMB_BRIGHTNESS, 0, 255);
            if (value > WeactPreviewStyle::THUMB_HIGHLIGHT_CUTOFF) value = 255;
        }
        int16_t errors[3][PREVIEW_W + 4] = {};
        memset(weactPreview, 0, sizeof(weactPreview));
        for (int y = 0; y < PREVIEW_H; ++y) {
            auto* current = errors[y % 3]; auto* next = errors[(y + 1) % 3];
            auto* afterNext = errors[(y + 2) % 3];
            for (int x = 0; x < PREVIEW_W; ++x) {
                int value = c->gray[y * PREVIEW_W + x];
                if (value == 255) continue; // Do not diffuse dots into white margins.
                value = constrain(value + current[x + 1], 0, 255);
                int quantized = value < WeactPreviewStyle::BLACK_THRESHOLD ? 0 : 255;
                if (!quantized) weactPreview[y * (PREVIEW_W / 8) + x / 8] |= 0x80 >> (x % 8);
                int error = value - quantized;
                if (dither == PreviewDither::Atkinson) {
                    // Six neighbors receive 1/8 each; the remaining 1/4 is discarded.
                    int share = error / 8;
                    current[x + 2] += share;
                    current[x + 3] += share;
                    next[x] += share;
                    next[x + 1] += share;
                    next[x + 2] += share;
                    afterNext[x + 1] += share;
                } else {
                    current[x + 2] += error * 7 / 16;
                    next[x] += error * 3 / 16;
                    next[x + 1] += error * 5 / 16;
                    next[x + 2] += error / 16;
                }
            }
            memset(current, 0, sizeof(errors[0]));
        }
    }
    pngle_destroy(png);
    delete c;
    return ok;
}
