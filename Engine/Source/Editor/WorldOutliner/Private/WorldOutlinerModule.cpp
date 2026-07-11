
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class WorldOutlinerModule : public we::core::IModuleInterface
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
