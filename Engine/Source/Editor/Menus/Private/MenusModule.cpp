
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class MenusModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "MenusModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "MenusModule shutdown");
    }
};

IMPLEMENT_MODULE(MenusModule, WindEffects_Menus)
