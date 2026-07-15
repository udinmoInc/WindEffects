#include "Modules/IModuleInterface.h"
#include "RHI/RHI.h"

#include "Core/Logger.h"

class RHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "RHIModule started");
    }

    void ShutdownModule() override {
        we::rhi::RHI::Shutdown();
        WE_LOG_TRACE("Plugin", "RHIModule shutdown");
    }
};

IMPLEMENT_MODULE(RHIModule, WindEffects_RHI)
