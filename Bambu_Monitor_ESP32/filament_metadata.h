#pragma once
#include <string>
#include <cstdlib>
#include <cmath>

struct FilamentEstimate {
    std::string material;
    float grams = 0;
    unsigned count = 0;
};

// Bounded slice_info.config input; only inspect the requested plate.
inline std::string sliceAttribute(const std::string& tag, const std::string& key) {
    size_t p = tag.find(" " + key + "=");
    if (p == std::string::npos) return "";
    p += key.size() + 2;
    if (p >= tag.size() || (tag[p] != '\"' && tag[p] != '\'')) return "";
    size_t end = tag.find(tag[p], p + 1);
    return end == std::string::npos ? "" : tag.substr(p + 1, end - p - 1);
}
inline bool parseFilamentEstimate(const char* data, size_t size, int plateIndex, FilamentEstimate& out) {
    if (!data || size > 24 * 1024 || plateIndex < 1) return false;
    std::string xml(data, size);
    size_t start = 0;
    while ((start = xml.find("<plate>", start)) != std::string::npos) {
        size_t end = xml.find("</plate>", start);
        if (end == std::string::npos) return false;
        std::string plate = xml.substr(start, end - start);
        int index = -1;
        FilamentEstimate result;
        size_t p = 0;
        while ((p = plate.find('<', p)) != std::string::npos) {
            size_t q = plate.find('>', p);
            if (q == std::string::npos) return false;
            std::string tag = plate.substr(p, q - p + 1);
            if (tag.compare(0, 10, "<metadata ") == 0 && sliceAttribute(tag, "key") == "index")
                index = std::atoi(sliceAttribute(tag, "value").c_str());
            if (tag.compare(0, 10, "<filament ") == 0) {
                float grams = std::strtof(sliceAttribute(tag, "used_g").c_str(), nullptr);
                std::string type = sliceAttribute(tag, "type");
                if (std::isfinite(grams) && grams > 0 && grams < 100000 && !type.empty()) {
                    if (!result.count) result.material = type;
                    else if (result.material != type) result.material = "Mix";
                    result.grams += grams;
                    ++result.count;
                }
            }
            p = q + 1;
        }
        if (index == plateIndex && result.count && result.material.size() <= 16) {
            out = result;
            return true;
        }
        start = end + 8;
    }
    return false;
}
