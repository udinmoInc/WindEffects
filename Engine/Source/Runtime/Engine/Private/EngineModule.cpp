
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class EngineModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "EngineModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "EngineModule shutdown\n";
    }
};

IMPLEMENT_MODULE(EngineModule, WindEffects_Engine)
