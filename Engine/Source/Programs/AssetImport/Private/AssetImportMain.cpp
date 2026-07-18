#include "AssetImporter/AssetImporter.h"
#include "Core/BuildPaths.h"
#include "Core/Logger.h"
#include "WindEffects/Platform.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void PrintUsage() {
    std::cout
        << "we-asset-import — WindEffects unified asset importer\n"
        << "Usage:\n"
        << "  we-asset-import <source> -out <dir> [-name <assetName>] [-kind <kind>]\n"
        << "  we-asset-import --list\n";
}

} // namespace

int main(int argc, char* argv[]) {
    we::platform::InitializeLogging();
    we::core::ConfigureModuleSearchPaths();

    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    using namespace we::runtime::assetimporter;

    if (std::string(argv[1]) == "--list" || std::string(argv[1]) == "-list") {
        std::cerr << "Creating asset import service...\n";
        auto service = CreateAssetImportService();
        if (!service) {
            std::cerr << "CreateAssetImportService returned null.\n";
            return 3;
        }
        const auto importers = service->GetRegistry().GetAll();
        std::cerr << "Registered importers: " << importers.size() << "\n";
        for (const auto& importer : importers) {
            if (!importer) {
                std::cerr << "(null importer)\n";
                continue;
            }
            std::cout << importer->GetImporterId() << "  "
                      << importer->GetDisplayName() << "  ["
                      << AssetKindToString(importer->GetAssetKind()) << "]  exts=";
            for (const auto& ext : importer->GetSupportedExtensions()) {
                std::cout << ext << " ";
            }
            std::cout << "\n";
        }
        std::cout.flush();
        return 0;
    }

    ImportRequest request{};
    request.sourcePath = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "-out" || arg == "--out") && i + 1 < argc) {
            request.outputDirectory = argv[++i];
        } else if ((arg == "-name" || arg == "--name") && i + 1 < argc) {
            request.assetName = argv[++i];
        } else if ((arg == "-kind" || arg == "--kind") && i + 1 < argc) {
            request.forcedKind = AssetKindFromString(argv[++i]);
        }
    }

    if (request.outputDirectory.empty()) {
        request.outputDirectory = std::filesystem::path("Imported");
    }

    AssetImportServiceDependencies deps{};
    deps.engineVersion = "0.1.0";
    deps.onDiagnostic = [](const ImportDiagnostic& d) {
        const char* sev = "INFO";
        if (d.severity == ImportSeverity::Warning) sev = "WARN";
        if (d.severity == ImportSeverity::Error || d.severity == ImportSeverity::Fatal) sev = "ERROR";
        std::cerr << "[" << sev << "] " << d.code << ": " << d.message << "\n";
    };

    auto service = CreateAssetImportService(std::move(deps));
    const auto result = service->ImportSync(request);
    if (!result.success) {
        std::cerr << "Import failed: " << result.PrimaryErrorMessage() << "\n";
        return 2;
    }

    std::cout << "Imported " << AssetKindToString(result.asset.metadata.kind)
              << " guid=" << result.asset.metadata.guid.ToString()
              << " -> " << result.asset.cookedPath.string() << "\n";
    return 0;
}
