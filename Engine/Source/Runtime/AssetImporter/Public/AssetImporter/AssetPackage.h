#pragma once

#include "AssetImporter/AssetMetadata.h"
#include "AssetImporter/Export.h"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace we::runtime::assetimporter {

/// Native cooked container: WEAS magic + versioned header + JSON metadata + binary payload.
/// Specialized formats (.wefont, .weiconatlas) remain valid; .weasset wraps any payload.
struct ASSETIMPORTER_API AssetPackage {
    static constexpr uint32_t kMagic = 0x53414557; // "WEAS" LE
    static constexpr uint16_t kVersion = 1;

    AssetMetadata metadata{};
    std::vector<std::byte> payload;
    std::string payloadContentType; // e.g. "application/vnd.windeffects.texture.v1"
};

[[nodiscard]] ASSETIMPORTER_API bool WriteAssetPackage(
    const std::filesystem::path& path,
    const AssetPackage& package);

[[nodiscard]] ASSETIMPORTER_API std::optional<AssetPackage> ReadAssetPackage(
    const std::filesystem::path& path);

[[nodiscard]] ASSETIMPORTER_API std::string ComputeFileHashHex(const std::filesystem::path& path);

} // namespace we::runtime::assetimporter
