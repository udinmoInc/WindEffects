#pragma once

#include "AssetCooker/CookTypes.h"
#include "AssetImporter/ImportTypes.h"
#include "AssetPipeline/PipelineTypes.h"
#include "AssetTools/AssetServiceHost.h"
#include "AssetTools/Export.h"

#include <filesystem>
#include <string>
#include <vector>

namespace we::runtime::assettools {

/// Lightweight result for hosts that should not depend on every asset DLL type.
struct ASSETTOOLS_API AssetOpResult {
    bool success = false;
    std::string message;
    uint32_t rebuiltCount = 0;
    uint32_t skippedCount = 0;
    uint32_t failedCount = 0;
    uint32_t assetCount = 0;
    std::filesystem::path outputPath;
};

struct ASSETTOOLS_API AssetListEntry {
    std::string id;
    std::string displayName;
    std::string kind;
    std::string detail; // extensions or path
};

struct ASSETTOOLS_API AssetInfoResult {
    bool success = false;
    std::string message;
    we::runtime::assetimporter::AssetMetadata metadata{};
    std::filesystem::path path;
};

struct ASSETTOOLS_API AssetDependenciesResult {
    bool success = false;
    std::string message;
    we::runtime::assetimporter::AssetGuid guid{};
    std::vector<we::runtime::assetimporter::AssetGuid> dependencies;
    std::vector<we::runtime::assetimporter::AssetGuid> dependents;
};

/// Single source of truth for asset operations used by `we.exe`, Editor, and automation.
class ASSETTOOLS_API AssetOperations {
public:
    explicit AssetOperations(AssetServiceHost& host);

    [[nodiscard]] we::runtime::assetimporter::ImportResult Import(
        const we::runtime::assetimporter::ImportRequest& request);

    [[nodiscard]] we::runtime::assetpipeline::PipelineResult Build(
        const we::runtime::assetpipeline::PipelineRequest& request);

    [[nodiscard]] we::runtime::assetpipeline::PipelineResult BuildDirectory(
        const std::filesystem::path& sourceRoot,
        const std::filesystem::path& outputDirectory,
        const we::runtime::assetpipeline::PipelineRequest& defaults = {});

    [[nodiscard]] we::runtime::assetpipeline::PipelineResult Rebuild(
        const std::filesystem::path& sourceRoot,
        const std::filesystem::path& outputDirectory,
        const std::string& platformTarget = "Windows");

    [[nodiscard]] we::runtime::assetpipeline::PipelineResult RebuildGuid(
        const we::runtime::assetimporter::AssetGuid& guid);

    [[nodiscard]] we::runtime::assetcooker::CookResult Cook(
        const we::runtime::assetcooker::CookRequest& request,
        we::runtime::assetcooker::CookProgressCallback onProgress = {});

    [[nodiscard]] we::runtime::assetcooker::CookResult Package(
        const std::filesystem::path& contentRoot,
        const std::filesystem::path& outputDirectory,
        const std::filesystem::path& packagePath,
        const std::string& platformName = "Windows",
        we::runtime::assetcooker::CookProgressCallback onProgress = {});

    [[nodiscard]] AssetOpResult Validate() const;
    [[nodiscard]] AssetOpResult Clean();

    [[nodiscard]] std::vector<AssetListEntry> ListImporters() const;
    [[nodiscard]] std::vector<AssetListEntry> ListProcessors() const;
    [[nodiscard]] std::vector<AssetListEntry> ListContent(const std::filesystem::path& contentRoot) const;

    [[nodiscard]] AssetInfoResult Info(const std::filesystem::path& assetPath) const;
    [[nodiscard]] AssetDependenciesResult Dependencies(
        const we::runtime::assetimporter::AssetGuid& guid) const;
    [[nodiscard]] AssetDependenciesResult DependenciesFromPath(
        const std::filesystem::path& assetPath) const;

    [[nodiscard]] we::runtime::assetpipeline::PipelineResult GenerateThumbnails(
        const std::filesystem::path& sourceRoot,
        const std::filesystem::path& outputDirectory,
        const std::string& platformTarget = "Windows");

    /// Icon atlas compile (LibGDX → .weiconatlas). Used by build tooling via `we asset icons`.
    [[nodiscard]] AssetOpResult CompileIcons(
        const std::filesystem::path& inputDirectory,
        const std::filesystem::path& outputDirectory);

    /// Blocks until interrupted (Ctrl+C) or watch fails to start. Requires platform on the host.
    [[nodiscard]] int Watch(
        const std::filesystem::path& sourceDirectory,
        const std::filesystem::path& outputDirectory,
        volatile bool* cancelFlag = nullptr);

private:
    AssetServiceHost& m_Host;
};

} // namespace we::runtime::assettools
