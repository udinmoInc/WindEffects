#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class CompilationModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "CompilationModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE("Plugin", "CompilationModule shutdown");
    }
};

IMPLEMENT_MODULE(CompilationModule, WindEffects_Compilation)
