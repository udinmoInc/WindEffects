#pragma once

#include "Core/Export.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <iostream>

namespace we::core {

class IModuleInterface {
public:
    virtual ~IModuleInterface() = default;
    virtual void StartupModule() = 0;
    virtual void ShutdownModule() = 0;
};

struct ModuleInfo {
    std::string Name;
    std::shared_ptr<IModuleInterface> Instance;
    bool bIsInitialized = false;
};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

class CORE_API ModuleManager {
public:
    static ModuleManager& Get();

    void RegisterModule(std::string_view name, std::shared_ptr<IModuleInterface> moduleInstance);
    void StartupAllModules();
    void ShutdownAllModules();

    [[nodiscard]] std::shared_ptr<IModuleInterface> GetModule(std::string_view name) const;

private:
    ModuleManager() = default;
    ~ModuleManager() = default;

    std::unordered_map<std::string, ModuleInfo> m_Modules;
    std::vector<std::string> m_LoadOrder; // To track startup order
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// Macro to easily register modules via static initialization
#define REGISTER_MODULE(ModuleClass, ModuleName) \
    namespace { \
        struct ModuleRegister_##ModuleClass { \
            ModuleRegister_##ModuleClass() { \
                we::core::ModuleManager::Get().RegisterModule(#ModuleName, std::make_shared<ModuleClass>()); \
            } \
        }; \
        static ModuleRegister_##ModuleClass g_Register_##ModuleClass; \
    }

} // namespace we::core
