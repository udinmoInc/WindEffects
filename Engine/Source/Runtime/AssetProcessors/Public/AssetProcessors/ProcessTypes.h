// ==============================================================================
// WindEffects — AssetProcessors — ProcessTypes
// Public API surface for the AssetProcessors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "AssetImporter/ImportTypes.h"
#include "AssetProcessors/Export.h"
#include "AssetProcessors/ProcessStage.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace we::runtime::assetprocessors {

struct ASSETPROCESSORS_API ProcessContext {
    we::runtime::assetimporter::ImportedAsset imported{};
    std::filesystem::path intermediateRoot;
    std::filesystem::path processedRoot;
    std::string engineVersion = "0.1.0";
    std::string platformTarget = "Windows"; // cook flavor hint for processors
    bool generateThumbnail = true;
    bool enableOptimization = true;
};

struct ASSETPROCESSORS_API ProcessDiagnostic {
    we::runtime::assetimporter::ImportSeverity severity =
        we::runtime::assetimporter::ImportSeverity::Info;
    std::string code;
    std::string message;
    std::string processorId;
};

struct ASSETPROCESSORS_API ProcessResult {
    bool success = false;
    we::runtime::assetimporter::ImportedAsset asset{};
    std::vector<ProcessDiagnostic> diagnostics;
    std::string processorFingerprint; // hash of processor id+version for cache keys
    bool skipped = false;             // incremental: nothing to do

    [[nodiscard]] bool HasErrors() const noexcept;
};

using ProcessProgressCallback = std::function<void(float fraction, std::string_view stage)>;

} // namespace we::runtime::assetprocessors
