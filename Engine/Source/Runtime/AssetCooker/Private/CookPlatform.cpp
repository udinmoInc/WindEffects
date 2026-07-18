#include "AssetCooker/CookPlatform.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace we::runtime::assetcooker {

std::string_view CookPlatformToString(CookPlatform platform) {
    switch (platform) {
    case CookPlatform::Windows: return "Windows";
    case CookPlatform::Linux: return "Linux";
    case CookPlatform::Android: return "Android";
    case CookPlatform::Custom: return "Custom";
    default: return "Unknown";
    }
}

CookPlatform CookPlatformFromString(std::string_view name) {
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower == "windows" || lower == "win64" || lower == "win32") {
        return CookPlatform::Windows;
    }
    if (lower == "linux") {
        return CookPlatform::Linux;
    }
    if (lower == "android") {
        return CookPlatform::Android;
    }
    return CookPlatform::Custom;
}

} // namespace we::runtime::assetcooker
