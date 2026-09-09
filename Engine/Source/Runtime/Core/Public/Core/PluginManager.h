// ==============================================================================
// WindEffects — Core — PluginManager
// Public API surface for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"

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
