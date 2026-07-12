#include "Icons/Compile/IconCompiler.h"
#include "Core/BuildPaths.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void PrintUsage()
{
    std::cout
        << "we-icon-compile --input <atlas-source-dir> --output <compiled-atlas-dir>\n"
        << "  Compiles existing atlas_N.png + atlas_N.atlas pairs into .weiconatlas and icons.weiconmeta.\n";
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

    using namespace we::runtime::icons::compiling;

    if (HasArg(argc, argv, "--help") || HasArg(argc, argv, "-h")) {
        PrintUsage();
        return 0;
    }

    CompileOptions options;
    options.inputDir = GetArgValue(argc, argv, "--input");
    options.outputDir = GetArgValue(argc, argv, "--output");

    if (options.inputDir.empty() || options.outputDir.empty()) {
        PrintUsage();
        return 1;
    }

    auto compiler = CreateIconCompiler();
    const auto result = compiler->Compile(options);
    if (!result.ok) {
        std::cerr << "Icon compile failed: " << result.error.message;
        if (!result.error.context.empty()) {
            std::cerr << " (" << result.error.context << ")";
        }
        std::cerr << '\n';
        return 1;
    }

    std::cout << "Compiled " << result.value.iconCount << " icons across "
              << result.value.tierCount << " atlas tiers\n";
    std::cout << "Meta: " << result.value.metaPath.string() << '\n';
    for (const auto& atlasPath : result.value.atlasPaths) {
        std::cout << "Atlas: " << atlasPath.string() << '\n';
    }
    return 0;
}
