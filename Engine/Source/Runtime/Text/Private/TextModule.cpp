#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class TextModule : public IModuleInterface
{
public:
    void StartupModule() override
    {
        WE_LOG_TRACE("Text", "TextModule started");
    }

    void ShutdownModule() override
    {
        WE_LOG_TRACE("Text", "TextModule shutdown");
    }
};

IMPLEMENT_MODULE(TextModule, WindEffects_Text)
