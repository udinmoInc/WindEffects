
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class CoreUObjectModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "CoreUObjectModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "CoreUObjectModule shutdown");
    }
};

IMPLEMENT_MODULE(CoreUObjectModule, WindEffects_CoreUObject)
