#include "Modules/IModuleInterface.hpp"
#include <iostream>

class PlaceActorsModule : public IModuleInterface
{
public:
    void StartupModule() override
    {
        std::cout << "PlaceActorsModule started\n";
    }

    void ShutdownModule() override
    {
        std::cout << "PlaceActorsModule shutdown\n";
    }
};

IMPLEMENT_MODULE(PlaceActorsModule, WindEffects_PlaceActors)
