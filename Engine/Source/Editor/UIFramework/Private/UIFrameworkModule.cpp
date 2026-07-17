
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class UIFrameworkModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "UIFrameworkModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "UIFrameworkModule shutdown");
    }
};

IMPLEMENT_MODULE(UIFrameworkModule, WindEffects_KindUIFramework)
