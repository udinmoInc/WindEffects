
#include "Modules/IModuleInterface.hpp"
#include "Core/Logger.hpp"

class RendererModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "RendererModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "RendererModule shutdown");
    }
};

IMPLEMENT_MODULE(RendererModule, WindEffects_Renderer)
