#include "WindEffects/Platform.h"
#include "WindEffects/Runtime/CoreSDK.h"
#include "Platform/PlatformSDK.h"
#include "Editor.h"

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#include "../Windows/resource.h"
#endif

namespace {

void SetWorkingDirectoryToExecutable() {
    auto& platform = we::platform::Platform::Get();
    const std::string exeDir = platform.GetExecutableDirectory();
    if (!exeDir.empty() && platform.SetCurrentWorkingDirectory(exeDir)) {
        HE_INFO("[Startup] Working directory set to executable folder.");
    } else {
        HE_INFO("[Startup] Could not resolve executable path; asset loading uses process CWD.");
    }
}

void ConfigureModuleSearchPath() {
    we::core::ConfigureModuleSearchPaths();
}

std::optional<std::filesystem::path> ParseProjectArgument(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "-project" || arg == "--project") && i + 1 < argc) {
            return std::filesystem::path(argv[i + 1]);
        }
    }
    return std::nullopt;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        we::platform::InitializeLogging();

        auto& platform = we::platform::Platform::Initialize({
            .appName = "WindEffects Editor",
            .highDpiAware = true,
            .enableRawInput = true,
            .enableGamepad = true,
            .enableDiagnostics = true,
        });
        const auto projectPath = ParseProjectArgument(argc, argv);
        if (projectPath) {
            const auto projectRoot = projectPath->parent_path();
            if (!projectRoot.empty() && platform.SetCurrentWorkingDirectory(projectRoot.string())) {
                HE_INFO("[Startup] Working directory set to project root: " + projectRoot.string());
            } else {
                SetWorkingDirectoryToExecutable();
            }
            HE_INFO("[Startup] Opening project: " + projectPath->string());
        } else {
            SetWorkingDirectoryToExecutable();
        }
        ConfigureModuleSearchPath();

        HE_INFO("[Startup] === WindEffects Editor bootstrap begin ===");
        HE_INFO(std::string("[Startup] Platform backend: ") + platform.GetName());
        if (platform.GetCapabilities().SupportsHighDPI()) {
            HE_INFO("[Startup] Platform capabilities: HighDPI enabled.");
        }

        auto& moduleManager = we::core::ModuleManager::Get();

        HE_INFO("[Startup] Loading editor feature modules...");
        const char* modules[] = {
            "WindEffects-KindUIFramework",
            "WindEffects-MainFrame",
            "WindEffects-Viewport",
            "WindEffects-ContentBrowser",
            "WindEffects-WorldOutliner",
            "WindEffects-PropertyEditor",
            "WindEffects-Details",
            "WindEffects-Toolbar",
            "WindEffects-Menus",
            "WindEffects-ToolsPanel",
            "WindEffects-PlaceActors",
            "WindEffects-Environment",
            "WindEffects-TerrainEditor",
        };
        for (const char* mod : modules) {
            if (!moduleManager.LoadModule(mod)) {
                HE_ERROR(std::string("[Startup] Failed to load module: ") + mod);
            } else {
                HE_INFO(std::string("[Startup]   Loaded module: ") + mod);
            }
        }

        HE_INFO("[Startup] Engine successfully initialized and modules loaded.");

        const auto windowResult = platform.CreateWindow({
            .title = "WindEffects Editor",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .maximized = true,
            .borderless = true,
            .visible = true,
            .highDpi = true,
            .acceptDropFiles = true,
        });
        if (!windowResult) {
            throw std::runtime_error(
                std::string("Failed to create platform window: ") + windowResult.error.message);
        }
        const we::platform::WindowId window = *windowResult;
        HE_INFO("[Startup] Platform window created (1280x720, maximized, borderless).");

        (void)platform.ApplyWindowChrome(window, {
            .roundedCorners = true,
            .borderColorRgb = 0x303030,
        });
#if defined(_WIN32)
        (void)platform.SetWindowIcon(window, IDI_ICON1);
#endif

        we::programs::editor::Editor editor(window);
        editor.Run();

        (void)platform.DestroyWindow(window);
        we::platform::Platform::Shutdown();
        HE_INFO("[Startup] === WindEffects Editor shutdown complete ===");
    } catch (const std::exception& e) {
        WE_LOG_CRITICAL(we::LogCategory::Crash.data(), std::string("Fatal exception: ") + e.what());
        we::runtime::core::Logger::ReportError("Fatal Exception", e.what(), true);
        return -1;
    }

    return 0;
}
