#include <SDL3/SDL.h>
#include <iostream>
#include "Core/Logger.hpp"
#include "CrashReporterApp.hpp"
#include <filesystem>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../Windows/Win32WindowIcon.hpp"
#include "../Windows/resource.h"
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    HE_INFO("[CrashReporter] === Starting WeCrashReporter.exe ===");
    HE_INFO("[CrashReporter] Step 1: Setting working directory");

#if defined(_WIN32)
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path workingDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::current_path(workingDir);
    HE_INFO("[CrashReporter] Working directory set to: " + workingDir.string());
#endif

    HE_INFO("[CrashReporter] Step 2: Initializing Logger");
    we::runtime::core::Logger::Init();
    HE_INFO("[CrashReporter] Logger initialized successfully");

#if defined(_WIN32)
    HE_INFO("[CrashReporter] Step 3: Configuring SDL class icons");
    we::programs::windows::ConfigureSdlClassIcons(IDI_ICON1);
    HE_INFO("[CrashReporter] SDL class icons configured");
#endif

    HE_INFO("[CrashReporter] Step 4: Initializing SDL");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        HE_ERROR("[CrashReporter] SDL_Init failed");
        return -1;
    }
    HE_INFO("[CrashReporter] SDL initialized successfully");

    HE_INFO("[CrashReporter] Step 5: Creating SDL window");
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_VULKAN |
        SDL_WINDOW_RESIZABLE |
        SDL_WINDOW_HIGH_PIXEL_DENSITY |
        SDL_WINDOW_HIDDEN // | SDL_WINDOW_BORDERLESS
    );

    SDL_Window* window = SDL_CreateWindow("WindEffects Crash Reporter", 1200, 760, window_flags);
    if (!window) {
        HE_ERROR("[CrashReporter] Failed to create SDL window");
        SDL_Quit();
        return -1;
    }
    HE_INFO("[CrashReporter] SDL window created successfully");

    HE_INFO("[CrashReporter] Step 6: Setting window minimum size");
    SDL_SetWindowMinimumSize(window, 1200, 760);
    HE_INFO("[CrashReporter] Window minimum size set");

    HE_INFO("[CrashReporter] Step 7: Showing window");
    SDL_ShowWindow(window);
    HE_INFO("[CrashReporter] Window shown");

#if defined(_WIN32)
    HE_INFO("[CrashReporter] Step 8: Applying embedded window icon");
    we::programs::windows::ApplyEmbeddedWindowIcon(window, IDI_ICON1);
    HE_INFO("[CrashReporter] Window icon applied");
#endif

    HE_INFO("[CrashReporter] Step 9: Creating CrashReporterApp");
    try {
        we::programs::crashreporter::CrashReporterApp app(window);
        HE_INFO("[CrashReporter] CrashReporterApp created successfully");
        
        HE_INFO("[CrashReporter] Step 10: Running main loop");
        app.Run();
        HE_INFO("[CrashReporter] Main loop completed");
    } catch (const std::exception& e) {
        HE_ERROR("[CrashReporter] Exception in CrashReporterApp: " + std::string(e.what()));
    } catch (...) {
        HE_ERROR("[CrashReporter] Unknown exception in CrashReporterApp");
    }
    
    HE_INFO("[CrashReporter] Step 11: Cleaning up - destroying window");
    SDL_DestroyWindow(window);
    HE_INFO("[CrashReporter] Window destroyed");

    HE_INFO("[CrashReporter] Step 12: Quitting SDL");
    SDL_Quit();
    HE_INFO("[CrashReporter] SDL quit");

    HE_INFO("[CrashReporter] === WeCrashReporter.exe exiting ===");
    return 0;
}
