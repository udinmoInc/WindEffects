#include "AssetTools/AssetOperations.h"

#include "AssetImporter/AssetPackage.h"
#include "AssetImporter/Types.h"
#include "AssetProcessors/ProcessStage.h"
#include "Icons/Compile/IconCompiler.h"
#include "Icons/Compile/IconThemeAudit.h"
#include "Platform/Platform.h"

#include <chrono>
#include <thread>

namespace we::runtime::assettools {
namespace {

bool IsCookedAssetExtension(const std::filesystem::path& path) {
    const auto ext = path.extension().string();
    return ext == ".weasset" || ext == ".wefont" || ext == ".weiconatlas" || ext == ".wemeta";
}

std::optional<we::runtime::assetimporter::AssetMetadata> TryLoadMetadata(
    const std::filesystem::path& path) {
    if (path.extension() == ".wemeta") {
        return we::runtime::assetimporter::LoadMetadataJson(path);
    }
    if (path.extension() == ".weasset") {
        if (auto pkg = we::runtime::assetimporter::ReadAssetPackage(path)) {
            return pkg->metadata;
        }
    }
    const auto sidecar = path.parent_path() / (path.stem().string() + ".wemeta");
    if (std::filesystem::exists(sidecar)) {
        return we::runtime::assetimporter::LoadMetadataJson(sidecar);
    }
    return std::nullopt;
}

we::runtime::assetcooker::CookRequest MakeCookRequest(
    const std::filesystem::path& contentRoot,
    const std::filesystem::path& outputDirectory,
    const std::filesystem::path& packagePath,
    const std::string& platformName) {
    we::runtime::assetcooker::CookRequest cook{};
    cook.contentRoot = contentRoot;
    cook.outputDirectory = outputDirectory;
    cook.packagePath = packagePath;
    cook.platform = we::runtime::assetcooker::CookPlatformFromString(platformName);
    if (cook.platform == we::runtime::assetcooker::CookPlatform::Custom) {
        cook.customPlatformName = platformName;
    }
    return cook;
}

} // namespace

AssetOperations::AssetOperations(AssetServiceHost& host)
    : m_Host(host) {}

we::runtime::assetimporter::ImportResult AssetOperations::Import(
    const we::runtime::assetimporter::ImportRequest& request) {
    return m_Host.ImportService().ImportSync(request);
}

we::runtime::assetpipeline::PipelineResult AssetOperations::Build(
    const we::runtime::assetpipeline::PipelineRequest& request) {
    return m_Host.Pipeline().BuildSync(request);
}

we::runtime::assetpipeline::PipelineResult AssetOperations::BuildDirectory(
    const std::filesystem::path& sourceRoot,
    const std::filesystem::path& outputDirectory,
    const we::runtime::assetpipeline::PipelineRequest& defaults) {
    return m_Host.Pipeline().BuildDirectorySync(sourceRoot, outputDirectory, defaults);
}

we::runtime::assetpipeline::PipelineResult AssetOperations::Rebuild(
    const std::filesystem::path& sourceRoot,
    const std::filesystem::path& outputDirectory,
    const std::string& platformTarget) {
    we::runtime::assetpipeline::PipelineRequest defaults{};
    defaults.platformTarget = platformTarget;
    defaults.forceRebuild = true;
    defaults.generateThumbnail = true;
    defaults.enableOptimization = true;
    return m_Host.Pipeline().BuildDirectorySync(sourceRoot, outputDirectory, defaults);
}

we::runtime::assetpipeline::PipelineResult AssetOperations::RebuildGuid(
    const we::runtime::assetimporter::AssetGuid& guid) {
    return m_Host.Pipeline().RebuildDependentsSync(guid);
}

we::runtime::assetcooker::CookResult AssetOperations::Cook(
    const we::runtime::assetcooker::CookRequest& request,
    we::runtime::assetcooker::CookProgressCallback onProgress) {
    return m_Host.Cooker().CookSync(request, std::move(onProgress));
}

we::runtime::assetcooker::CookResult AssetOperations::Package(
    const std::filesystem::path& contentRoot,
    const std::filesystem::path& outputDirectory,
    const std::filesystem::path& packagePath,
    const std::string& platformName,
    we::runtime::assetcooker::CookProgressCallback onProgress) {
    auto cook = MakeCookRequest(contentRoot, outputDirectory, packagePath, platformName);
    return m_Host.Cooker().CookSync(cook, std::move(onProgress));
}

AssetOpResult AssetOperations::Validate() const {
    AssetOpResult out{};
    out.message = m_Host.Pipeline().GenerateValidationReport();
    out.success = !m_Host.Pipeline().GetDependencyGraph().HasCycles();
    out.assetCount = static_cast<uint32_t>(m_Host.Pipeline().GetCache().Size());
    if (!out.success) {
        out.message += "\nValidation failed: dependency cycles detected.\n";
    }
    return out;
}

AssetOpResult AssetOperations::Clean() {
    m_Host.Pipeline().GetCache().Clear();
    m_Host.Pipeline().GetDependencyGraph().Clear();
    (void)m_Host.Pipeline().GetCache().Save();
    AssetOpResult out{};
    out.success = true;
    out.message = "Asset pipeline cache and dependency graph cleared.";
    return out;
}

std::vector<AssetListEntry> AssetOperations::ListImporters() const {
    std::vector<AssetListEntry> entries;
    for (const auto& importer : m_Host.ImportService().GetRegistry().GetAll()) {
        if (!importer) {
            continue;
        }
        AssetListEntry e{};
        e.id = std::string(importer->GetImporterId());
        e.displayName = std::string(importer->GetDisplayName());
        e.kind = we::runtime::assetimporter::AssetKindToString(importer->GetAssetKind());
        for (const auto& ext : importer->GetSupportedExtensions()) {
            if (!e.detail.empty()) {
                e.detail += ' ';
            }
            e.detail += ext;
        }
        entries.push_back(std::move(e));
    }
    return entries;
}

std::vector<AssetListEntry> AssetOperations::ListProcessors() const {
    std::vector<AssetListEntry> entries;
    for (const auto& processor : m_Host.ProcessorService().GetRegistry().GetAll()) {
        if (!processor) {
            continue;
        }
        AssetListEntry e{};
        e.id = std::string(processor->GetProcessorId());
        e.displayName = std::string(processor->GetDisplayName());
        e.kind = std::string(we::runtime::assetprocessors::ProcessStageToString(processor->GetStage()));
        e.detail = "v=" + std::string(processor->GetVersion());
        entries.push_back(std::move(e));
    }
    return entries;
}

std::vector<AssetListEntry> AssetOperations::ListContent(const std::filesystem::path& contentRoot) const {
    std::vector<AssetListEntry> entries;
    if (!std::filesystem::exists(contentRoot)) {
        return entries;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(contentRoot)) {
        if (!entry.is_regular_file() || !IsCookedAssetExtension(entry.path())) {
            continue;
        }
        if (entry.path().extension() == ".wemeta") {
            continue;
        }
        AssetListEntry item{};
        item.detail = entry.path().string();
        if (auto meta = TryLoadMetadata(entry.path())) {
            item.id = meta->guid.ToString();
            item.displayName = meta->displayName;
            item.kind = we::runtime::assetimporter::AssetKindToString(meta->kind);
        } else {
            item.id = entry.path().stem().string();
            item.displayName = entry.path().filename().string();
            item.kind = "unknown";
        }
        entries.push_back(std::move(item));
    }
    return entries;
}

AssetInfoResult AssetOperations::Info(const std::filesystem::path& assetPath) const {
    AssetInfoResult out{};
    out.path = assetPath;
    auto meta = TryLoadMetadata(assetPath);
    if (!meta) {
        out.message = "Could not load asset metadata: " + assetPath.string();
        return out;
    }
    out.success = true;
    out.metadata = std::move(*meta);
    out.message = "ok";
    return out;
}

AssetDependenciesResult AssetOperations::Dependencies(
    const we::runtime::assetimporter::AssetGuid& guid) const {
    AssetDependenciesResult out{};
    out.guid = guid;
    out.dependencies = m_Host.Pipeline().GetDependencyGraph().GetDependencies(guid);
    out.dependents = m_Host.Pipeline().GetDependencyGraph().GetDependents(guid);
    if (auto cache = m_Host.Pipeline().GetCache().Find(guid)) {
        out.success = true;
        out.message = cache->sourcePath;
    } else if (!out.dependencies.empty() || !out.dependents.empty()) {
        out.success = true;
        out.message = "ok";
    } else {
        out.message = "Asset not found in pipeline cache/graph: " + guid.ToString();
    }
    return out;
}

AssetDependenciesResult AssetOperations::DependenciesFromPath(
    const std::filesystem::path& assetPath) const {
    auto meta = TryLoadMetadata(assetPath);
    if (!meta) {
        AssetDependenciesResult out{};
        out.message = "Could not resolve asset guid from: " + assetPath.string();
        return out;
    }
    auto out = Dependencies(meta->guid);
    if (out.success && out.message == "ok") {
        out.message = assetPath.string();
    }
    // Prefer metadata-declared dependencies when graph is empty.
    if (out.dependencies.empty() && !meta->dependencies.empty()) {
        out.dependencies = meta->dependencies;
        out.success = true;
        out.message = assetPath.string();
    }
    return out;
}

we::runtime::assetpipeline::PipelineResult AssetOperations::GenerateThumbnails(
    const std::filesystem::path& sourceRoot,
    const std::filesystem::path& outputDirectory,
    const std::string& platformTarget) {
    we::runtime::assetpipeline::PipelineRequest defaults{};
    defaults.platformTarget = platformTarget;
    defaults.generateThumbnail = true;
    defaults.forceRebuild = true;
    return m_Host.Pipeline().BuildDirectorySync(sourceRoot, outputDirectory, defaults);
}

AssetOpResult AssetOperations::CompileIcons(
    const std::filesystem::path& inputDirectory,
    const std::filesystem::path& outputDirectory) {
    AssetOpResult out{};
    we::runtime::icons::compiling::CompileOptions options{};
    options.inputDir = inputDirectory;
    options.outputDir = outputDirectory;

    auto compiler = we::runtime::icons::compiling::CreateIconCompiler();
    const auto result = compiler->Compile(options);
    if (!result.ok) {
        out.message = result.error.message;
        if (!result.error.context.empty()) {
            out.message += " (" + result.error.context + ")";
        }
        return out;
    }

    out.success = true;
    out.assetCount = result.value.iconCount;
    out.outputPath = result.value.metaPath;
    out.message = "Compiled " + std::to_string(result.value.iconCount) + " icons across "
        + std::to_string(result.value.tierCount) + " atlas tiers";

    const auto audit = we::runtime::icons::compiling::AuditIconAtlas(inputDirectory);
    if (!audit.ok) {
        m_Host.Emit(
            DiagnosticSeverity::Warning,
            "Tools",
            "IconAudit",
            "Icon atlas audit reported issues (see compiler output).");
    }
    return out;
}

int AssetOperations::Watch(
    const std::filesystem::path& sourceDirectory,
    const std::filesystem::path& outputDirectory,
    volatile bool* cancelFlag) {
    if (!we::platform::Platform::IsAvailable()) {
        m_Host.Emit(
            DiagnosticSeverity::Error,
            "Tools",
            "Watch.NoPlatform",
            "Platform is not initialized; cannot watch source directories.");
        return 2;
    }

    (void)outputDirectory; // Pipeline watch rebuilds into <source>/Cooked by default.
    if (!m_Host.Pipeline().StartWatching(sourceDirectory)) {
        m_Host.Emit(
            DiagnosticSeverity::Error,
            "Tools",
            "Watch.StartFailed",
            "Failed to start watching: " + sourceDirectory.string());
        return 2;
    }

    m_Host.Emit(
        DiagnosticSeverity::Info,
        "Tools",
        "Watch.Started",
        "Watching " + sourceDirectory.string());

    auto& platform = we::platform::Platform::Get();
    while (cancelFlag == nullptr || !*cancelFlag) {
        if (!platform.PollEvents()) {
            break;
        }
        m_Host.Pipeline().PollWatchers();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    m_Host.Pipeline().StopWatching();
    return 0;
}

} // namespace we::runtime::assettools
