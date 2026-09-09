// ==============================================================================
// WindEffects — Environment — EnvironmentModule
// Internal implementation for the Environment module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class EnvironmentModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        WE_LOG_TRACE("Plugin", "EnvironmentModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "EnvironmentModule shutdown");
    }
};

IMPLEMENT_MODULE(EnvironmentModule, WindEffects_Environment)
