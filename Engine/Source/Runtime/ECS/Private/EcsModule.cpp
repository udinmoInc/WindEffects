// ==============================================================================
// WindEffects — ECS — EcsModule
// Internal implementation for the ECS module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
