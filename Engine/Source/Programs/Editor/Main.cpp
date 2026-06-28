#include "Modules/ModuleManager.hpp"
#include "Editor.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <exception>
#include "Core/Logger.hpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    try {
        we::runtime::core::Logger::Init();
        std::cout << "WindEffects Engine Bootstrapping...\n";
        
        ModuleManager& ModuleManager = ModuleManager::Get();

        // 1. Load Editor Modules
        ModuleManager.LoadModule("WindEffects-Application");
        ModuleManager.LoadModule("WindEffects-MainFrame");
        ModuleManager.LoadModule("WindEffects-Docking");
        ModuleManager.LoadModule("WindEffects-Viewport");
        ModuleManager.LoadModule("WindEffects-ContentBrowser");
        ModuleManager.LoadModule("WindEffects-WorldOutliner");
        ModuleManager.LoadModule("WindEffects-PropertyEditor");
        ModuleManager.LoadModule("WindEffects-Details");
        ModuleManager.LoadModule("WindEffects-Toolbar");
        ModuleManager.LoadModule("WindEffects-Menus");

        // 2. Discover Plugins
        // std::vector<std::string> Plugins = PluginManager::DiscoverPlugins();
        // for (const auto& Plugin : Plugins) {
        //     ModuleManager.LoadModule(Plugin);
        // }
        
        std::cout << "Engine successfully initialized and modules loaded.\n";

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error("Failed to initialize SDL");
        }

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

        we::programs::editor::Editor editor(window);
        SDL_ShowWindow(window);
        editor.Run();
        
        SDL_DestroyWindow(window);
        SDL_Quit();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal Exception: " << e.what() << "\n";
        HE_ERROR(std::string("Fatal Exception: ") + e.what());
        return -1;
    }
    
    return 0;
}
