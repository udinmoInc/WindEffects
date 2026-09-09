// ==============================================================================
// WindEffects — RHI — RHIModule
// Internal implementation for the RHI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "RHI/RHI.h"

#include "Core/Logger.h"

class RHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE("Plugin", "RHIModule started");
    }

    void ShutdownModule() override {
        we::rhi::RHI::Shutdown();
        WE_LOG_TRACE("Plugin", "RHIModule shutdown");
    }
};

IMPLEMENT_MODULE(RHIModule, WindEffects_RHI)
