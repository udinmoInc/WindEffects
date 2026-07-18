#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include "Reflection/ITypeRegistry.h"

class ReflectionModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        we::runtime::reflection::ITypeRegistry& registry =
            we::runtime::reflection::GetTypeRegistry();
        // Freeze builtins + any static initializers for lock-free concurrent reads.
        if (!registry.IsSealed()) {
            registry.Seal();
        }
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ReflectionModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ReflectionModule shutdown");
    }
};

IMPLEMENT_MODULE(ReflectionModule, WindEffects_Reflection)
