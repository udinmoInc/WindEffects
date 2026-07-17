#include "Core/PluginManager.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include <filesystem>

namespace we::core {

PluginManager& PluginManager::Get() {
    static PluginManager instance;
    return instance;
}

void PluginManager::ScanAndLoadPlugins(const std::string& pluginDirectory) {
    if (!std::filesystem::exists(pluginDirectory)) {
        WE_LOG_DEBUG(we::LogCategory::Plugin.data(), "Plugin directory does not exist: " + pluginDirectory);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(pluginDirectory)) {
        if (entry.is_regular_file()) {
#ifdef _WIN32
            if (entry.path().extension() == ".dll") {
                std::string pathStr = entry.path().string();
                HMODULE handle = LoadLibraryA(pathStr.c_str());
                if (handle) {
                    WE_LOG_INFO(we::LogCategory::Plugin.data(), "Loaded plugin: " + entry.path().filename().string());
                    m_LoadedPlugins.push_back({ entry.path().filename().string(), handle });

                    // Look for InitializePlugin function
                    using InitFunc = void(*)();
                    InitFunc init = (InitFunc)GetProcAddress(handle, "InitializePlugin");
                    if (init) {
                        init();
                    } else {
                        WE_LOG_WARN(we::LogCategory::Plugin.data(), "InitializePlugin not found in " + entry.path().filename().string());
                    }
                } else {
                    WE_LOG_ERROR(we::LogCategory::Plugin.data(), "Failed to load plugin: " + pathStr);
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
