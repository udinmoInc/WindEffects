
#include "Modules/IModuleInterface.hpp"
#include "Core/Logger.hpp"

class PropertyEditorModule : public IModuleInterface
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
