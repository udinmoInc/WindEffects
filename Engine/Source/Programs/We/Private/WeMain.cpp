#include "AssetTools/AssetTools.h"
#include "Core/BuildPaths.h"
#include "Core/Logger.h"
#include "Core/Paths.h"
#include "Platform/Capabilities.h"
#include "Platform/Platform.h"
#include "WindEffects/Platform.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

/// Write CLI results to the process stdout handle. Avoids CRT iostream breakage after asset DLLs load.
void WriteOut(std::string_view text) {
#if defined(_WIN32)
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != nullptr && out != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(
            out,
            text.data(),
            static_cast<DWORD>(text.size()),
            &written,
            nullptr);
        return;
    }
#endif
    std::cout << text;
    std::cout.flush();
}

void WriteOutLine(std::string_view text) {
    WriteOut(text);
    WriteOut("\n");
}

struct OutBuffer {
    std::ostringstream stream;
    ~OutBuffer() { WriteOut(stream.str()); }

    template <typename T>
    OutBuffer& operator<<(const T& value) {
        stream << value;
        return *this;
    }
};

void PrintRootUsage() {
    WriteOut(
        "WindEffects CLI (we)\n"
        "\n"
        "Asset commands (native engine modules):\n"
        "  we asset import <source> -out <dir> [-name <name>] [-kind <kind>]\n"
        "  we asset cook <contentRoot> -out <dir> [-pak <file.wepak>] [-platform <name>]\n"
        "  we asset package <contentRoot> -out <dir> -pak <file.wepak> [-platform <name>]\n"
        "  we asset rebuild <sourceRoot> -out <dir> [-platform <name>]\n"
        "  we asset validate\n"
        "  we asset clean\n"
        "  we asset list [importers|processors|content <root>]\n"
        "  we asset info <assetPath>\n"
        "  we asset dependencies <assetPath|guid>\n"
        "  we asset thumbnails <sourceRoot> -out <dir> [-platform <name>]\n"
        "  we asset watch <sourceRoot> -out <dir>\n"
        "  we asset icons --input <atlasDir> --output <outDir>\n"
        "\n"
        "Build / project commands are provided by IgniteBT via the we launcher scripts.\n");
}

void PrintAssetUsage() {
    PrintRootUsage();
}

std::string Opt(const std::vector<std::string>& args, size_t start, std::initializer_list<const char*> keys) {
    for (size_t i = start; i + 1 < args.size(); ++i) {
        for (const char* key : keys) {
            if (args[i] == key) {
                return args[i + 1];
            }
        }
    }
    return {};
}

std::filesystem::path Positional(const std::vector<std::string>& args, size_t start) {
    for (size_t i = start; i < args.size(); ++i) {
        if (!args[i].empty() && args[i][0] != '-') {
            return args[i];
        }
    }
    return {};
}

void PrintDiagnostic(const we::runtime::assettools::AssetDiagnostic& d) {
    const char* sev = "INFO";
    using we::runtime::assettools::DiagnosticSeverity;
    if (d.severity == DiagnosticSeverity::Warning) sev = "WARN";
    if (d.severity == DiagnosticSeverity::Error || d.severity == DiagnosticSeverity::Fatal) sev = "ERROR";
    std::cerr << "[" << sev << "][" << d.source << "] " << d.code << ": " << d.message << "\n";
}

std::unique_ptr<we::runtime::assettools::AssetServiceHost> CreateHost(bool needWatchPlatform) {
    we::runtime::assettools::AssetServiceHostConfig config{};
    config.engineVersion = "0.1.0";
    config.onDiagnostic = PrintDiagnostic;

    if (needWatchPlatform) {
        we::platform::PlatformDesc desc{};
        desc.services = static_cast<uint32_t>(
            we::platform::PlatformService::FileSystem
            | we::platform::PlatformService::DirectoryWatch
            | we::platform::PlatformService::Timers
            | we::platform::PlatformService::Diagnostics);
        auto& platform = we::platform::Platform::Initialize(desc);
        config.platform = &platform;
    }

    return we::runtime::assettools::CreateAssetServiceHost(std::move(config));
}

