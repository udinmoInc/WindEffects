#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class PrefabEditorModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "PrefabEditorModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE("Plugin", "PrefabEditorModule shutdown");
    }
};

IMPLEMENT_MODULE(PrefabEditorModule, WindEffects_PrefabEditor)
