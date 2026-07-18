#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetImporter/AssetMetadata.h"
#include "AssetImporter/Export.h"
#include "AssetImporter/Types.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::assetimporter {

struct ASSETIMPORTER_API ImportDiagnostic {
    ImportSeverity severity = ImportSeverity::Info;
    std::string code;
    std::string message;
    std::string contextPath;
};

struct ASSETIMPORTER_API ImportProgress {
    AssetGuid jobId{};
    ImportJobState state = ImportJobState::Queued;
    float fraction = 0.0f; // 0..1
    std::string stage;
    std::string detail;
};

struct ASSETIMPORTER_API ImportRequest {
    std::filesystem::path sourcePath;
    std::filesystem::path outputDirectory;
    std::string assetName;                 // optional; defaults to source stem
    AssetKind forcedKind = AssetKind::Unknown; // Unknown = auto-detect
    AssetGuid existingGuid{};              // non-nil = reimport in place
    bool generateThumbnail = true;
    bool overwriteExisting = true;
    bool copySourceToIntermediate = false;
#if WE_HAS_NLOHMANN_JSON
    nlohmann::json settings = nlohmann::json::object();
#endif
};

struct ASSETIMPORTER_API ImportedAsset {
    AssetMetadata metadata{};
    std::filesystem::path cookedPath;      // primary native asset (.weasset / .wefont / …)
    std::filesystem::path metadataPath;    // .wemeta sidecar when not embedded
    std::filesystem::path thumbnailPath;
    std::vector<std::filesystem::path> additionalOutputs;
    uint64_t cookedBytes = 0;
};

struct ASSETIMPORTER_API ImportResult {
    bool success = false;
    AssetGuid jobId{};
    ImportedAsset asset{};
    std::vector<ImportDiagnostic> diagnostics;

    [[nodiscard]] bool HasErrors() const noexcept;
    [[nodiscard]] std::string PrimaryErrorMessage() const;
};

using ImportProgressCallback = std::function<void(const ImportProgress&)>;
using ImportCompletedCallback = std::function<void(const ImportResult&)>;

} // namespace we::runtime::assetimporter
