#pragma once

#include <string>
#include <vector>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

namespace we::core {

class PluginManager {
public:
    static PluginManager& Get();

    void ScanAndLoadPlugins(const std::string& pluginDirectory);
    void UnloadAllPlugins();

private:
    PluginManager() = default;
    ~PluginManager() = default;

    struct PluginHandle {
        std::string Name;
#ifdef _WIN32
        HMODULE Handle;
#else
        void* Handle;
#endif
    };

    std::vector<PluginHandle> m_LoadedPlugins;
};

} // namespace we::core
