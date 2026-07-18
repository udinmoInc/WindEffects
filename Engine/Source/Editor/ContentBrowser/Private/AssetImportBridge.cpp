#include "ContentBrowser/AssetImportBridge.h"

#include "AssetCooker/IAssetCooker.h"
#include "AssetProcessors/IAssetProcessorService.h"
#include "Core/Logger.h"

namespace we::programs::editor {
namespace {

std::unique_ptr<we::runtime::assetimporter::IAssetImportService> g_ImportService;
std::unique_ptr<we::runtime::assetprocessors::IAssetProcessorService> g_ProcessorService;
std::unique_ptr<we::runtime::assetpipeline::IAssetPipeline> g_Pipeline;
std::unique_ptr<we::runtime::assetcooker::IAssetCooker> g_Cooker;

} // namespace

we::runtime::assetimporter::IAssetImportService& GetEditorAssetImportService() {
    if (!g_ImportService) {
        we::runtime::assetimporter::AssetImportServiceDependencies deps{};
        deps.engineVersion = "0.1.0";
        deps.onDiagnostic = [](const we::runtime::assetimporter::ImportDiagnostic& d) {
            if (d.severity == we::runtime::assetimporter::ImportSeverity::Warning) {
                HE_WARN("[AssetImport] " + d.message);
            } else if (d.severity == we::runtime::assetimporter::ImportSeverity::Error
                || d.severity == we::runtime::assetimporter::ImportSeverity::Fatal) {
                HE_ERROR("[AssetImport] " + d.message);
            } else {
                HE_INFO("[AssetImport] " + d.message);
            }
        };
        g_ImportService = we::runtime::assetimporter::CreateAssetImportService(std::move(deps));
    }
    return *g_ImportService;
}

we::runtime::assetpipeline::IAssetPipeline& GetEditorAssetPipeline() {
    if (!g_Pipeline) {
        if (!g_ProcessorService) {
            we::runtime::assetprocessors::AssetProcessorServiceDependencies deps{};
            deps.engineVersion = "0.1.0";
            g_ProcessorService = we::runtime::assetprocessors::CreateAssetProcessorService(std::move(deps));
        }
        we::runtime::assetpipeline::AssetPipelineDependencies deps{};
        deps.importService = &GetEditorAssetImportService();
        deps.processorService = g_ProcessorService.get();
        deps.engineVersion = "0.1.0";
        deps.onDiagnostic = [](const we::runtime::assetpipeline::PipelineDiagnostic& d) {
            if (d.severity == we::runtime::assetimporter::ImportSeverity::Warning) {
                HE_WARN("[AssetPipeline] " + d.message);
            } else if (d.severity == we::runtime::assetimporter::ImportSeverity::Error
                || d.severity == we::runtime::assetimporter::ImportSeverity::Fatal) {
                HE_ERROR("[AssetPipeline] " + d.message);
            }
        };
        g_Pipeline = we::runtime::assetpipeline::CreateAssetPipeline(std::move(deps));
    }
    return *g_Pipeline;
}

we::runtime::assetimporter::ImportResult ImportAssetToContent(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& contentOutputDirectory,
    const std::string& assetName) {
    we::runtime::assetimporter::ImportRequest request{};
    request.sourcePath = sourcePath;
    request.outputDirectory = contentOutputDirectory;
    request.assetName = assetName;
    request.overwriteExisting = true;
    request.generateThumbnail = true;
    return GetEditorAssetImportService().ImportSync(request);
}

we::runtime::assetpipeline::PipelineResult BuildAssetToContent(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& contentOutputDirectory,
    const std::string& assetName) {
    we::runtime::assetpipeline::PipelineRequest request{};
    request.import.sourcePath = sourcePath;
    request.import.outputDirectory = contentOutputDirectory;
    request.import.assetName = assetName;
    request.import.overwriteExisting = true;
    request.generateThumbnail = true;
    request.enableOptimization = true;
    return GetEditorAssetPipeline().BuildSync(request);
}

we::runtime::assetcooker::CookResult CookProjectContent(
    const std::filesystem::path& contentRoot,
    const std::filesystem::path& outputDirectory,
    const std::filesystem::path& packagePath,
    const std::string& platformName) {
    if (!g_Cooker) {
        we::runtime::assetcooker::AssetCookerDependencies deps{};
        deps.engineVersion = "0.1.0";
        deps.onDiagnostic = [](const we::runtime::assetcooker::CookDiagnostic& d) {
            if (d.severity == we::runtime::assetimporter::ImportSeverity::Warning) {
                HE_WARN("[AssetCooker] " + d.message);
            } else if (d.severity == we::runtime::assetimporter::ImportSeverity::Error
                || d.severity == we::runtime::assetimporter::ImportSeverity::Fatal) {
                HE_ERROR("[AssetCooker] " + d.message);
            } else {
                HE_INFO("[AssetCooker] " + d.message);
            }
        };
        g_Cooker = we::runtime::assetcooker::CreateAssetCooker(std::move(deps));
    }

    we::runtime::assetcooker::CookRequest cook{};
    cook.contentRoot = contentRoot;
    cook.outputDirectory = outputDirectory;
    cook.packagePath = packagePath;
    cook.platform = we::runtime::assetcooker::CookPlatformFromString(platformName);
    if (cook.platform == we::runtime::assetcooker::CookPlatform::Custom) {
        cook.customPlatformName = platformName;
    }
    return g_Cooker->CookSync(cook);
}

AssetBridgeOpResult BuildContentDirectory(
    const std::filesystem::path& sourceRoot,
    const std::filesystem::path& outputDirectory) {
    we::runtime::assetpipeline::PipelineRequest defaults{};
    const auto result = GetEditorAssetPipeline().BuildDirectorySync(sourceRoot, outputDirectory, defaults);
    AssetBridgeOpResult out{};
    out.success = result.success;
    out.message = result.success ? "ok" : result.PrimaryErrorMessage();
    out.rebuiltCount = result.rebuiltCount;
    out.skippedCount = result.skippedCount;
    out.failedCount = result.failedCount;
    out.assetCount = static_cast<uint32_t>(result.assets.size());
    out.outputPath = outputDirectory;
    return out;
}

AssetBridgeOpResult CookOrPackageContent(
    const std::filesystem::path& contentRoot,
    const std::filesystem::path& outputDirectory,
    const std::filesystem::path& packagePath,
    const std::string& platformName) {
    const auto result = CookProjectContent(contentRoot, outputDirectory, packagePath, platformName);
    AssetBridgeOpResult out{};
    out.success = result.success;
    out.message = result.success ? "ok" : result.PrimaryErrorMessage();
    out.assetCount = static_cast<uint32_t>(result.assets.size());
    out.outputPath = packagePath.empty() ? result.cookedRoot : result.packagePath;
    return out;
}

} // namespace we::programs::editor
