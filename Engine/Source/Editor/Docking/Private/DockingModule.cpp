
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class DockingModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "DockingModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "DockingModule shutdown\n";
    }
};

IMPLEMENT_MODULE(DockingModule, WindEffects_Docking)