int CmdImport(we::runtime::assettools::AssetOperations& ops, const std::vector<std::string>& args) {
    const auto source = Positional(args, 0);
    if (source.empty()) {
        PrintAssetUsage();
        return 1;
    }
    we::runtime::assetimporter::ImportRequest request{};
    request.sourcePath = source;
    request.outputDirectory = Opt(args, 0, {"-out", "--out"});
    if (request.outputDirectory.empty()) {
        request.outputDirectory = "Imported";
    }
    request.assetName = Opt(args, 0, {"-name", "--name"});
    if (const auto kind = Opt(args, 0, {"-kind", "--kind"}); !kind.empty()) {
        request.forcedKind = we::runtime::assetimporter::AssetKindFromString(kind);
    }
    request.overwriteExisting = true;
    request.generateThumbnail = true;

    const auto result = ops.Import(request);
    if (!result.success) {
        std::cerr << "Import failed: " << result.PrimaryErrorMessage() << "\n";
        return 2;
    }
    OutBuffer{} << "Imported " << we::runtime::assetimporter::AssetKindToString(result.asset.metadata.kind)
              << " guid=" << result.asset.metadata.guid.ToString()
              << " -> " << result.asset.cookedPath.string() << "\n";
    return 0;
}

int CmdCookOrPackage(
    we::runtime::assettools::AssetOperations& ops,
    const std::vector<std::string>& args,
    bool requirePak) {
    const auto input = Positional(args, 0);
    if (input.empty()) {
        PrintAssetUsage();
        return 1;
    }
    auto outDir = Opt(args, 0, {"-out", "--out"});
    const auto pak = Opt(args, 0, {"-pak", "--pak"});
    const auto platform = Opt(args, 0, {"-platform", "--platform"});
    const std::string platformName = platform.empty() ? "Windows" : platform;

    if (requirePak && pak.empty()) {
        std::cerr << "package requires -pak <file.wepak>\n";
        return 1;
    }
    if (outDir.empty()) {
        outDir = we::core::PathService::ToUtf8(
            we::core::PathService::Get().CookedRootForPlatform(platformName));
    }

    we::runtime::assetcooker::CookRequest cook{};
    cook.contentRoot = input;
    cook.outputDirectory = outDir;
    cook.packagePath = pak;
    cook.platform = we::runtime::assetcooker::CookPlatformFromString(platformName);
    if (cook.platform == we::runtime::assetcooker::CookPlatform::Custom) {
        cook.customPlatformName = platformName;
    }

    const auto result = ops.Cook(cook, [](const we::runtime::assetcooker::CookProgress& p) {
        std::cerr << "\rCook " << static_cast<int>(p.fraction * 100.0f) << "% ("
                  << p.cookedAssets << "/" << p.totalAssets << ")" << std::flush;
    });
    std::cerr << "\n";
    if (!result.success) {
        std::cerr << (requirePak ? "Package" : "Cook") << " failed: " << result.PrimaryErrorMessage() << "\n";
        return 2;
    }
    OutBuffer{} << (requirePak ? "Packaged " : "Cooked ") << result.assets.size() << " assets -> "
              << (result.packagePath.empty() ? result.cookedRoot.string() : result.packagePath.string())
              << "\n";
    return 0;
}

int CmdRebuild(we::runtime::assettools::AssetOperations& ops, const std::vector<std::string>& args) {
    const auto input = Positional(args, 0);
    if (input.empty()) {
        PrintAssetUsage();
        return 1;
    }
    auto outDir = Opt(args, 0, {"-out", "--out"});
    if (outDir.empty()) {
        outDir = "Imported";
    }
    const auto platform = Opt(args, 0, {"-platform", "--platform"});
    const auto result = ops.Rebuild(input, outDir, platform.empty() ? "Windows" : platform);
    if (!result.success) {
        std::cerr << "Rebuild failed: " << result.PrimaryErrorMessage() << "\n";
        return 2;
    }
    OutBuffer{} << "Rebuild rebuilt=" << result.rebuiltCount
              << " skipped=" << result.skippedCount
              << " failed=" << result.failedCount << "\n";
    return 0;
}

int CmdValidate(we::runtime::assettools::AssetOperations& ops) {
    const auto result = ops.Validate();
    OutBuffer{} << result.message;
    if (!result.message.empty() && result.message.back() != '\n') {
        WriteOut("\n");
    }
    return result.success ? 0 : 2;
}

int CmdClean(we::runtime::assettools::AssetOperations& ops) {
    const auto result = ops.Clean();
    OutBuffer{} << result.message << "\n";
    return result.success ? 0 : 2;
}

int CmdList(we::runtime::assettools::AssetOperations& ops, const std::vector<std::string>& args) {
    std::string mode = "importers";
    std::filesystem::path contentRoot;
    if (!args.empty() && args[0][0] != '-') {
        mode = args[0];
        if (mode == "content") {
            contentRoot = Positional(args, 1);
            if (contentRoot.empty()) {
                std::cerr << "Usage: we asset list content <contentRoot>\n";
                return 1;
            }
        }
    }

    if (mode == "importers") {
        for (const auto& e : ops.ListImporters()) {
            OutBuffer{} << e.id << "  " << e.displayName << "  [" << e.kind << "]  exts=" << e.detail << "\n";
        }
        return 0;
    }
    if (mode == "processors") {
        for (const auto& e : ops.ListProcessors()) {
            OutBuffer{} << e.id << "  " << e.displayName << "  stage=" << e.kind << "  " << e.detail << "\n";
        }
        return 0;
    }
    if (mode == "content") {
        for (const auto& e : ops.ListContent(contentRoot)) {
            OutBuffer{} << e.id << "  " << e.displayName << "  [" << e.kind << "]  " << e.detail << "\n";
        }
        return 0;
    }

    std::cerr << "Unknown list mode: " << mode << "\n";
    return 1;
}

