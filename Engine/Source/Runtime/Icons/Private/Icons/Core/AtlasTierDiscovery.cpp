#include "Icons/Core/IconTypes.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::icons {

std::uint32_t ParseAtlasTierFromStem(std::string_view stem) {
    constexpr std::string_view kPrefix = "ui_Atlas_";
    if (stem.size() <= kPrefix.size() || stem.substr(0, kPrefix.size()) != kPrefix) {
        return 0;
    }
    const std::string_view number = stem.substr(kPrefix.size());
    if (number.empty()) {
        return 0;
    }
    std::uint32_t value = 0;
    for (char c : number) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return 0;
        }
        const std::uint32_t digit = static_cast<std::uint32_t>(c - '0');
        if (value > (std::numeric_limits<std::uint32_t>::max() - digit) / 10u) {
            return 0;
        }
        value = value * 10u + digit;
    }
    return value;
}

namespace {

std::vector<std::uint32_t> CollectTiers(const std::filesystem::path& directory) {
    std::vector<std::uint32_t> tiers;
    std::error_code ec;
    if (!std::filesystem::is_directory(directory, ec)) {
        return tiers;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
        if (ec || !entry.is_regular_file(ec)) {
            continue;
        }
        const auto ext = entry.path().extension().string();
        if (ext != ".atlas" && ext != ".weiconatlas") {
            continue;
        }
        const std::uint32_t tier = ParseAtlasTierFromStem(entry.path().stem().string());
        if (tier != 0) {
            tiers.push_back(tier);
        }
    }
    std::sort(tiers.begin(), tiers.end());
    tiers.erase(std::unique(tiers.begin(), tiers.end()), tiers.end());
    return tiers;
}

} // namespace

bool DiscoverAtlasTiersFlat(
    const char* directoryUtf8,
    std::uint32_t*& outTiers,
    std::uint32_t& outCount)
{
    outTiers = nullptr;
    outCount = 0;
    if (!directoryUtf8 || directoryUtf8[0] == '\0') {
        return true;
    }
    const auto tiers = CollectTiers(std::filesystem::path(directoryUtf8));
    if (tiers.empty()) {
        return true;
    }
    auto* buffer = static_cast<std::uint32_t*>(std::malloc(sizeof(std::uint32_t) * tiers.size()));
    if (!buffer) {
        return false;
    }
    std::memcpy(buffer, tiers.data(), sizeof(std::uint32_t) * tiers.size());
    outTiers = buffer;
    outCount = static_cast<std::uint32_t>(tiers.size());
    return true;
}

void FreeAtlasTiers(std::uint32_t* tiers)
{
    std::free(tiers);
}

std::vector<std::uint32_t> DiscoverAtlasTiers(const std::filesystem::path& directory) {
    // Same-module callers only (Icons.dll). Do not call across DLL boundaries.
    return CollectTiers(directory);
}

} // namespace we::runtime::icons
