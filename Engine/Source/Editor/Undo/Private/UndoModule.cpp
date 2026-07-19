#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class UndoModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "UndoModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE("Plugin", "UndoModule shutdown");
    }
};

IMPLEMENT_MODULE(UndoModule, WindEffects_Undo)
