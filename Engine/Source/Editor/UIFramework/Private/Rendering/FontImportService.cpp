#include "Rendering/FontImportService.h"

#include "Core/Logger.h"
#include "Text/Assets/FontAsset.h"

namespace WindEffects::Editor::UI {

bool FontImportService::ImportFontFile(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputDirectory,
    const float bakeSizePx)
{
    return ImportFontFile(inputPath, outputDirectory, bakeSizePx, "basic");
}

bool FontImportService::ImportFontFile(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputDirectory,
    const float bakeSizePx,
    const std::string& charset)
{
    if (!std::filesystem::exists(inputPath)) {
        WE_LOG_ERROR("FontImport", "Input font not found: " + inputPath.string());
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(outputDirectory, ec);

    const auto stem = inputPath.stem().string();
    const auto outputPath = outputDirectory / (stem + ".wefont");

    we::runtime::text::importing::ImportOptions options;
    options.bakeSizePx = bakeSizePx;
    options.charset = charset;

    auto importer = we::runtime::text::importing::CreateFontImporter(inputPath);
    if (!importer) {
        return false;
    }

    const auto imported = importer->Import(inputPath, options);
    if (!imported.ok) {
        WE_LOG_ERROR("FontImport", imported.error.message);
        return false;
    }

    const auto written = we::runtime::text::assets::FontAssetWriter::WriteToFile(imported.value, outputPath);
    if (!written.ok) {
        WE_LOG_ERROR("FontImport", written.error.message);
        return false;
    }

    WE_LOG_INFO("FontImport", "Imported " + inputPath.string() + " -> " + outputPath.string());
    return true;
}

} // namespace WindEffects::Editor::UI
