
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class MainFrameModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "MainFrameModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "MainFrameModule shutdown");
    }
};

IMPLEMENT_MODULE(MainFrameModule, WindEffects_MainFrame)
