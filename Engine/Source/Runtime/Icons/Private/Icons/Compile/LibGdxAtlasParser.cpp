#include "Icons/Compile/IconCompileDetail.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace we::runtime::icons::compiling::detail {

namespace {

std::string Trim(std::string value)
{
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

std::string StripIconsPrefix(std::string value)
{
    if (value.starts_with("Icons_")) {
        value = value.substr(6);
    }
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool ParseKeyValue(const std::string& line, std::string& outKey, std::string& outValue)
{
    const size_t colon = line.find(':');
    if (colon == std::string::npos) {
        return false;
    }
    outKey = Trim(line.substr(0, colon));
    outValue = Trim(line.substr(colon + 1));
    return true;
}

bool ParsePair(const std::string& value, uint16_t& outA, uint16_t& outB)
{
    const size_t comma = value.find(',');
    if (comma == std::string::npos) {
        return false;
    }
    outA = static_cast<uint16_t>(std::stoul(Trim(value.substr(0, comma))));
    outB = static_cast<uint16_t>(std::stoul(Trim(value.substr(comma + 1))));
    return true;
}

uint32_t TierFromAtlasStem(const std::filesystem::path& atlasPath)
{
    const std::string stem = atlasPath.stem().string();
    const std::string prefix = "atlas_";
    if (!stem.starts_with(prefix)) {
        return 0;
    }
    return static_cast<uint32_t>(std::stoul(stem.substr(prefix.size())));
}

} // namespace

bool ParseLibGdxAtlasFile(
    const std::filesystem::path& atlasPath,
    const std::filesystem::path& searchRoot,
    ParsedAtlasDescriptor& outDescriptor,
    std::string& outError)
{
    std::ifstream input(atlasPath);
    if (!input) {
        outError = "Failed to open atlas descriptor: " + atlasPath.string();
        return false;
    }

    outDescriptor = {};
    outDescriptor.tierPx = TierFromAtlasStem(atlasPath);

    ParsedAtlasRegion* currentRegion = nullptr;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        line = Trim(line);
        if (line.empty()) {
            continue;
        }

        if (outDescriptor.pngPath.empty() && line.find(':') == std::string::npos) {
            const auto pngCandidate = searchRoot / line;
            if (!std::filesystem::exists(pngCandidate)) {
                outError = "Atlas PNG not found: " + pngCandidate.string();
                return false;
            }
            outDescriptor.pngPath = pngCandidate;
            continue;
        }

        std::string key;
        std::string value;
        if (ParseKeyValue(line, key, value)) {
            if (currentRegion == nullptr && key == "size") {
                uint16_t width = 0;
                uint16_t height = 0;
                if (!ParsePair(value, width, height)) {
                    outError = "Invalid atlas size in " + atlasPath.string();
                    return false;
                }
                outDescriptor.width = width;
                outDescriptor.height = height;
            } else if (currentRegion != nullptr) {
                if (key == "xy") {
                    ParsePair(value, currentRegion->x, currentRegion->y);
                } else if (key == "size" || key == "orig") {
                    ParsePair(value, currentRegion->width, currentRegion->height);
                }
            }
            continue;
        }

        outDescriptor.regions.push_back(ParsedAtlasRegion{.sourceName = line});
        currentRegion = &outDescriptor.regions.back();
    }

    if (outDescriptor.pngPath.empty() || outDescriptor.width == 0 || outDescriptor.height == 0) {
        outError = "Atlas descriptor missing PNG or size: " + atlasPath.string();
        return false;
    }
    if (outDescriptor.regions.empty()) {
        outError = "Atlas descriptor contains no regions: " + atlasPath.string();
        return false;
    }
    return true;
}

std::string ResolveRuntimeIconName(const std::string& atlasRegionName)
{
    static const std::unordered_map<std::string, std::string> kAtlasNameMap = {
        {"Icons_Close", "x"},
        {"Icons_Cursor", "mouse-pointer-2"},
        {"Icons_Grid", "grid-3x3"},
        {"Icons_Info", "info"},
        {"Icons_Move", "move"},
        {"Icons_Object", "toolbar-object"},
        {"Icons_Play", "play"},
        {"Icons_Plus", "plus"},
        {"Icons_Redo", "redo"},
        {"Icons_Rotate", "rotate-cw"},
        {"Icons_Scale", "scaling"},
        {"Icons_Search", "search"},
        {"Icons_Settings", "settings"},
        {"Icons_Stop", "square"},
        {"Icons_Undo", "undo"},
    };

    if (const auto it = kAtlasNameMap.find(atlasRegionName); it != kAtlasNameMap.end()) {
        return it->second;
    }

    const std::string stem = StripIconsPrefix(atlasRegionName);
    if (stem == "close") {
        return "x";
    }
    if (stem == "cursor") {
        return "mouse-pointer-2";
    }
    if (stem == "grid") {
        return "grid-3x3";
    }
    if (stem == "object") {
        return "toolbar-object";
    }
    if (stem == "rotate") {
        return "rotate-cw";
    }
    if (stem == "scale") {
        return "scaling";
    }
    if (stem == "stop") {
        return "square";
    }
    return stem;
}

bool IsFullColorIcon(const std::string& runtimeName)
{
    return runtimeName == "search"
        || runtimeName == "settings"
        || runtimeName == "toolbar-object"
        || runtimeName == "toolbar-environment"
        || runtimeName == "windeffects"
        || runtimeName == "save"
        || runtimeName == "save-all"
        || runtimeName == "project-folder";
}

std::vector<std::string> RuntimeAliasesFor(const std::string& runtimeName)
{
    static const std::unordered_map<std::string, std::vector<std::string>> kAliases = {
        {"mouse-pointer-2", {"cursor"}},
        {"grid-3x3", {"grid", "wireframe"}},
        {"rotate-cw", {"rotate"}},
        {"scaling", {"scale"}},
        {"square", {"stop"}},
        {"toolbar-object", {"object", "component", "toolbar-object"}},
        {"x", {"close"}},
    };

    std::vector<std::string> names;
    names.push_back(runtimeName);
    if (const auto it = kAliases.find(runtimeName); it != kAliases.end()) {
        for (const auto& alias : it->second) {
            if (alias != runtimeName) {
                names.push_back(alias);
            }
        }
    }
    return names;
}

} // namespace we::runtime::icons::compiling::detail
