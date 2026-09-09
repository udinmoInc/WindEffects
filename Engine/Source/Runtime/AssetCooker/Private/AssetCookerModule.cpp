// ==============================================================================
// WindEffects — AssetCooker — AssetCookerModule
// Internal implementation for the AssetCooker module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

class AssetCookerModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetCookerModule started");
    }
    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetCookerModule shutdown");
    }
};

IMPLEMENT_MODULE(AssetCookerModule, WindEffects_AssetCooker)
