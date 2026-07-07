#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class PlaceActorsModule : public IModuleInterface
{
public:
    void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "PlaceActorsModule started");
    }

    void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "PlaceActorsModule shutdown");
    }
};

IMPLEMENT_MODULE(PlaceActorsModule, WindEffects_PlaceActors)
