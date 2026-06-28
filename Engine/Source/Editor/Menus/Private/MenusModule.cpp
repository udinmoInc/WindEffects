
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class MenusModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "MenusModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "MenusModule shutdown\n";
    }
};

IMPLEMENT_MODULE(MenusModule, WindEffects_Menus)
