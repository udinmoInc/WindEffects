
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class RHIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "RHIModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "RHIModule shutdown\n";
    }
};

IMPLEMENT_MODULE(RHIModule, WindEffects_RHI)
