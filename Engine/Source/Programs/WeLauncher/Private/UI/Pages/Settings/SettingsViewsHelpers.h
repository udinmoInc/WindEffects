#pragma once
#include "UI/Pages/Settings/SettingsViews.h"
#include "UI/Shell/LauncherHelpers.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/Types.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace we::programs::welauncher {
namespace settings_detail {

using we::runtime::kindui::Color;
using we::runtime::kindui::ColorToken;
using we::runtime::kindui::TextMetrics;

// Input/dropdown value text — slightly brighter than primary for long paths.
inline Color InputValueTextColor() {
    return Color::Lerp(LColor(ColorToken::TextPrimary), Color{ 1.0f, 1.0f, 1.0f, 1.0f }, 0.28f);
}

inline float ApproxTextWidth(const std::string& text, float textSize) {
    return TextMetrics::MeasureWidth(text, textSize);
}

inline std::string ToLowerLocal(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

inline bool ContainsInsensitive(const std::string& haystack, const std::string& needleLower) {
    if (needleLower.empty()) {
        return true;
    }
    return ToLowerLocal(haystack).find(needleLower) != std::string::npos;
}

inline const char* kAccentPalette[] = {
    "#5B8DEF", "#6BCB9A", "#E0A35A", "#C97BDB", "#5AABB8", "#E07070", "#A0A8B8"
};

} // namespace settings_detail

inline const char* SettingsCategoryTitle(SettingsCategory category) {
    switch (category) {
    case SettingsCategory::General: return "General";
    case SettingsCategory::Engine: return "Engine";
    case SettingsCategory::Storage: return "Storage";
    case SettingsCategory::FileAssociations: return "File Associations";
    case SettingsCategory::About: return "About";
    default: return "Settings";
    }
}

inline const char* SettingsCategoryKeywords(SettingsCategory category) {
    switch (category) {
    case SettingsCategory::General:
        return "general projects folder engine version recent limit open last start";
    case SettingsCategory::Engine:
        return "engine install directory scan verify updates launcher";
    case SettingsCategory::Storage:
        return "storage cache thumbnail clear size disk";
    case SettingsCategory::FileAssociations:
        return "file associations weproj weproject register";
    case SettingsCategory::About:
        return "about version logs installation documentation reset";
    default:
        return "";
    }
}

inline we::runtime::kindui::Color ParseHexColor(const std::string& hex) {
    unsigned int r = 91, g = 141, b = 239;
    if (hex.size() >= 7 && hex[0] == '#') {
        unsigned int value = 0;
        if (std::sscanf(hex.c_str() + 1, "%06x", &value) == 1) {
            r = (value >> 16) & 0xFF;
            g = (value >> 8) & 0xFF;
            b = value & 0xFF;
        }
    }
    return we::runtime::kindui::Color{
        static_cast<float>(r) / 255.0f,
        static_cast<float>(g) / 255.0f,
        static_cast<float>(b) / 255.0f,
        1.0f
    };
}

inline int IndexOfOption(const std::vector<std::string>& options, const std::string& value) {
    for (int i = 0; i < static_cast<int>(options.size()); ++i) {
        if (options[static_cast<std::size_t>(i)] == value) {
            return i;
        }
    }
    return 0;
}

} // namespace we::programs::welauncher
