
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class ViewportModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "ViewportModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "ViewportModule shutdown\n";
    }
};

IMPLEMENT_MODULE(ViewportModule, WindEffects_Viewport)
