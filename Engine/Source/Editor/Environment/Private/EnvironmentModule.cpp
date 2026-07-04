
#include "Modules/IModuleInterface.hpp"
#include "Core/Logger.hpp"

class EnvironmentModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "EnvironmentModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "EnvironmentModule shutdown");
    }
};

IMPLEMENT_MODULE(EnvironmentModule, WindEffects_Environment)
