
#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

class ContentBrowserModule : public we::core::IModuleInterface
{
public:
    void StartupModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ContentBrowserModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "ContentBrowserModule shutdown");
    }
};

IMPLEMENT_MODULE(ContentBrowserModule, WindEffects_ContentBrowser)
