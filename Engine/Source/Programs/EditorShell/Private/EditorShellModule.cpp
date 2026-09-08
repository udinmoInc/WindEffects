#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class EditorShellModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "EditorShellModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "EditorShellModule shutdown");
    }
};

IMPLEMENT_MODULE(EditorShellModule, WindEffects_EditorShell)
