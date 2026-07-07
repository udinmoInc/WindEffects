
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class DetailsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "DetailsModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "DetailsModule shutdown");
    }
};

IMPLEMENT_MODULE(DetailsModule, WindEffects_Details)
