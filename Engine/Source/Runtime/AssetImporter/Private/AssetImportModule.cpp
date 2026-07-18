#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

class AssetImporterModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetImporterModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetImporterModule shutdown");
    }
};

IMPLEMENT_MODULE(AssetImporterModule, WindEffects_AssetImporter)
