// ==============================================================================
// WindEffects — WorldOutliner — WorldOutlinerModule
// Internal implementation for the WorldOutliner module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class WorldOutlinerModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "WorldOutlinerModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "WorldOutlinerModule shutdown");
    }
};

IMPLEMENT_MODULE(WorldOutlinerModule, WindEffects_WorldOutliner)
