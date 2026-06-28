
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class RendererModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "RendererModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "RendererModule shutdown\n";
    }
};

IMPLEMENT_MODULE(RendererModule, WindEffects_Renderer)
