// ==============================================================================
// WindEffects — Compilation — CompilationModule
// Internal implementation for the Compilation module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class CompilationModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "CompilationModule started");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE("Plugin", "CompilationModule shutdown");
    }
};

IMPLEMENT_MODULE(CompilationModule, WindEffects_Compilation)
