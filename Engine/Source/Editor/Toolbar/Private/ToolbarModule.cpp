
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class ToolbarModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "ToolbarModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "ToolbarModule shutdown\n";
    }
};

IMPLEMENT_MODULE(ToolbarModule, WindEffects_Toolbar)
