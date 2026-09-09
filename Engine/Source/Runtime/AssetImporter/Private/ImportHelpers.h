// ==============================================================================
// WindEffects — AssetImporter — ImportHelpers
// Internal implementation for the AssetImporter module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "AssetImporter/ImportTypes.h"
#include "AssetImporter/IAssetImportService.h"

#include <filesystem>
#include <string>

namespace we::runtime::assetimporter::detail {

[[nodiscard]] std::string NowUtc();

[[nodiscard]] std::filesystem::path MakeOutputPath(
    const ImportRequest& request,
    AssetKind kind,
    std::string_view preferredExtension = {});

void ReportProgress(
    const ImportProgressCallback& cb,
    const AssetGuid& jobId,
    float fraction,
    std::string stage,
    std::string detail = {});

void AddDiagnostic(
    ImportResult& result,
    ImportSeverity severity,
    std::string code,
    std::string message,
    std::string contextPath = {});

[[nodiscard]] AssetMetadata BuildBaseMetadata(
    const ImportRequest& request,
    AssetKind kind,
    std::string_view importerId,
    std::string_view engineVersion);

[[nodiscard]] bool WriteBytes(
    const std::filesystem::path& path,
    const void* data,
    size_t size);

[[nodiscard]] std::vector<std::byte> ReadAllBytes(const std::filesystem::path& path);

} // namespace we::runtime::assetimporter::detail
