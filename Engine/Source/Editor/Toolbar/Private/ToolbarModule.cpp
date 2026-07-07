
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class ToolbarModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "ToolbarModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "ToolbarModule shutdown");
    }
};

IMPLEMENT_MODULE(ToolbarModule, WindEffects_Toolbar)
