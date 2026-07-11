
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class SceneModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "SceneModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "SceneModule shutdown");
    }
};

IMPLEMENT_MODULE(SceneModule, WindEffects_Scene)
