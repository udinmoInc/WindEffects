#pragma once

#include "Icons/Core/Errors.h"
#include "Icons/Core/IconTypes.h"
#include "Icons/Export.h"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace we::runtime::icons::assets {

constexpr uint32_t kWeIconAtlasMagic = 0x534F4349; // "ICOS"
constexpr uint32_t kWeIconMetaMagic = 0x54454D49;  // "IMET"
constexpr uint16_t kWeIconAtlasVersion = 1;
constexpr uint16_t kWeIconMetaVersion = 1;

class ICONS_API IconAtlasReader {
public:
    [[nodiscard]] static IconResult<IconAtlasAsset> LoadFromFile(const std::filesystem::path& path);
    [[nodiscard]] static IconResult<IconAtlasAsset> LoadFromMemory(std::span<const std::byte> data);
};

class ICONS_API IconAtlasWriter {
public:
    [[nodiscard]] static IconResult<void> WriteToFile(
        const IconAtlasAsset& asset,
        const std::filesystem::path& path);
};

class ICONS_API IconMetaReader {
public:
    [[nodiscard]] static IconResult<IconMetaAsset> LoadFromFile(const std::filesystem::path& path);
    [[nodiscard]] static IconResult<IconMetaAsset> LoadFromMemory(std::span<const std::byte> data);
};

class ICONS_API IconMetaWriter {
public:
    [[nodiscard]] static IconResult<void> WriteToFile(
        const IconMetaAsset& asset,
        const std::filesystem::path& path);
};

class ICONS_API IconAssetValidator {
public:
    struct ValidationReport {
        bool isValid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    [[nodiscard]] static ValidationReport Validate(const IconAtlasAsset& asset);
    [[nodiscard]] static ValidationReport Validate(const IconMetaAsset& asset);
};

} // namespace we::runtime::icons::assets
