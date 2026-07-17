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
    // Canonical: Icons_3DPlane. Tolerate missing underscore: Icons3DPlane.
    if (value.starts_with("Icons_")) {
        value = value.substr(6);
    } else if (value.size() > 5 && value.starts_with("Icons")) {
        const unsigned char next = static_cast<unsigned char>(value[5]);
        if (std::isdigit(next) || std::isupper(next)) {
            value = value.substr(5);
        }
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
    const std::string prefix = "ui_Atlas_";
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
    std::string stem = StripIconsPrefix(atlasRegionName);

    if (stem == "cons_object" || atlasRegionName == "cons_Object") {
        return "object";
    }
    // Legacy mislabeled region from ui_Atlas_64 (Icons3DPlane → icons3dplane).
    if (stem == "icons3dplane") {
        return "3dplane";
    }

    return stem;
}

bool IsFullColorIcon(const std::string& runtimeName)
{
    // Brand / shaded 3D previews keep baked RGB instead of mono tinting.
    return runtimeName == "windeffects"
        || runtimeName == "windlogo"
        || runtimeName == "3dcube"
        || runtimeName == "3dsphere"
        || runtimeName == "3dcylinder"
        || runtimeName == "3dplane"
        || runtimeName == "3dcone"
        || runtimeName == "3dcapsule"
        || runtimeName == "3dblankactor";
}

std::vector<std::string> RuntimeAliasesFor(const std::string& runtimeName)
{
    static const std::unordered_map<std::string, std::vector<std::string>> kAliases = {
        {"cursor", {"mouse-pointer-2"}},
        {"grid", {"grid-3x3", "wireframe"}},
        {"rotate", {"rotate-cw"}},
        {"scale", {"scaling"}},
        {"stop", {"square"}},
        {"object", {"component", "toolbar-object"}},
        {"close", {"x"}},
        {"chevrondown", {"chevron-down"}},
        {"chevronright", {"chevron-right"}},
        {"chevronleft", {"chevron-left"}},
        {"chevronup", {"chevron-up"}},
        {"sun", {"lit", "globe", "toolbar-environment"}},
        {"eyeoff", {"eye-off"}},
        {"playsolid", {"play-solid"}},
        {"outputlog", {"output-log"}},
        {"mediaplay", {"media-play"}},
        {"saveall", {"save-all"}},
        {"contentbrowser", {"content-browser"}},
        {"newfile", {"new-file", "file-plus", "new"}},
        {"openfolder", {"open-folder", "folder-open", "open"}},
        {"addactor", {"add-actor"}},
        {"star", {"favorites", "star-filled"}},
        {"windlogo", {"windeffects", "wind-logo", "logo"}},
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
