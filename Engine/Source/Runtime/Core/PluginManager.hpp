#pragma once

#include "Core/Export.hpp"

#include <string>
#include <vector>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

namespace we::core {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

class CORE_API PluginManager {
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

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace we::core
