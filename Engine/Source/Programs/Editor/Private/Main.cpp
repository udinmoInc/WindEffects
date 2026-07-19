#include "WindEffects/Platform.h"
#include "WindEffects/Runtime/CoreSDK.h"
#include "Platform/PlatformSDK.h"
#include "Projects/EditorCommandLine.h"
#include "Projects/EngineContext.h"
#include "Editor.h"

#include <filesystem>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#include "../../Windows/Resources/resource.h"
#endif

namespace {

void SetWorkingDirectoryToExecutable() {
    auto& platform = we::platform::Platform::Get();
    const std::string exeDir = platform.GetExecutableDirectory();
    if (!exeDir.empty() && platform.SetCurrentWorkingDirectory(exeDir)) {
        HE_INFO("[Startup] Working directory set to executable folder (engine install).");
    } else {
        HE_INFO("[Startup] Could not resolve executable path; asset loading uses process CWD.");
    }
}

void ConfigureModuleSearchPath() {
    we::core::ConfigureModuleSearchPaths();
}

[[nodiscard]] bool NeedsWeLauncher(const we::projects::EditorCommandLine& commandLine) {
    return commandLine.forceProjectManager
        || commandLine.newProject
        || !commandLine.projectPath.has_value();
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

        // Engine CWD stays at the executable — never chdir into a project.
        SetWorkingDirectoryToExecutable();
        ConfigureModuleSearchPath();

        const auto commandLine = we::projects::ParseEditorCommandLine(argc, argv);
        we::projects::EngineContext::Get().Initialize(
            std::filesystem::path(platform.GetExecutableDirectory()));

        HE_INFO("[Startup] === WindEffects Editor bootstrap begin ===");
        HE_INFO(std::string("[Startup] Platform backend: ") + platform.GetName());

        // No project → hand off to WeLauncher.exe (never show an in-editor project UI).
        if (NeedsWeLauncher(commandLine)) {
            HE_INFO("[Startup] No project selected — launching WeLauncher.exe.");
            std::vector<std::string> launcherArgs;
            if (commandLine.newProject) {
                launcherArgs.push_back("-newproject");
            }
            const bool launched = we::programs::editor::Editor::LaunchWeLauncher(launcherArgs);
            we::projects::EngineContext::Get().Shutdown();
            we::platform::Platform::Shutdown();
            if (!launched) {
                HE_ERROR("[Startup] WeLauncher.exe not found next to the Editor.");
                we::runtime::core::Logger::ReportError(
                    "WeLauncher Missing",
                    "WeLauncher.exe was not found. Build the WeLauncher target or open a .weproj file.",
                    true);
                return 1;
            }
            return 0;
        }

        HE_INFO("[Startup] Project argument: " + commandLine.projectPath->string());
        if (commandLine.safeMode) {
            HE_INFO("[Startup] Safe mode enabled.");
        }
        if (commandLine.recoveryMode) {
            HE_INFO("[Startup] Recovery mode enabled.");
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
            "WindEffects-Prefab",
            "WindEffects-PrefabEditor",
            "WindEffects-Compilation",
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

        we::programs::editor::Editor editor(window, commandLine);
        editor.Run();

        we::projects::EngineContext::Get().Shutdown();
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
