#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class ViewportEditModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "ViewportEditModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE("Plugin", "ViewportEditModule shutdown");
    }
};

IMPLEMENT_MODULE(ViewportEditModule, WindEffects_ViewportEdit)
