
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class WorldOutlinerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "WorldOutlinerModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "WorldOutlinerModule shutdown\n";
    }
};

IMPLEMENT_MODULE(WorldOutlinerModule, WindEffects_WorldOutliner)
