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

/// POD atlas page for safe cross-DLL consumption (RGBA owned via malloc in Icons.dll).
/// Prefer LoadAtlasPixels — avoids passing structs by reference across the DLL boundary.
[[nodiscard]] ICONS_API bool LoadAtlasPixels(
    const char* pathUtf8,
    uint32_t* outWidth,
    uint32_t* outHeight,
    uint32_t* outTierPx,
    uint8_t** outRgba,
    uint32_t* outRgbaSize,
    char* errorBuf,
    size_t errorBufSize);
ICONS_API void FreeAtlasPixels(uint8_t* rgba);

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

/// POD icon meta entry for safe cross-DLL consumption (no std::string / STL ownership transfer).
struct IconMetaFlatEntry {
    uint64_t nameHash = 0;
    uint32_t tierPx = 0;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    uint8_t flags = 0;
    char name[96] = {};
};

struct IconMetaFlatList {
    IconMetaFlatEntry* entries = nullptr;
    uint32_t count = 0;
};

/// Load meta into a malloc'd POD array owned by Icons.dll. Pair with FreeFlatList.
/// Returns false on failure; optional errorBuf receives a NUL-terminated message.
[[nodiscard]] ICONS_API bool LoadFlatList(
    const char* pathUtf8,
    IconMetaFlatList& outList,
    char* errorBuf = nullptr,
    size_t errorBufSize = 0);
ICONS_API void FreeFlatList(IconMetaFlatList& list);

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
