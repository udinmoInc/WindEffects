#pragma once

#include "Icons/Export.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::icons {

/// Discover atlas tier sizes from `ui_Atlas_<N>.atlas` and/or `ui_Atlas_<N>.weiconatlas`
/// files in a directory. No hardcoded tier list.
///
/// Cross-DLL safe: allocates with malloc inside Icons.dll. Pair with FreeAtlasTiers.
[[nodiscard]] ICONS_API bool DiscoverAtlasTiersFlat(
    const char* directoryUtf8,
    std::uint32_t*& outTiers,
    std::uint32_t& outCount);
ICONS_API void FreeAtlasTiers(std::uint32_t* tiers);

/// Same-module convenience (Icons.dll only). Do not call across DLL boundaries.
[[nodiscard]] ICONS_API std::vector<std::uint32_t> DiscoverAtlasTiers(
    const std::filesystem::path& directory);

/// Parse `ui_Atlas_24` / `ui_Atlas_24.atlas` / `ui_Atlas_101.weiconatlas` → 24 / 101.
/// Returns 0 when the name is not a recognized atlas stem.
[[nodiscard]] ICONS_API std::uint32_t ParseAtlasTierFromStem(std::string_view stem);

/// Square UI glyph tiers (kindicons + toolbar/tree/search). Folder-only packs (13/19/101/202) are excluded.
inline constexpr std::array<std::uint32_t, 7> kEssentialUiAtlasTiers = {12, 16, 20, 24, 32, 48, 64};

[[nodiscard]] constexpr bool IsEssentialUiAtlasTier(const std::uint32_t tierPx)
{
    for (const std::uint32_t essential : kEssentialUiAtlasTiers) {
        if (tierPx == essential) {
            return true;
        }
    }
    return false;
}

/// Intersect discovered tiers with the essential UI allowlist (sorted ascending).
[[nodiscard]] inline std::vector<std::uint32_t> FilterEssentialUiAtlasTiers(const std::span<const std::uint32_t> tiers)
{
    std::vector<std::uint32_t> filtered;
    filtered.reserve(kEssentialUiAtlasTiers.size());
    for (const std::uint32_t essential : kEssentialUiAtlasTiers) {
        if (std::find(tiers.begin(), tiers.end(), essential) != tiers.end()) {
            filtered.push_back(essential);
        }
    }
    if (!filtered.empty()) {
        return filtered;
    }
    filtered.assign(kEssentialUiAtlasTiers.begin(), kEssentialUiAtlasTiers.end());
    return filtered;
}

/// Folder glyphs may snap to dedicated folder-pack tiers (13/19/101/202).
[[nodiscard]] inline bool UsesFolderPackTiers(const std::string_view runtimeIconName)
{
    return runtimeIconName == "folder" || runtimeIconName == "openfolder";
}

enum class IconAtlasFormat : uint8_t {
    Rgba8 = 0,
};

enum class IconEntryFlags : uint8_t {
    None = 0,
    FullColor = 1 << 0,
};

inline IconEntryFlags operator|(IconEntryFlags lhs, IconEntryFlags rhs)
{
    return static_cast<IconEntryFlags>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline bool HasFlag(IconEntryFlags flags, IconEntryFlags test)
{
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(test)) != 0;
}

struct IconUvRect {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
};

struct IconPixelRect {
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t width = 0;
    uint16_t height = 0;
};

struct IconAtlasPage {
    uint32_t tierPx = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    IconAtlasFormat format = IconAtlasFormat::Rgba8;
    std::vector<uint8_t> rgba;
};

struct IconMetaEntry {
    uint64_t nameHash = 0;
    std::string name;
    uint32_t tierPx = 0;
    IconUvRect uv{};
    IconPixelRect pixel{};
    IconEntryFlags flags = IconEntryFlags::None;
};

struct IconMetaAsset {
    uint32_t version = 0;
    std::vector<IconMetaEntry> entries;
};

struct IconAtlasAsset {
    uint32_t version = 0;
    IconAtlasPage page{};
};

} // namespace we::runtime::icons
