
#include "Modules/IModuleInterface.hpp"
#include "Core/Logger.hpp"

class DockingModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "DockingModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "DockingModule shutdown");
    }
};

IMPLEMENT_MODULE(DockingModule, WindEffects_Docking)
