#include "Importers/BuiltinImporters.h"

#include "ImportHelpers.h"

#include "Icons/Compile/IconCompiler.h"

#include <algorithm>

namespace we::runtime::assetimporter {
namespace {

class IconImporterAdapter final : public IAssetImporter {
public:
    [[nodiscard]] std::string_view GetImporterId() const override { return "icon.atlas"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "Icon Atlas Compiler"; }
    [[nodiscard]] AssetKind GetAssetKind() const override { return AssetKind::IconAtlas; }
    [[nodiscard]] std::vector<std::string> GetSupportedExtensions() const override {
        return { ".atlas", ".svg" };
    }
    [[nodiscard]] int GetPriority() const override { return 90; }

    [[nodiscard]] bool CanImport(const std::filesystem::path& sourcePath) const override {
        std::string ext = sourcePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        // Directory of icons or a LibGDX .atlas file.
        if (std::filesystem::is_directory(sourcePath)) {
            return true;
        }
        return ext == ".atlas";
    }

    [[nodiscard]] ImportResult Import(
        const ImportRequest& request,
        const ImportProgressCallback& progress) const override {
        ImportResult result{};
        result.jobId = request.existingGuid.IsNil() ? AssetGuid::Generate() : request.existingGuid;

        std::filesystem::path inputDir = request.sourcePath;
        if (std::filesystem::is_regular_file(inputDir)) {
            inputDir = inputDir.parent_path();
        }
        if (!std::filesystem::exists(inputDir)) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "icon.input_missing",
                "Icon input directory does not exist.", inputDir.string());
            return result;
        }
        if (request.outputDirectory.empty()) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "output.missing",
                "Output directory is required.");
            return result;
        }

        detail::ReportProgress(progress, result.jobId, 0.2f, "CreateCompiler");
        auto compiler = we::runtime::icons::compiling::CreateIconCompiler();
        if (!compiler) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "icon.compiler_missing",
                "Icon compiler could not be created.");
            return result;
        }

        we::runtime::icons::compiling::CompileOptions options{};
        options.inputDir = inputDir;
        options.outputDir = request.outputDirectory;

        detail::ReportProgress(progress, result.jobId, 0.55f, "CompileAtlas");
        auto compileResult = compiler->Compile(options);
        if (!compileResult.ok) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "icon.compile_failed",
                compileResult.error.message.empty() ? "Icon compile failed." : compileResult.error.message,
                inputDir.string());
            return result;
        }

        auto metadata = detail::BuildBaseMetadata(request, AssetKind::IconAtlas, GetImporterId(), "0.1.0");
        metadata.guid = result.jobId;
#if WE_HAS_NLOHMANN_JSON
        metadata.custom["iconCount"] = compileResult.value.iconCount;
        metadata.custom["tierCount"] = compileResult.value.tierCount;
#endif

        const auto metaSidecar = request.outputDirectory
            / ((request.assetName.empty() ? std::string("icons") : request.assetName) + ".wemeta");
        (void)SaveMetadataJson(metaSidecar, metadata);

        result.success = true;
        result.asset.metadata = std::move(metadata);
        result.asset.cookedPath = compileResult.value.atlasPaths.empty()
            ? compileResult.value.metaPath
            : compileResult.value.atlasPaths.front();
        result.asset.metadataPath = metaSidecar;
        result.asset.additionalOutputs = compileResult.value.atlasPaths;
        if (!compileResult.value.metaPath.empty()) {
            result.asset.additionalOutputs.push_back(compileResult.value.metaPath);
        }
        detail::ReportProgress(progress, result.jobId, 1.0f, "Completed");
        return result;
    }
};

} // namespace

std::shared_ptr<IAssetImporter> CreateIconImporterAdapter() {
    return std::make_shared<IconImporterAdapter>();
}

void RegisterBuiltinImporters(ImporterRegistry& registry) {
    registry.Register(CreateTextureImporter());
    registry.Register(CreateFontImporterAdapter());
    registry.Register(CreateIconImporterAdapter());
    registry.Register(CreateMeshImporter());
    registry.Register(CreateAnimationImporter());
    registry.Register(CreateAudioImporter());
    registry.Register(CreateShaderImporter());
    registry.Register(CreateMaterialImporter());
    registry.Register(CreateSceneImporter());
    registry.Register(CreateRawBinaryImporter());
}

} // namespace we::runtime::assetimporter
