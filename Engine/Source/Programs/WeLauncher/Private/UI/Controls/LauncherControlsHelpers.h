#pragma once
#include "UI/Controls/LauncherControls.h"
#include "KindUI/Core/TextMetrics.h"
#include <string>
namespace we::programs::welauncher {
namespace launcher_controls_detail {

constexpr float kSegmentH = 28.0f;

inline bool AppendUtf8(std::string& out, char32_t cp) {
    if (cp < 32 && cp != '\t') {
        return false;
    }
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        return false;
    }
    return true;
}

inline float ApproxTextWidth(const std::string& text, float textSize) {
    return we::runtime::kindui::TextMetrics::MeasureWidth(text, textSize);
}

inline void EraseLastUtf8Codepoint(std::string& text) {
    if (text.empty()) {
        return;
    }
    while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80) {
        text.pop_back();
    }
    if (!text.empty()) {
        text.pop_back();
    }
}

} // namespace launcher_controls_detail
} // namespace we::programs::welauncher
