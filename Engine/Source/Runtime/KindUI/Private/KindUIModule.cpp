#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class KindUIModule : public we::core::IModuleInterface
{
public:
    void StartupModule() override
    {
        WE_LOG_TRACE("KindUI", "KindUIModule started");
    }

    void ShutdownModule() override
    {
        WE_LOG_TRACE("KindUI", "KindUIModule shutdown");
    }
};

IMPLEMENT_MODULE(KindUIModule, WindEffects_KindUI)
