
#include "Modules/IModuleInterface.hpp"
#include "Core/Logger.hpp"

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
