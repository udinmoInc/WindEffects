// ==============================================================================
// WindEffects — Core — ModuleManager
// Public API surface for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"
#include "Modules/IModuleInterface.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace we::core {

class CORE_API ModuleManager {
public:
    static ModuleManager& Get();

    IModuleInterface* LoadModule(const std::string& moduleName);
    bool IsModuleLoaded(const std::string& moduleName) const;
    void UnloadAllModules();

private:
    ModuleManager() = default;
    ~ModuleManager();

    struct ModuleData {
        void* handle = nullptr;
        IModuleInterface* moduleInterface = nullptr;
    };

    std::unordered_map<std::string, ModuleData> m_LoadedModules;
    std::vector<std::string> m_LoadOrder;
};

} // namespace we::core

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
