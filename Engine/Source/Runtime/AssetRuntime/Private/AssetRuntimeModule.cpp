#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

class AssetRuntimeModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetRuntimeModule started");
    }
    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetRuntimeModule shutdown");
    }
};

IMPLEMENT_MODULE(AssetRuntimeModule, WindEffects_AssetRuntime)
