#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"
#include "ECS/ComponentOps.h"
#include "ECS/ComponentType.h"
#include "ECS/Components/CoreComponents.h"

#include <cstring>

class EcsModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        using namespace we::runtime::ecs;
        ComponentTypeRegistry::Get().EnsureCoreTypesRegistered();
        WE_LOG_TRACE("Plugin", "EcsModule started (archetype ECS)");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "EcsModule shutdown");
    }
};

IMPLEMENT_MODULE(EcsModule, WindEffects_ECS)
