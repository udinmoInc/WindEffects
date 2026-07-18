#include "AssetCooker/AssetCooker.h"
#include "AssetImporter/AssetImporter.h"
#include "AssetPipeline/AssetPipeline.h"
#include "AssetProcessors/AssetProcessors.h"
#include "AssetRuntime/AssetRuntime.h"
#include "Core/BuildPaths.h"
#include "Core/Logger.h"
#include "WindEffects/Platform.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void PrintUsage() {
    std::cout
        << "we-asset-build — WindEffects asset processing / cook pipeline\n"
        << "Usage:\n"
        << "  we-asset-build build <source> -out <dir> [-platform Windows|Linux|Android]\n"
        << "  we-asset-build build-dir <sourceRoot> -out <dir> [-platform ...]\n"
        << "  we-asset-build cook <contentRoot> -out <cookedDir> [-pak <file.wepak>] [-platform ...]\n"
        << "  we-asset-build processors\n"
        << "  we-asset-build report\n";
}

} // namespace

int main(int argc, char* argv[]) {
    we::platform::InitializeLogging();
    we::core::ConfigureModuleSearchPaths();

    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    const std::string cmd = argv[1];

    auto importService = we::runtime::assetimporter::CreateAssetImportService();
    auto processorService = we::runtime::assetprocessors::CreateAssetProcessorService();

    we::runtime::assetpipeline::AssetPipelineDependencies pipeDeps{};
    pipeDeps.importService = importService.get();
    pipeDeps.processorService = processorService.get();
    pipeDeps.engineVersion = "0.1.0";
    pipeDeps.onDiagnostic = [](const we::runtime::assetpipeline::PipelineDiagnostic& d) {
        const char* sev = "INFO";
        if (d.severity == we::runtime::assetimporter::ImportSeverity::Warning) sev = "WARN";
        if (d.severity == we::runtime::assetimporter::ImportSeverity::Error
            || d.severity == we::runtime::assetimporter::ImportSeverity::Fatal) sev = "ERROR";
        std::cerr << "[" << sev << "] " << d.code << ": " << d.message << "\n";
    };
    auto pipeline = we::runtime::assetpipeline::CreateAssetPipeline(std::move(pipeDeps));

    if (cmd == "processors") {
        for (const auto& p : processorService->GetRegistry().GetAll()) {
            std::cout << p->GetProcessorId() << "  " << p->GetDisplayName()
                      << "  stage=" << we::runtime::assetprocessors::ProcessStageToString(p->GetStage())
                      << "  v=" << p->GetVersion() << "\n";
        }
        return 0;
    }

    if (cmd == "report") {
        std::cout << pipeline->GenerateValidationReport();
        return 0;
    }

    std::string platform = "Windows";
    std::filesystem::path outDir;
    std::filesystem::path pakPath;
    std::filesystem::path input;

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "-out" || arg == "--out") && i + 1 < argc) {
            outDir = argv[++i];
        } else if ((arg == "-platform" || arg == "--platform") && i + 1 < argc) {
            platform = argv[++i];
        } else if ((arg == "-pak" || arg == "--pak") && i + 1 < argc) {
            pakPath = argv[++i];
        } else if (arg[0] != '-') {
            input = arg;
        }
    }

    if (cmd == "build") {
        if (input.empty()) {
            PrintUsage();
            return 1;
        }
        if (outDir.empty()) {
            outDir = "Imported";
        }
        we::runtime::assetpipeline::PipelineRequest req{};
        req.import.sourcePath = input;
        req.import.outputDirectory = outDir;
        req.platformTarget = platform;
        const auto result = pipeline->BuildSync(req);
        if (!result.success) {
            std::cerr << "Build failed: " << result.PrimaryErrorMessage() << "\n";
            return 2;
        }
        std::cout << "Built assets=" << result.assets.size()
                  << " rebuilt=" << result.rebuiltCount
                  << " skipped=" << result.skippedCount << "\n";
        return 0;
    }

    if (cmd == "build-dir") {
        if (input.empty()) {
            PrintUsage();
            return 1;
        }
        if (outDir.empty()) {
            outDir = "Imported";
        }
        we::runtime::assetpipeline::PipelineRequest defaults{};
        defaults.platformTarget = platform;
        const auto result = pipeline->BuildDirectorySync(input, outDir, defaults);
        if (!result.success) {
            std::cerr << "Directory build failed: " << result.PrimaryErrorMessage() << "\n";
            return 2;
        }
        std::cout << "Directory build rebuilt=" << result.rebuiltCount
                  << " skipped=" << result.skippedCount
                  << " failed=" << result.failedCount << "\n";
        return 0;
    }

    if (cmd == "cook") {
        if (input.empty()) {
            PrintUsage();
            return 1;
        }
        auto cooker = we::runtime::assetcooker::CreateAssetCooker();
        we::runtime::assetcooker::CookRequest cook{};
        cook.contentRoot = input;
        cook.outputDirectory = outDir.empty() ? std::filesystem::path("Cooked") / platform : outDir;
        cook.packagePath = pakPath;
        cook.platform = we::runtime::assetcooker::CookPlatformFromString(platform);
        if (cook.platform == we::runtime::assetcooker::CookPlatform::Custom) {
            cook.customPlatformName = platform;
        }
        const auto result = cooker->CookSync(cook, [](const we::runtime::assetcooker::CookProgress& p) {
            std::cerr << "\rCook " << static_cast<int>(p.fraction * 100.0f) << "% ("
                      << p.cookedAssets << "/" << p.totalAssets << ")" << std::flush;
        });
        std::cerr << "\n";
        if (!result.success) {
            std::cerr << "Cook failed: " << result.PrimaryErrorMessage() << "\n";
            return 2;
        }
        std::cout << "Cooked " << result.assets.size() << " assets -> "
                  << result.packagePath.string() << "\n";
        return 0;
    }

    PrintUsage();
    return 1;
}
