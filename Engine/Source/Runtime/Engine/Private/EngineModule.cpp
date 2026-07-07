
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class EngineModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "EngineModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "EngineModule shutdown");
    }
};

IMPLEMENT_MODULE(EngineModule, WindEffects_Engine)
