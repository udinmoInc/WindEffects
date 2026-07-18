#include "Text/Assets/FontAsset.h"
#include "Text/Import/FontImporter.h"
#include "Core/BuildPaths.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void PrintUsage()
{
    std::cout
        << "we-font-import --input <font.ttf|otf|ttc> --output <font.wefont>\n"
        << "  [--size <px>] [--range <px>] [--charset basic|full]\n"
        << "  [--fallback <font.ttf>] [--face-index <n>]\n";
}

std::string GetArgValue(int argc, char** argv, const std::string& key)
{
    for (int i = 1; i + 1 < argc; ++i) {
        if (key == argv[i]) {
            return argv[i + 1];
        }
    }
    return {};
}

bool HasArg(int argc, char** argv, const std::string& key)
{
    for (int i = 1; i < argc; ++i) {
        if (key == argv[i]) {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    we::core::ConfigureModuleSearchPaths();

    using namespace we::runtime::text::assets;
    using namespace we::runtime::text::importing;

    if (HasArg(argc, argv, "--help") || HasArg(argc, argv, "-h")) {
        PrintUsage();
        return 0;
    }

    const std::string input = GetArgValue(argc, argv, "--input");
    const std::string output = GetArgValue(argc, argv, "--output");
    if (input.empty() || output.empty()) {
        PrintUsage();
        return 1;
    }

    ImportOptions options;
    if (const std::string size = GetArgValue(argc, argv, "--size"); !size.empty()) {
        options.bakeSizePx = std::stof(size);
    }
    if (const std::string range = GetArgValue(argc, argv, "--range"); !range.empty()) {
        options.msdfPixelRange = std::stof(range);
    }
    if (const std::string charset = GetArgValue(argc, argv, "--charset"); !charset.empty()) {
        options.charset = charset;
    }
    if (const std::string fallback = GetArgValue(argc, argv, "--fallback"); !fallback.empty()) {
        options.fallbackFontPath = fallback;
    }
    if (const std::string faceIndex = GetArgValue(argc, argv, "--face-index"); !faceIndex.empty()) {
        options.ttcFaceIndex = static_cast<uint32_t>(std::stoul(faceIndex));
    }

    const std::filesystem::path inputPath = input;
    const std::filesystem::path outputPath = output;
    auto importer = CreateFontImporter(inputPath);
    if (!importer) {
        std::cerr << "No importer available for: " << input << '\n';
        return 1;
    }

    const auto importResult = importer->Import(inputPath, options);
    if (!importResult.ok) {
        std::cerr << "Import failed: " << importResult.error.message;
        if (!importResult.error.context.empty()) {
            std::cerr << " (" << importResult.error.context << ')';
        }
        std::cerr << '\n';
        return 1;
    }

    const auto writeResult = FontAssetWriter::WriteToFile(importResult.value, outputPath);
    if (!writeResult.ok) {
        std::cerr << "Write failed: " << writeResult.error.message << '\n';
        return 1;
    }

    std::cout << "Imported " << input << " -> " << output
              << " glyphs=" << importResult.value.glyphs.size()
              << " atlas=" << importResult.value.atlasPages.front().width
              << 'x' << importResult.value.atlasPages.front().height << '\n';
    return 0;
}
