// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsModule
// Internal implementation for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "PlaceActors/PlaceActorsRegistration.h"
#include "Core/Logger.h"

class PlaceActorsModule : public we::core::IModuleInterface
{
public:
    void StartupModule() override
    {
        we::programs::editor::EnsurePlaceActorsRegistered();
        WE_LOG_TRACE("Plugin", "PlaceActorsModule started");
    }

    void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "PlaceActorsModule shutdown");
    }
};

IMPLEMENT_MODULE(PlaceActorsModule, WindEffects_PlaceActors)
