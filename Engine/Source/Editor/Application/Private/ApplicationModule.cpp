
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class ApplicationModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "ApplicationModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "ApplicationModule shutdown\n";
    }
};

IMPLEMENT_MODULE(ApplicationModule, WindEffects_Application)
