#pragma once
#include "UI/Pages/Settings/SettingsViews.h"
#include "UI/Shell/LauncherHelpers.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Theming/PaletteRuntime.h"

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
using we::runtime::kindui::palette::GraphiteDarkLive;

// Input/dropdown value text — slightly brighter than primary for long paths.
inline Color InputValueTextColor() {
    return Color::Pick(LColor(ColorToken::TextPrimary), GraphiteDarkLive().ForegroundHover, 0.28f);
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

inline constexpr int kAccentPaletteCount = 7;

inline Color AccentSwatch(int index) {
    const auto& p = GraphiteDarkLive();
    switch (index) {
    case 0: return p.Primary;
    case 1: return p.AccentGreen;
    case 2: return p.AccentOrange;
    case 3: return p.AccentPurple;
    case 4: return p.AccentBlue;
    case 5: return p.AccentRed;
    case 6: return p.AccentGray;
    default: return p.Primary;
    }
}

inline std::string ColorToHexRgb(Color color) {
    char buf[8];
    const int r = static_cast<int>(std::lround(std::clamp(color.r, 0.0f, 1.0f) * 255.0f));
    const int g = static_cast<int>(std::lround(std::clamp(color.g, 0.0f, 1.0f) * 255.0f));
    const int b = static_cast<int>(std::lround(std::clamp(color.b, 0.0f, 1.0f) * 255.0f));
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
    return buf;
}

inline std::string DefaultAccentHex() {
    return ColorToHexRgb(GraphiteDarkLive().Primary);
}

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
    using we::runtime::kindui::palette::GraphiteDarkLive;
    unsigned int r = 0;
    unsigned int g = 0;
    unsigned int b = 0;
    if (hex.size() >= 7 && hex[0] == '#') {
        unsigned int value = 0;
        if (std::sscanf(hex.c_str() + 1, "%06x", &value) == 1) {
            r = (value >> 16) & 0xFF;
            g = (value >> 8) & 0xFF;
            b = value & 0xFF;
            return we::runtime::kindui::Color{
                static_cast<float>(r) / 255.0f,
                static_cast<float>(g) / 255.0f,
                static_cast<float>(b) / 255.0f,
                1.0f
            };
        }
    }
    return GraphiteDarkLive().Primary;
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
