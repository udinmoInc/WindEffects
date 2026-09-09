// ==============================================================================
// WindEffects — Core — ModuleInitializer
// Internal implementation for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/ModuleInitializer.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

namespace we::core {

ModuleInitializerRegistry& ModuleInitializerRegistry::Get() {
    static ModuleInitializerRegistry instance;
    return instance;
}

void ModuleInitializerRegistry::Register(
    std::string_view moduleName,
    std::function<void()> onStartup,
    std::function<void()> onShutdown) {
    const std::string key(moduleName);
    if (m_Entries.find(key) == m_Entries.end()) {
        m_StartupOrder.push_back(key);
    }

    m_Entries[key] = ModuleInitializerEntry{
        std::move(onStartup),
        std::move(onShutdown),
    };
}

void ModuleInitializerRegistry::RunStartup(std::string_view moduleName) {
    const auto it = m_Entries.find(std::string(moduleName));
    if (it == m_Entries.end() || !it->second.onStartup) {
        return;
    }

    WE_LOG_TRACE(we::LogCategory::Plugin.data(),
        std::string("[ModuleInitializer] Startup: ") + std::string(moduleName));
    it->second.onStartup();
}

void ModuleInitializerRegistry::RunShutdown(std::string_view moduleName) {
    const auto it = m_Entries.find(std::string(moduleName));
    if (it == m_Entries.end() || !it->second.onShutdown) {
        return;
    }

    WE_LOG_TRACE(we::LogCategory::Plugin.data(),
        std::string("[ModuleInitializer] Shutdown: ") + std::string(moduleName));
    it->second.onShutdown();
}

} // namespace we::core
