#include "ContentBrowser/AssetImportBridge.h"

#include "AssetTools/AssetTools.h"
#include "Core/Logger.h"

namespace we::programs::editor {
namespace {

std::unique_ptr<we::runtime::assettools::AssetServiceHost> g_AssetHost;

we::runtime::assettools::AssetServiceHost& GetEditorAssetHost() {
    if (!g_AssetHost) {
        we::runtime::assettools::AssetServiceHostConfig config{};
        config.engineVersion = "0.1.0";
        config.onDiagnostic = [](const we::runtime::assettools::AssetDiagnostic& d) {
            using we::runtime::assettools::DiagnosticSeverity;
            const auto line = "[" + d.source + "] " + d.message;
            if (d.severity == DiagnosticSeverity::Warning) {
                HE_WARN(line);
            } else if (d.severity == DiagnosticSeverity::Error
                || d.severity == DiagnosticSeverity::Fatal) {
                HE_ERROR(line);
            } else {
                HE_INFO(line);
            }
        };
        g_AssetHost = we::runtime::assettools::CreateAssetServiceHost(std::move(config));
    }
    return *g_AssetHost;
}

} // namespace

we::runtime::assetimporter::IAssetImportService& GetEditorAssetImportService() {
    return GetEditorAssetHost().ImportService();
}

we::runtime::assetpipeline::IAssetPipeline& GetEditorAssetPipeline() {
    return GetEditorAssetHost().Pipeline();
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
    we::runtime::assettools::AssetOperations ops(GetEditorAssetHost());
    return ops.Import(request);
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
    we::runtime::assettools::AssetOperations ops(GetEditorAssetHost());
    return ops.Build(request);
}

} // namespace we::programs::editor
