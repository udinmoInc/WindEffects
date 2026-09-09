// ==============================================================================
// WindEffects — PrefabEditor — PrefabEditorModule
// Internal implementation for the PrefabEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class PrefabEditorModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "PrefabEditorModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE("Plugin", "PrefabEditorModule shutdown");
    }
};

IMPLEMENT_MODULE(PrefabEditorModule, WindEffects_PrefabEditor)
