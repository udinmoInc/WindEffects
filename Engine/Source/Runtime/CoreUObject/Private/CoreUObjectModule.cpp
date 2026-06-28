
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class CoreUObjectModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "CoreUObjectModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "CoreUObjectModule shutdown\n";
    }
};

IMPLEMENT_MODULE(CoreUObjectModule, WindEffects_CoreUObject)
