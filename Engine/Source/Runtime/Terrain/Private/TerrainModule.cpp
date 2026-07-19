#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"
#include "Terrain/TerrainReflection.h"
#include "Reflection/ITypeRegistry.h"

class TerrainModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        we::runtime::terrain::RegisterTerrainReflection(
            we::runtime::reflection::GetTypeRegistry());
        WE_LOG_TRACE("Plugin", "TerrainModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "TerrainModule shutdown");
    }
};

IMPLEMENT_MODULE(TerrainModule, WindEffects_Terrain)
