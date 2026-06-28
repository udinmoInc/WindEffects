
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class MainFrameModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "MainFrameModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "MainFrameModule shutdown\n";
    }
};

IMPLEMENT_MODULE(MainFrameModule, WindEffects_MainFrame)
