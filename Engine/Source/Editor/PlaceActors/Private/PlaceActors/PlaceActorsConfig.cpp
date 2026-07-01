#include "PlaceActors/PlaceActorsConfig.h"

#include "Core/EditorConfigPaths.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace we::programs::editor {

namespace {

bool ParseBool(const std::string& value, bool fallback) {
    std::string lower = value;
    for (char& ch : lower) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") return true;
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off") return false;
    return fallback;
}

float ParseFloat(const std::string& value, float fallback) {
    try {
        return std::stof(value);
    } catch (...) {
        return fallback;
    }
}

int ParseInt(const std::string& value, int fallback) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

} // namespace

PlaceActorsConfig& PlaceActorsConfig::Get() {
    static PlaceActorsConfig instance;
    return instance;
}

void PlaceActorsConfig::EnsureLoaded() {
    if (!m_Loaded) {
        Load();
        m_Loaded = true;
    }
}

void PlaceActorsConfig::Load() {
    const auto path = we::core::ResolveEditorConfigPath("function.ini");
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    bool inSection = false;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.front() == '[') {
            inSection = (line.find("[Editor.PlaceActors]") != std::string::npos);
            continue;
        }
        if (!inSection) continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        const std::string key = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);

        if (key == "DefaultView") {
            defaultView = (value == "List") ? PlaceActorsViewMode::List : PlaceActorsViewMode::Grid;
        } else if (key == "IconSize") {
            iconSize = std::clamp(ParseFloat(value, iconSize), 32.0f, 96.0f);
        } else if (key == "CardSize") {
            cardSize = std::clamp(ParseFloat(value, cardSize), 64.0f, 140.0f);
        } else if (key == "ShowDescriptions") {
            showDescriptions = ParseBool(value, showDescriptions);
        } else if (key == "EnableAnimations") {
            enableAnimations = ParseBool(value, enableAnimations);
        } else if (key == "RememberCategoryState") {
            rememberCategoryState = ParseBool(value, rememberCategoryState);
        } else if (key == "RememberSearchHistory") {
            rememberSearchHistory = ParseBool(value, rememberSearchHistory);
        } else if (key == "ShowRecent") {
            showRecent = ParseBool(value, showRecent);
        } else if (key == "ShowFavorites") {
            showFavorites = ParseBool(value, showFavorites);
        } else if (key == "GridColumns") {
            gridColumns = std::clamp(ParseInt(value, gridColumns), 1, 6);
        } else if (key == "ListRowHeight") {
            listRowHeight = std::clamp(ParseFloat(value, listRowHeight), 32.0f, 72.0f);
        } else if (key == "CategoryHeaderHeight") {
            categoryHeaderHeight = std::clamp(ParseFloat(value, categoryHeaderHeight), 24.0f, 40.0f);
        }
    }
}

} // namespace we::programs::editor
