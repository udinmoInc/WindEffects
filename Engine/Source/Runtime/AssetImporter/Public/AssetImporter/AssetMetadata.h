#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetImporter/Export.h"
#include "AssetImporter/Types.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::assetimporter {

/// Sidecar / embedded metadata written for every imported asset.
struct ASSETIMPORTER_API AssetMetadata {
    int schemaVersion = 1;
    AssetGuid guid{};
    AssetKind kind = AssetKind::Unknown;
    std::string displayName;
    std::string sourcePath;
    std::string sourceHash;          // hex SHA-256 or FNV of source bytes
    std::string importerId;          // e.g. "texture.stb", "font.ttf"
    std::string importerVersion = "1.0.0";
    std::string engineVersion;
    std::string createdUtc;
    std::string modifiedUtc;
    std::vector<AssetGuid> dependencies;
    std::vector<std::string> tags;
#if WE_HAS_NLOHMANN_JSON
    nlohmann::json importSettings = nlohmann::json::object();
    nlohmann::json custom = nlohmann::json::object();
#endif

    [[nodiscard]] bool IsValid() const noexcept;
};

[[nodiscard]] ASSETIMPORTER_API bool SaveMetadataJson(
    const std::filesystem::path& path,
    const AssetMetadata& metadata);

[[nodiscard]] ASSETIMPORTER_API std::optional<AssetMetadata> LoadMetadataJson(
    const std::filesystem::path& path);

} // namespace we::runtime::assetimporter
