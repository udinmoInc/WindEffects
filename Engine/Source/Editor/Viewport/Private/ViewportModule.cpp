
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class ViewportModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "ViewportModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "ViewportModule shutdown");
    }
};

IMPLEMENT_MODULE(ViewportModule, WindEffects_Viewport)
