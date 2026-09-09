// ==============================================================================
// WindEffects — AssetRuntime — AssetRuntimeModule
// Internal implementation for the AssetRuntime module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

class AssetRuntimeModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetRuntimeModule started");
    }
    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "AssetRuntimeModule shutdown");
    }
};

IMPLEMENT_MODULE(AssetRuntimeModule, WindEffects_AssetRuntime)
