
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class SceneModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "SceneModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "SceneModule shutdown\n";
    }
};

IMPLEMENT_MODULE(SceneModule, WindEffects_Scene)
