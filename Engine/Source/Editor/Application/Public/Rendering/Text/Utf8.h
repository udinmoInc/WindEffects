#pragma once

#include "Application/Export.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::UI::Text {

// Decode UTF-8 text into Unicode codepoints.
inline void DecodeUtf8(std::string_view text, std::vector<uint32_t>& outCodepoints) {
    outCodepoints.clear();
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    size_t i = 0;
    while (i < text.size()) {
        const unsigned char c = bytes[i];
        uint32_t codepoint = 0;
        size_t extra = 0;

        if (c < 0x80) {
            codepoint = c;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
            codepoint = (static_cast<uint32_t>(c & 0x1F) << 6)
                | static_cast<uint32_t>(bytes[i + 1] & 0x3F);
            extra = 1;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) {
            codepoint = (static_cast<uint32_t>(c & 0x0F) << 12)
                | (static_cast<uint32_t>(bytes[i + 1] & 0x3F) << 6)
                | static_cast<uint32_t>(bytes[i + 2] & 0x3F);
            extra = 2;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < text.size()) {
            codepoint = (static_cast<uint32_t>(c & 0x07) << 18)
                | (static_cast<uint32_t>(bytes[i + 1] & 0x3F) << 12)
                | (static_cast<uint32_t>(bytes[i + 2] & 0x3F) << 6)
                | static_cast<uint32_t>(bytes[i + 3] & 0x3F);
            extra = 3;
        } else {
            codepoint = 0xFFFD;
        }

        outCodepoints.push_back(codepoint);
        i += 1 + extra;
    }
}

} // namespace we::UI::Text
