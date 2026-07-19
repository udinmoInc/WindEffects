#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class PrefabModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "PrefabModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE("Plugin", "PrefabModule shutdown");
    }
};

IMPLEMENT_MODULE(PrefabModule, WindEffects_Prefab)
