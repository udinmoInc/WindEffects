
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

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
