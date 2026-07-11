#include "WindEffects/Platform.h"
#include "WindEffects/Runtime/CoreSDK.h"
#include "Editor.h"

#include <SDL3/SDL.h>

#include <filesystem>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../Windows/Win32WindowIcon.h"
#include "../Windows/Win32WindowChrome.h"
#include "../Windows/resource.h"
#endif

namespace {

void SetWorkingDirectoryToExecutable() {
#if defined(_WIN32)
    wchar_t exePath[MAX_PATH]{};
    const DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        HE_INFO("[Startup] Could not resolve executable path; asset loading uses process CWD.");
        return;
    }
    std::filesystem::current_path(std::filesystem::path(exePath).parent_path());
    HE_INFO("[Startup] Working directory set to executable folder.");
#else
    HE_INFO("[Startup] Non-Windows platform: using process CWD for assets.");
#endif
}

void ConfigureModuleSearchPath() {
    we::core::ConfigureModuleSearchPaths();
}

#if defined(_WIN32)
void EnablePerMonitorDpiAwareness() {
    // Prevent Windows from bitmap-scaling the entire editor surface (global blur).
    // This must happen before creating the SDL window.
    if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        HE_INFO("[Startup] DPI awareness: Per-monitor v2 enabled.");
        return;
    }

    // Fallback for older environments where PMv2 context may not be available.
    if (SetProcessDPIAware()) {
        HE_INFO("[Startup] DPI awareness: System-aware fallback enabled.");
    } else {
        HE_WARN("[Startup] DPI awareness: unable to enable process DPI awareness.");
    }
}
#endif

} // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    try {
        we::platform::InitializeLogging();
        SetWorkingDirectoryToExecutable();
        ConfigureModuleSearchPath();

        HE_INFO("[Startup] === WindEffects Editor bootstrap begin ===");

        auto& moduleManager = we::core::ModuleManager::Get();

        HE_INFO("[Startup] Loading editor feature modules...");
        const char* modules[] = {
            "WindEffects-UIFramework",
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
        };
        for (const char* mod : modules) {
            if (!moduleManager.LoadModule(mod)) {
                HE_ERROR(std::string("[Startup] Failed to load module: ") + mod);
            } else {
                HE_INFO(std::string("[Startup]   Loaded module: ") + mod);
            }
        }

        HE_INFO("[Startup] Engine successfully initialized and modules loaded.");

#if defined(_WIN32)
        we::programs::windows::ConfigureSdlClassIcons(IDI_ICON1);
        EnablePerMonitorDpiAwareness();
#endif
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error("Failed to initialize SDL");
        }
        HE_INFO("[Startup] SDL video initialized.");

        SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(
            SDL_WINDOW_VULKAN |
            SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_HIGH_PIXEL_DENSITY |
            SDL_WINDOW_HIDDEN |
            SDL_WINDOW_BORDERLESS);

        SDL_Window* window = SDL_CreateWindow("WindEffects Editor", 1280, 720, window_flags);
        if (!window) {
            throw std::runtime_error("Failed to create SDL Window");
        }
        HE_INFO("[Startup] SDL window created (1280x720, hidden until UI is ready).");

        SDL_ShowWindow(window);
        SDL_MaximizeWindow(window);
#if defined(_WIN32)
        we::programs::windows::ConfigureBorderlessWindow(window);
        we::programs::windows::ApplyEmbeddedWindowIcon(window, IDI_ICON1);
#endif
        HE_INFO("[Startup] Window shown — swapchain will use visible pixel dimensions.");

        we::programs::editor::Editor editor(window);
        editor.Run();

        SDL_DestroyWindow(window);
        SDL_Quit();
        HE_INFO("[Startup] === WindEffects Editor shutdown complete ===");
    } catch (const std::exception& e) {
        WE_LOG_CRITICAL(we::LogCategory::Crash.data(), std::string("Fatal exception: ") + e.what());
        we::runtime::core::Logger::ReportError("Fatal Exception", e.what(), true);
        return -1;
    }

    return 0;
}
