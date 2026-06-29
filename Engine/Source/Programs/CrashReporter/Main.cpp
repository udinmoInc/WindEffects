#include <SDL3/SDL.h>
#include <iostream>
#include "Core/Logger.hpp"
#include "CrashReporterApp.hpp"
#include <filesystem>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
#if defined(_WIN32)
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::current_path(std::filesystem::path(exePath).parent_path());
#endif

    we::runtime::core::Logger::Init();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return -1;
    }

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_VULKAN |
        SDL_WINDOW_RESIZABLE |
        SDL_WINDOW_HIGH_PIXEL_DENSITY |
        SDL_WINDOW_HIDDEN // | SDL_WINDOW_BORDERLESS
    );

    SDL_Window* window = SDL_CreateWindow("WindEffects Crash Reporter", 1200, 760, window_flags);
    if (!window) {
        return -1;
    }
    SDL_SetWindowMinimumSize(window, 1200, 760);

    SDL_ShowWindow(window);

    {
        we::programs::crashreporter::CrashReporterApp app(window);
        app.Run();
    }
    
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
