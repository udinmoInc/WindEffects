
#include "Modules/IModuleInterface.hpp"
#include "Core/Logger.hpp"

class CoreUObjectModule : public IModuleInterface
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