int CmdInfo(we::runtime::assettools::AssetOperations& ops, const std::vector<std::string>& args) {
    const auto path = Positional(args, 0);
    if (path.empty()) {
        PrintAssetUsage();
        return 1;
    }
    const auto info = ops.Info(path);
    if (!info.success) {
        std::cerr << info.message << "\n";
        return 2;
    }
    const auto& m = info.metadata;
    OutBuffer{} << "path=" << info.path.string() << "\n"
              << "guid=" << m.guid.ToString() << "\n"
              << "kind=" << we::runtime::assetimporter::AssetKindToString(m.kind) << "\n"
              << "name=" << m.displayName << "\n"
              << "source=" << m.sourcePath << "\n"
              << "importer=" << m.importerId << " @" << m.importerVersion << "\n"
              << "engine=" << m.engineVersion << "\n"
              << "created=" << m.createdUtc << "\n"
              << "modified=" << m.modifiedUtc << "\n"
              << "dependencies=" << m.dependencies.size() << "\n";
    return 0;
}

int CmdDependencies(we::runtime::assettools::AssetOperations& ops, const std::vector<std::string>& args) {
    const auto token = Positional(args, 0);
    if (token.empty()) {
        PrintAssetUsage();
        return 1;
    }

    we::runtime::assettools::AssetDependenciesResult deps{};
    if (auto guid = we::runtime::assetimporter::AssetGuid::Parse(token.string())) {
        deps = ops.Dependencies(*guid);
    } else {
        deps = ops.DependenciesFromPath(token);
    }
    if (!deps.success) {
        std::cerr << deps.message << "\n";
        return 2;
    }
    OutBuffer{} << "guid=" << deps.guid.ToString() << "\n";
    OutBuffer{} << "dependencies (" << deps.dependencies.size() << "):\n";
    for (const auto& g : deps.dependencies) {
        OutBuffer{} << "  " << g.ToString() << "\n";
    }
    OutBuffer{} << "dependents (" << deps.dependents.size() << "):\n";
    for (const auto& g : deps.dependents) {
        OutBuffer{} << "  " << g.ToString() << "\n";
    }
    return 0;
}

int CmdThumbnails(we::runtime::assettools::AssetOperations& ops, const std::vector<std::string>& args) {
    const auto input = Positional(args, 0);
    if (input.empty()) {
        PrintAssetUsage();
        return 1;
    }
    auto outDir = Opt(args, 0, {"-out", "--out"});
    if (outDir.empty()) {
        outDir = "Imported";
    }
    const auto platform = Opt(args, 0, {"-platform", "--platform"});
    const auto result = ops.GenerateThumbnails(input, outDir, platform.empty() ? "Windows" : platform);
    if (!result.success) {
        std::cerr << "Thumbnails failed: " << result.PrimaryErrorMessage() << "\n";
        return 2;
    }
    OutBuffer{} << "Thumbnails rebuilt=" << result.rebuiltCount
              << " skipped=" << result.skippedCount
              << " failed=" << result.failedCount << "\n";
    return 0;
}

int CmdWatch(we::runtime::assettools::AssetOperations& ops, const std::vector<std::string>& args) {
    const auto input = Positional(args, 0);
    if (input.empty()) {
        PrintAssetUsage();
        return 1;
    }
    auto outDir = Opt(args, 0, {"-out", "--out"});
    if (outDir.empty()) {
        outDir = (input / we::core::layout::kCooked).string();
    }
    OutBuffer{} << "Watching " << input.string() << " -> " << outDir << " (Ctrl+C to stop)\n";
    return ops.Watch(input, outDir);
}

int CmdIcons(we::runtime::assettools::AssetOperations& ops, const std::vector<std::string>& args) {
    const auto input = Opt(args, 0, {"--input", "-input"});
    const auto output = Opt(args, 0, {"--output", "-output"});
    if (input.empty() || output.empty()) {
        std::cerr << "Usage: we asset icons --input <atlasDir> --output <outDir>\n";
        return 1;
    }
    const auto result = ops.CompileIcons(input, output);
    if (!result.success) {
        std::cerr << "Icon compile failed: " << result.message << "\n";
        return 2;
    }
    OutBuffer{} << result.message << "\n";
    OutBuffer{} << "Meta: " << result.outputPath.string() << "\n";
    return 0;
}

