#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

class AssetProcessorsModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetProcessorsModule started");
    }
    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetProcessorsModule shutdown");
    }
};

IMPLEMENT_MODULE(AssetProcessorsModule, WindEffects_AssetProcessors)
