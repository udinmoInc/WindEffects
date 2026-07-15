#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class TerrainModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "TerrainModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "TerrainModule shutdown");
    }
};

IMPLEMENT_MODULE(TerrainModule, WindEffects_Terrain)
