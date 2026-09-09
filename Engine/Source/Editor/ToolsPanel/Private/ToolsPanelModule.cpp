// ==============================================================================
// WindEffects — ToolsPanel — ToolsPanelModule
// Internal implementation for the ToolsPanel module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class ToolsPanelModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        HE_INFO("ToolsPanelModule: Editor tools panel module started.");
    }

    void ShutdownModule() override {
        HE_INFO("ToolsPanelModule: Editor tools panel module shutdown.");
    }
};

IMPLEMENT_MODULE(ToolsPanelModule, WindEffects_ToolsPanel)
