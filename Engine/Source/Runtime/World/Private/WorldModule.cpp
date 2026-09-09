// ==============================================================================
// WindEffects — World — WorldModule
// Internal implementation for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include "World/WorldReflection.h"

#include "Reflection/Reflection.h"

namespace {

class WorldModule final : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        we::runtime::world::RegisterWorldReflectionTypes(
            we::runtime::reflection::GetTypeRegistry());
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "WorldModule started (World Runtime)");
    }

    void ShutdownModule() override {
        WE_LOG_TRACE(we::LogCategory::Plugin.data(), "WorldModule shutdown");
    }
};

} // namespace

IMPLEMENT_MODULE(WorldModule, WindEffects_World)
