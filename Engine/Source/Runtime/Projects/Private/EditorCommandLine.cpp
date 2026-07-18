#include "Projects/EditorCommandLine.h"

#include <filesystem>

namespace we::projects {
namespace {

std::filesystem::path NormalizeProjectPath(std::filesystem::path path) {
    std::error_code ec;
    path = std::filesystem::absolute(path, ec);
    if (ec) {
        return path;
    }
    return path.lexically_normal();
}

bool LooksLikeProjectFile(const std::filesystem::path& path) {
    const auto ext = path.extension().string();
    return ext == ".weproj" || ext == ".weproject";
}

} // namespace

EditorCommandLine ParseEditorCommandLine(int argc, char* argv[]) {
    EditorCommandLine result{};
    result.rawArgs.reserve(static_cast<size_t>(argc > 0 ? argc - 1 : 0));

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] ? argv[i] : "";
        result.rawArgs.push_back(arg);

        if (arg == "-projectmanager" || arg == "--projectmanager") {
            result.forceProjectManager = true;
            continue;
        }
        if (arg == "-newproject" || arg == "--newproject") {
            result.newProject = true;
            result.forceProjectManager = true;
            continue;
        }
        if (arg == "-safe" || arg == "--safe") {
            result.safeMode = true;
            continue;
        }
        if (arg == "-recovery" || arg == "--recovery") {
            result.recoveryMode = true;
            continue;
        }

        if (arg == "-project" || arg == "--project") {
            if (i + 1 < argc && argv[i + 1]) {
                result.projectPath = NormalizeProjectPath(argv[++i]);
                result.rawArgs.push_back(argv[i]);
            }
            continue;
        }

        constexpr const char kProjectEq[] = "-project=";
        constexpr const char kProjectEqLong[] = "--project=";
        if (arg.rfind(kProjectEq, 0) == 0) {
            result.projectPath = NormalizeProjectPath(arg.substr(sizeof(kProjectEq) - 1));
            continue;
        }
        if (arg.rfind(kProjectEqLong, 0) == 0) {
            result.projectPath = NormalizeProjectPath(arg.substr(sizeof(kProjectEqLong) - 1));
            continue;
        }

        // Positional .weproj (file association / double-click).
        if (!arg.empty() && arg[0] != '-' && LooksLikeProjectFile(arg)) {
            result.projectPath = NormalizeProjectPath(arg);
            continue;
        }
    }

    if (result.forceProjectManager) {
        result.projectPath.reset();
    }

    return result;
}

} // namespace we::projects
