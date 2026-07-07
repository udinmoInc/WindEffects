#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class ToolsPanelModule : public IModuleInterface {
public:
    void StartupModule() override {
        HE_INFO("ToolsPanelModule: Editor tools panel module started.");
    }

    void ShutdownModule() override {
        HE_INFO("ToolsPanelModule: Editor tools panel module shutdown.");
    }
};

extern "C" __declspec(dllexport) IModuleInterface* InitializeModule() {
    return new ToolsPanelModule();
}
