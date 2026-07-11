#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class ToolsPanelModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        HE_INFO("ToolsPanelModule: Editor tools panel module started.");
    }

    void ShutdownModule() override {
        HE_INFO("ToolsPanelModule: Editor tools panel module shutdown.");
    }
};

IMPLEMENT_MODULE(ToolsPanelModule, WindEffects_ToolsPanel)
