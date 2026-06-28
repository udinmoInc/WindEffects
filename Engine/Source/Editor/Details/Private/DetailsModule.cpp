
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class DetailsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "DetailsModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "DetailsModule shutdown\n";
    }
};

IMPLEMENT_MODULE(DetailsModule, WindEffects_Details)
