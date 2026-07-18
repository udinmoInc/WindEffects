#include "Importers/BuiltinImporters.h"

#include "ImportHelpers.h"
#include "AssetImporter/AssetPackage.h"

#include "Text/Import/FontImporter.h"
#include "Text/Assets/FontAsset.h"

#include <algorithm>
#include <fstream>

namespace we::runtime::assetimporter {
namespace {

class FontImporterAdapter final : public IAssetImporter {
public:
    [[nodiscard]] std::string_view GetImporterId() const override { return "font.ttf"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "Font Importer"; }
    [[nodiscard]] AssetKind GetAssetKind() const override { return AssetKind::Font; }
    [[nodiscard]] std::vector<std::string> GetSupportedExtensions() const override {
        return { ".ttf", ".otf", ".ttc" };
    }
    [[nodiscard]] int GetPriority() const override { return 100; }

    [[nodiscard]] bool CanImport(const std::filesystem::path& sourcePath) const override {
        std::string ext = sourcePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return ext == ".ttf" || ext == ".otf" || ext == ".ttc";
    }

    [[nodiscard]] ImportResult Import(
        const ImportRequest& request,
        const ImportProgressCallback& progress) const override {
        ImportResult result{};
        result.jobId = request.existingGuid.IsNil() ? AssetGuid::Generate() : request.existingGuid;

        if (!std::filesystem::exists(request.sourcePath)) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "source.missing",
                "Font source does not exist.", request.sourcePath.string());
            return result;
        }

        detail::ReportProgress(progress, result.jobId, 0.15f, "CreateImporter");
        auto importer = we::runtime::text::importing::CreateFontImporter(request.sourcePath);
        if (!importer) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "font.unsupported",
                "No font importer for this file type.", request.sourcePath.string());
            return result;
        }

        we::runtime::text::importing::ImportOptions options{};
#if WE_HAS_NLOHMANN_JSON
        if (request.settings.contains("bakeSizePx")) {
            options.bakeSizePx = request.settings.value("bakeSizePx", 18.0f);
        }
        if (request.settings.contains("charset")) {
            options.charset = request.settings.value("charset", std::string{ "basic" });
        }
#endif

        detail::ReportProgress(progress, result.jobId, 0.4f, "ImportFont");
        auto fontResult = importer->Import(request.sourcePath, options);
        if (!fontResult.ok) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "font.import_failed",
                fontResult.error.message.empty() ? "Font import failed." : fontResult.error.message,
                request.sourcePath.string());
            return result;
        }

        const auto cookedPath = detail::MakeOutputPath(request, AssetKind::Font, ".wefont");
        detail::ReportProgress(progress, result.jobId, 0.75f, "WriteWeFont");
        auto writeResult = we::runtime::text::assets::FontAssetWriter::WriteToFile(fontResult.value, cookedPath);
        if (!writeResult.ok) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "font.write_failed",
                writeResult.error.message.empty() ? "Failed to write .wefont." : writeResult.error.message,
                cookedPath.string());
            return result;
        }

        auto metadata = detail::BuildBaseMetadata(request, AssetKind::Font, GetImporterId(), "0.1.0");
        metadata.guid = result.jobId;
        const auto metaPath = cookedPath.parent_path() / (cookedPath.stem().string() + ".wemeta");
        (void)SaveMetadataJson(metaPath, metadata);

        result.success = true;
        result.asset.metadata = std::move(metadata);
        result.asset.cookedPath = cookedPath;
        result.asset.metadataPath = metaPath;
        result.asset.cookedBytes = std::filesystem::exists(cookedPath)
            ? static_cast<uint64_t>(std::filesystem::file_size(cookedPath))
            : 0;
        detail::ReportProgress(progress, result.jobId, 1.0f, "Completed");
        return result;
    }
};

} // namespace

std::shared_ptr<IAssetImporter> CreateFontImporterAdapter() {
    return std::make_shared<FontImporterAdapter>();
}

} // namespace we::runtime::assetimporter