int RunAssetCommand(const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "-h" || args[0] == "--help" || args[0] == "help") {
        PrintAssetUsage();
        return args.empty() ? 1 : 0;
    }

    const std::string cmd = args[0];
    const std::vector<std::string> rest(args.begin() + 1, args.end());
    const bool needWatch = (cmd == "watch");

    auto host = CreateHost(needWatch);
    we::runtime::assettools::AssetOperations ops(*host);

    if (cmd == "import") return CmdImport(ops, rest);
    if (cmd == "cook") return CmdCookOrPackage(ops, rest, false);
    if (cmd == "package") return CmdCookOrPackage(ops, rest, true);
    if (cmd == "rebuild") return CmdRebuild(ops, rest);
    if (cmd == "validate") return CmdValidate(ops);
    if (cmd == "clean") return CmdClean(ops);
    if (cmd == "list") return CmdList(ops, rest);
    if (cmd == "info") return CmdInfo(ops, rest);
    if (cmd == "dependencies") return CmdDependencies(ops, rest);
    if (cmd == "thumbnails") return CmdThumbnails(ops, rest);
    if (cmd == "watch") return CmdWatch(ops, rest);
    if (cmd == "icons") return CmdIcons(ops, rest);

    // Compatibility aliases from legacy we-asset-build / we-asset-import.
    if (cmd == "build") {
        const auto input = Positional(rest, 0);
        if (input.empty()) {
            PrintAssetUsage();
            return 1;
        }
        we::runtime::assetpipeline::PipelineRequest req{};
        req.import.sourcePath = input;
        req.import.outputDirectory = Opt(rest, 0, {"-out", "--out"});
        if (req.import.outputDirectory.empty()) {
            req.import.outputDirectory = "Imported";
        }
        const auto platform = Opt(rest, 0, {"-platform", "--platform"});
        req.platformTarget = platform.empty() ? "Windows" : platform;
        const auto result = ops.Build(req);
        if (!result.success) {
            std::cerr << "Build failed: " << result.PrimaryErrorMessage() << "\n";
            return 2;
        }
        OutBuffer{} << "Built assets=" << result.assets.size()
                  << " rebuilt=" << result.rebuiltCount
                  << " skipped=" << result.skippedCount << "\n";
        return 0;
    }
    if (cmd == "build-dir") {
        const auto input = Positional(rest, 0);
        if (input.empty()) {
            PrintAssetUsage();
            return 1;
        }
        auto outDir = Opt(rest, 0, {"-out", "--out"});
        if (outDir.empty()) {
            outDir = "Imported";
        }
        const auto platform = Opt(rest, 0, {"-platform", "--platform"});
        we::runtime::assetpipeline::PipelineRequest defaults{};
        defaults.platformTarget = platform.empty() ? "Windows" : platform;
        const auto result = ops.BuildDirectory(input, outDir, defaults);
        if (!result.success) {
            std::cerr << "Directory build failed: " << result.PrimaryErrorMessage() << "\n";
            return 2;
        }
        OutBuffer{} << "Directory build rebuilt=" << result.rebuiltCount
                  << " skipped=" << result.skippedCount
                  << " failed=" << result.failedCount << "\n";
        return 0;
    }
    if (cmd == "processors") {
        return CmdList(ops, {"processors"});
    }
    if (cmd == "report") {
        return CmdValidate(ops);
    }

    PrintAssetUsage();
    return 1;
}

} // namespace

int main(int argc, char* argv[]) {
    we::platform::InitializeLogging();
    we::core::ConfigureModuleSearchPaths();

    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc > 0 ? argc - 1 : 0));
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (args.empty() || args[0] == "-h" || args[0] == "--help" || args[0] == "help") {
        PrintRootUsage();
        return args.empty() ? 1 : 0;
    }

    if (args[0] == "asset") {
        return RunAssetCommand(std::vector<std::string>(args.begin() + 1, args.end()));
    }

    // Allow `we.exe import ...` when invoked directly without the `asset` prefix
    // (e.g. bootstrap manifest section [asset] already strips context).
    static const char* kAssetCmds[] = {
        "import", "cook", "package", "rebuild", "validate", "clean", "list", "info",
        "dependencies", "thumbnails", "watch", "icons", "build", "build-dir", "processors", "report"
    };
    for (const char* cmd : kAssetCmds) {
        if (args[0] == cmd) {
            return RunAssetCommand(args);
        }
    }

    std::cerr << "Unknown command: " << args[0] << "\n";
    std::cerr << "For build/project commands use the we launcher scripts (IgniteBT).\n";
    PrintRootUsage();
    return 1;
}
