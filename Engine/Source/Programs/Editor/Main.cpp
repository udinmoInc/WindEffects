#include "Modules/ModuleManager.hpp"
#include "Editor.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <cstring>
#include <exception>
#include <filesystem>
#include "Core/Logger.hpp"
#include "Core/LogCategory.hpp"
#include "Core/BuildPaths.hpp"
#include "Renderer/AtmosphereValidation.hpp"
#include "Renderer/RenderPipelineInvestigator.hpp"
#include "Renderer/RenderForensics.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../Windows/Win32WindowIcon.hpp"
#include "../Windows/Win32WindowChrome.hpp"
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

} // namespace

int main(int argc, char* argv[]) {
    try {
        we::runtime::core::Logger::Init();
        SetWorkingDirectoryToExecutable();
        ConfigureModuleSearchPath();

        const auto validationSettings =
            we::runtime::renderer::AtmosphereValidation::ParseCommandLine(argc, argv);
        we::runtime::renderer::AtmosphereValidation::Get().Configure(validationSettings);
        const auto pipelineInvestigationSettings =
            we::runtime::renderer::RenderPipelineInvestigator::ParseCommandLine(argc, argv);
        auto pipelineSettings = pipelineInvestigationSettings;
        we::runtime::renderer::RenderPipelineInvestigator::ApplyConfigOverrides(pipelineSettings);
        if (!pipelineInvestigationSettings.skyIsolationStepFromCommandLine) {
            we::runtime::renderer::RenderPipelineInvestigator::ApplyPersistedOverrides(pipelineSettings);
        }
        // Command-line isolation/effect steps take precedence over function.ini and saved state.
        if (pipelineInvestigationSettings.skyIsolationStepFromCommandLine) {
            pipelineSettings.skyIsolationStep = pipelineInvestigationSettings.skyIsolationStep;
            pipelineSettings.enabled = pipelineInvestigationSettings.skyIsolationStep >= 0;
            pipelineSettings.logEveryFrame = pipelineInvestigationSettings.logEveryFrame;
            pipelineSettings.haltOnInvalid = pipelineInvestigationSettings.haltOnInvalid;
        }
        if (pipelineInvestigationSettings.effectStep >= 0) {
            pipelineSettings.effectStep = pipelineInvestigationSettings.effectStep;
            pipelineSettings.enabled = true;
        }
        we::runtime::renderer::RenderPipelineInvestigator::Get().Configure(pipelineSettings);
        auto forensicSettings = we::runtime::renderer::RenderForensics::DefaultEditorSettings();
        if (const char* forensicEnv = std::getenv("WE_RENDER_FORENSICS")) {
            if (forensicEnv[0] == '1' || std::strcmp(forensicEnv, "true") == 0) {
                forensicSettings.enabled = true;
                forensicSettings.enableGpuReadback = true;
                forensicSettings.logEveryFrame = true;
                forensicSettings.haltOnInvalid = true;
                forensicSettings.haltOnWhiteScreen = true;
            } else if (forensicEnv[0] == '0' || std::strcmp(forensicEnv, "false") == 0) {
                forensicSettings.enabled = false;
                forensicSettings.enableGpuReadback = false;
            }
        }
        if (pipelineSettings.enabled) {
            forensicSettings.logEveryFrame = pipelineSettings.logEveryFrame;
            forensicSettings.haltOnInvalid = pipelineSettings.haltOnInvalid;
        }
        we::runtime::renderer::RenderForensics::Get().Configure(forensicSettings);
        if (validationSettings.enabled) {
            HE_INFO("[Startup] Atmosphere validation mode enabled from command line.");
        }
        if (pipelineSettings.enabled) {
            HE_INFO("[Startup] Render pipeline debug mode enabled from command line.");
        }
        if (forensicSettings.enabled) {
            HE_INFO("[Startup] Render forensic diagnostics enabled (GPU readback every frame).");
        }

        HE_INFO("[Startup] === WindEffects Editor bootstrap begin ===");
        
        ModuleManager& ModuleManager = ModuleManager::Get();

        // 1. Load Editor Modules (runs REGISTER_EDITOR_PANEL static initializers)
        HE_INFO("[Startup] Loading editor feature modules...");
        const char* modules[] = {
            "WindEffects-Application",
            "WindEffects-MainFrame",
            "WindEffects-Docking",
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
            if (!ModuleManager.LoadModule(mod)) {
                HE_ERROR(std::string("[Startup] Failed to load module: ") + mod);
            } else {
                HE_INFO(std::string("[Startup]   Loaded module: ") + mod);
            }
        }
        
        HE_INFO("[Startup] Engine successfully initialized and modules loaded.");

#if defined(_WIN32)
        we::programs::windows::ConfigureSdlClassIcons(IDI_ICON1);
#endif
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error("Failed to initialize SDL");
        }
        HE_INFO("[Startup] SDL video initialized.");

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(
            SDL_WINDOW_VULKAN |
            SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_HIGH_PIXEL_DENSITY |
            SDL_WINDOW_HIDDEN |
            SDL_WINDOW_BORDERLESS
        );

        SDL_Window* window = SDL_CreateWindow("WindEffects Editor", 1280, 720, window_flags);
        if (!window) {
            throw std::runtime_error("Failed to create SDL Window");
        }
        HE_INFO("[Startup] SDL window created (1280x720, hidden until UI is ready).");

        // Show the window BEFORE Vulkan/swapchain creation so surface extent is valid.
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
