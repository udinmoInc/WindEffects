
#include "Modules/IModuleInterface.hpp"
#include "Core/Logger.hpp"

class ApplicationModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "ApplicationModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "ApplicationModule shutdown");
    }
};

IMPLEMENT_MODULE(ApplicationModule, WindEffects_Application)
