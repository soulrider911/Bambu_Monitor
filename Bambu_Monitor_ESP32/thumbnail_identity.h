#pragma once
#include <stddef.h>
#include <string.h>
#include <string>

// Bounded XML metadata reader; does not load geometry, resolve entities or URLs.
inline size_t thumbnailFind(const char* data, size_t size, size_t start, const char* needle) {
    const size_t length = strlen(needle);
    for (size_t i = start; i + length <= size; ++i)
        if (memcmp(data + i, needle, length) == 0) return i;
    return size;
}

inline std::string thumbnailXmlText(const char* data, size_t size) {
    std::string value(data, size);
    const char* encoded[] = {"&lt;", "&gt;", "&quot;", "&apos;", "&amp;"};
    const char* decoded[] = {"<", ">", "\"", "'", "&"};
    for (int i = 0; i < 5; ++i) {
        size_t at = 0;
        while ((at = value.find(encoded[i], at)) != std::string::npos) {
            value.replace(at, strlen(encoded[i]), decoded[i]);
            ++at;
        }
    }
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

inline bool thumbnailMetadataMatches(const char* data, size_t size, const char* job) {
    const std::string expected = thumbnailXmlText(job, strlen(job));
    if (expected.empty()) return false;
    size_t at = 0;
    while ((at = thumbnailFind(data, size, at, "<metadata ")) < size) {
        const size_t end = thumbnailFind(data, size, at, ">");
        if (end == size) break;
        const size_t close = thumbnailFind(data, size, end + 1, "</metadata>");
        if (close == size) break;
        const bool title = thumbnailFind(data, end, at, "name=\"ProfileTitle\"") < end ||
                           thumbnailFind(data, end, at, "name=\"Title\"") < end ||
                           thumbnailFind(data, end, at, "name='ProfileTitle'") < end ||
                           thumbnailFind(data, end, at, "name='Title'") < end;
        if (title && close - end - 1 <= 512 &&
            thumbnailXmlText(data + end + 1, close - end - 1) == expected) return true;
        at = close + 11;
    }
    return false;
}
