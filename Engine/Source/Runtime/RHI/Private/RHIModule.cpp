
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class RHIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "RHIModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "RHIModule shutdown");
    }
};

IMPLEMENT_MODULE(RHIModule, WindEffects_RHI)
