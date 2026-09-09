// ==============================================================================
// WindEffects — Prefab — PrefabModule
// Internal implementation for the Prefab module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class PrefabModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "PrefabModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE("Plugin", "PrefabModule shutdown");
    }
};

IMPLEMENT_MODULE(PrefabModule, WindEffects_Prefab)
