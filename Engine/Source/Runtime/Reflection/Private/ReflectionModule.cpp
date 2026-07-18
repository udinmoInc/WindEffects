#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include "Reflection/ITypeRegistry.h"

class ReflectionModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        // Ensure the process-wide registry (and builtins) are initialized on module load.
        (void)we::runtime::reflection::GetTypeRegistry();
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ReflectionModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ReflectionModule shutdown");
    }
};

IMPLEMENT_MODULE(ReflectionModule, WindEffects_Reflection)
