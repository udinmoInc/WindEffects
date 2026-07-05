#include "Modules/IModuleInterface.hpp"
#include "Core/Logger.hpp"

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
