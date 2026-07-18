#pragma once

#include "AssetImporter/IAssetImportService.h"
#include "AssetPipeline/IAssetPipeline.h"
#include "ContentBrowser/Export.h"

#include <filesystem>
#include <memory>
#include <string>

namespace we::programs::editor {

/// Lazy shared import service for Content Browser / Editor UI.
[[nodiscard]] CONTENTBROWSER_API we::runtime::assetimporter::IAssetImportService& GetEditorAssetImportService();

/// Lazy shared asset pipeline (import → process → cache) for Editor tooling.
[[nodiscard]] CONTENTBROWSER_API we::runtime::assetpipeline::IAssetPipeline& GetEditorAssetPipeline();

/// Import a source file into the project content root (sync).
[[nodiscard]] CONTENTBROWSER_API we::runtime::assetimporter::ImportResult ImportAssetToContent(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& contentOutputDirectory,
    const std::string& assetName = {});

/// Full pipeline build (import + processors + incremental cache).
[[nodiscard]] CONTENTBROWSER_API we::runtime::assetpipeline::PipelineResult BuildAssetToContent(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& contentOutputDirectory,
    const std::string& assetName = {});

} // namespace we::programs::editor
