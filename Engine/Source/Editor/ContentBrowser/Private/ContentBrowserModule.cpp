
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class ContentBrowserModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "ContentBrowserModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "ContentBrowserModule shutdown");
    }
};

IMPLEMENT_MODULE(ContentBrowserModule, WindEffects_ContentBrowser)
