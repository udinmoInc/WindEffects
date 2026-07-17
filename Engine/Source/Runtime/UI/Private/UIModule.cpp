#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class UIModule : public we::core::IModuleInterface
{
public:
    void StartupModule() override
    {
        WE_LOG_TRACE("UI", "UIModule started");
    }

    void ShutdownModule() override
    {
        WE_LOG_TRACE("UI", "UIModule shutdown");
    }
};

IMPLEMENT_MODULE(UIModule, WindEffects_UI)
