#include "../ModuleManager.h"
#include "Core/DiagnosticMacros.h"

namespace we::core {

ModuleManager& ModuleManager::Get() {
    static ModuleManager instance;
    return instance;
}

void ModuleManager::RegisterModule(std::string_view name, std::shared_ptr<IModuleInterface> moduleInstance) {
    std::string strName{name};
    if (m_Modules.find(strName) == m_Modules.end()) {
        m_Modules[strName] = {strName, moduleInstance, false};
        m_LoadOrder.push_back(strName);
        WE_LOG_TRACE("Plugin", std::string("[ModuleManager] Registered module: ") + strName);
    }
}

void ModuleManager::StartupAllModules() {
    for (const auto& name : m_LoadOrder) {
        auto& moduleInfo = m_Modules[name];
        if (!moduleInfo.bIsInitialized) {
            WE_LOG_TRACE("Plugin", std::string("[ModuleManager] Starting up module: ") + name);
            moduleInfo.Instance->StartupModule();
            moduleInfo.bIsInitialized = true;
        }
    }
}

void ModuleManager::ShutdownAllModules() {
    // Shutdown in reverse order
    for (auto it = m_LoadOrder.rbegin(); it != m_LoadOrder.rend(); ++it) {
        auto& moduleInfo = m_Modules[*it];
        if (moduleInfo.bIsInitialized) {
            WE_LOG_TRACE("Plugin", std::string("[ModuleManager] Shutting down module: ") + *it);
            moduleInfo.Instance->ShutdownModule();
            moduleInfo.bIsInitialized = false;
        }
    }
}

std::shared_ptr<IModuleInterface> ModuleManager::GetModule(std::string_view name) const {
    auto it = m_Modules.find(std::string(name));
    if (it != m_Modules.end()) {
        return it->second.Instance;
    }
    return nullptr;
}

} // namespace we::core
