#pragma once

#include "AssetCooker/CookTypes.h"
#include "AssetCooker/Export.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace we::runtime::assetcooker {

/// Shipping package container: WEPA magic + TOC + concatenated payloads.
struct ASSETCOOKER_API AssetPackageArchive {
    static constexpr uint32_t kMagic = 0x41504557; // "WEPA" LE
    static constexpr uint16_t kVersion = 1;

    CookPlatform platform = CookPlatform::Windows;
    std::string platformName;
    std::vector<CookedAssetRecord> toc;
    std::vector<std::byte> payloadBlob;
};

[[nodiscard]] ASSETCOOKER_API bool WriteWepak(
    const std::filesystem::path& path,
    const AssetPackageArchive& archive);

[[nodiscard]] ASSETCOOKER_API std::optional<AssetPackageArchive> ReadWepak(
    const std::filesystem::path& path);

} // namespace we::runtime::assetcooker
