#include "Modules/ModuleManager.hpp"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    try {
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

        // Note: The main loop would typically be driven by the Application module here.
        // e.g. IApplicationModule* App = (IApplicationModule*)ModuleManager.GetModule("WindEffects-Application");
        // App->Run();
        
        std::cout << "Engine successfully initialized and modules loaded.\n";
        
        // Simulating main loop for now...
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal Exception: " << e.what() << "\n";
        return -1;
    }
    
    return 0;
}
