// ==============================================================================
// WindEffects — Core — IModuleInterface
// Public API surface for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

namespace we::core {

class IModuleInterface {
public:
    virtual ~IModuleInterface() = default;

    virtual void StartupModule() = 0;

    // Defaulted so modules without shutdown work (RHI backends, etc.)
    // don't carry an empty override just to satisfy the base.
    virtual void ShutdownModule() {}
};

#define IMPLEMENT_MODULE(ModuleClass, ModuleName) \
    extern "C" __declspec(dllexport) we::core::IModuleInterface* InitializeModule() \
    { \
        return new ModuleClass(); \
    }

} // namespace we::core
