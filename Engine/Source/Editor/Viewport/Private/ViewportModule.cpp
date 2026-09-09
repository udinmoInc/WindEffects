// ==============================================================================
// WindEffects — Viewport — ViewportModule
// Internal implementation for the Viewport module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class ViewportModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "ViewportModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "ViewportModule shutdown");
    }
};

IMPLEMENT_MODULE(ViewportModule, WindEffects_Viewport)
