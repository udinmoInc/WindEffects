#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class TerrainEditorModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "TerrainEditorModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "TerrainEditorModule shutdown");
    }
};

IMPLEMENT_MODULE(TerrainEditorModule, WindEffects_TerrainEditor)
