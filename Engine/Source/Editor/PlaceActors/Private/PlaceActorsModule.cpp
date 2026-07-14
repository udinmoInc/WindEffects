#include "Modules/IModuleInterface.h"
#include "PlaceActors/PlaceActorsRegistration.h"
#include "Core/Logger.h"

class PlaceActorsModule : public we::core::IModuleInterface
{
public:
    void StartupModule() override
    {
        we::programs::editor::EnsurePlaceActorsRegistered();
        WE_LOG_TRACE("Plugin", "PlaceActorsModule started");
    }

    void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "PlaceActorsModule shutdown");
    }
};

IMPLEMENT_MODULE(PlaceActorsModule, WindEffects_PlaceActors)
