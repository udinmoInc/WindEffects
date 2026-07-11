
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class PropertyEditorModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "PropertyEditorModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "PropertyEditorModule shutdown");
    }
};

IMPLEMENT_MODULE(PropertyEditorModule, WindEffects_PropertyEditor)
