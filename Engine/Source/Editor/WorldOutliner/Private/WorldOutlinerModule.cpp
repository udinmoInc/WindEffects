
#include "Modules/IModuleInterface.hpp"
#include "Core/Logger.hpp"

class WorldOutlinerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "WorldOutlinerModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "WorldOutlinerModule shutdown");
    }
};

IMPLEMENT_MODULE(WorldOutlinerModule, WindEffects_WorldOutliner)
