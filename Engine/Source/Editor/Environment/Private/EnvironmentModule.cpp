
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class EnvironmentModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "EnvironmentModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "EnvironmentModule shutdown\n";
    }
};

IMPLEMENT_MODULE(EnvironmentModule, WindEffects_Environment)
