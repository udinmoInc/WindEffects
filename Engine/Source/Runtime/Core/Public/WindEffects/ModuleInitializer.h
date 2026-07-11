#pragma once

#include "Core/Export.h"

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace we::core {

struct ModuleInitializerEntry {
    std::function<void()> onStartup;
    std::function<void()> onShutdown;
};

class CORE_API ModuleInitializerRegistry {
public:
    static ModuleInitializerRegistry& Get();

    void Register(
        std::string_view moduleName,
        std::function<void()> onStartup,
        std::function<void()> onShutdown = {});

    void RunStartup(std::string_view moduleName);
    void RunShutdown(std::string_view moduleName);

private:
    ModuleInitializerRegistry() = default;

    std::unordered_map<std::string, ModuleInitializerEntry> m_Entries;
    std::vector<std::string> m_StartupOrder;
};

#define REGISTER_MODULE_INITIALIZER(ModuleNameStr, StartupExpr) \
    namespace { \
        struct ModuleInitializerEntry_##__LINE__ { \
            ModuleInitializerEntry_##__LINE__() { \
                we::core::ModuleInitializerRegistry::Get().Register( \
                    (ModuleNameStr), (StartupExpr)); \
            } \
        }; \
        static ModuleInitializerEntry_##__LINE__ g_ModuleInitializer_##__LINE__; \
    }

#define REGISTER_MODULE_LIFECYCLE(ModuleNameStr, StartupExpr, ShutdownExpr) \
    namespace { \
        struct ModuleInitializerEntry_##__LINE__ { \
            ModuleInitializerEntry_##__LINE__() { \
                we::core::ModuleInitializerRegistry::Get().Register( \
                    (ModuleNameStr), (StartupExpr), (ShutdownExpr)); \
            } \
        }; \
        static ModuleInitializerEntry_##__LINE__ g_ModuleInitializer_##__LINE__; \
    }

} // namespace we::core

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
