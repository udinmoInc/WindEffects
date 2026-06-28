#include "PluginManager.hpp"
#include <iostream>
#include <filesystem>

namespace we::core {

PluginManager& PluginManager::Get() {
    static PluginManager instance;
    return instance;
}

void PluginManager::ScanAndLoadPlugins(const std::string& pluginDirectory) {
    if (!std::filesystem::exists(pluginDirectory)) {
        std::cout << "[PluginManager] Plugin directory does not exist: " << pluginDirectory << "\n";
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(pluginDirectory)) {
        if (entry.is_regular_file()) {
#ifdef _WIN32
            if (entry.path().extension() == ".dll") {
                std::string pathStr = entry.path().string();
                HMODULE handle = LoadLibraryA(pathStr.c_str());
                if (handle) {
                    std::cout << "[PluginManager] Loaded Plugin: " << entry.path().filename().string() << "\n";
                    m_LoadedPlugins.push_back({ entry.path().filename().string(), handle });

                    // Look for InitializePlugin function
                    using InitFunc = void(*)();
                    InitFunc init = (InitFunc)GetProcAddress(handle, "InitializePlugin");
                    if (init) {
                        init();
                    } else {
                        std::cout << "[PluginManager] Warning: InitializePlugin not found in " << entry.path().filename().string() << "\n";
                    }
                } else {
                    std::cout << "[PluginManager] Failed to load plugin: " << pathStr << "\n";
                }
            }
#endif
        }
    }
}

void PluginManager::UnloadAllPlugins() {
    for (auto& plugin : m_LoadedPlugins) {
#ifdef _WIN32
        if (plugin.Handle) {
            FreeLibrary(plugin.Handle);
        }
#endif
    }
    m_LoadedPlugins.clear();
}

} // namespace we::core
