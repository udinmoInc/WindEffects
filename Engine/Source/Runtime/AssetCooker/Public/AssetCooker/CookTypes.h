#pragma once

#include "AssetCooker/CookPlatform.h"
#include "AssetCooker/Export.h"
#include "AssetImporter/AssetGuid.h"
#include "AssetImporter/ImportTypes.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace we::runtime::assetcooker {

struct ASSETCOOKER_API CookRequest {
    std::filesystem::path contentRoot;      // directory of .weasset / natives
    std::filesystem::path outputDirectory;  // Cooked/<Platform>/
    std::filesystem::path packagePath;      // optional .wepak output
    CookPlatform platform = CookPlatform::Windows;
    std::string customPlatformName;         // when platform == Custom
    bool compressPayloads = true;
    bool includeThumbnails = false;
    bool deterministic = true;              // stable ordering for reproducible packages
    std::vector<we::runtime::assetimporter::AssetGuid> onlyGuids; // empty = all
};

struct ASSETCOOKER_API CookDiagnostic {
    we::runtime::assetimporter::ImportSeverity severity =
        we::runtime::assetimporter::ImportSeverity::Info;
    std::string code;
    std::string message;
    std::string path;
};

struct ASSETCOOKER_API CookedAssetRecord {
    we::runtime::assetimporter::AssetGuid guid{};
    std::string relativePath;
    std::string contentType;
    uint64_t uncompressedSize = 0;
    uint64_t storedSize = 0;
    uint32_t offset = 0; // within .wepak payload region
};

struct ASSETCOOKER_API CookProgress {
    float fraction = 0.0f;
    std::string stage;
    uint32_t cookedAssets = 0;
    uint32_t totalAssets = 0;
};

struct ASSETCOOKER_API CookResult {
    bool success = false;
    CookPlatform platform = CookPlatform::Windows;
    std::filesystem::path cookedRoot;
    std::filesystem::path packagePath;
    std::vector<CookedAssetRecord> assets;
    std::vector<CookDiagnostic> diagnostics;
    uint64_t totalBytes = 0;

    [[nodiscard]] std::string PrimaryErrorMessage() const;
};

using CookProgressCallback = std::function<void(const CookProgress&)>;

} // namespace we::runtime::assetcooker
