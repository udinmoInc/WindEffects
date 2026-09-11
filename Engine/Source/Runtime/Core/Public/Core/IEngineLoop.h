// ==============================================================================
// WindEffects — Core — IEngineLoop
// Generalized application loop contract for Editor and standalone runtimes.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"

namespace we::runtime::core {

/**
 * Portable engine loop surface.
 * ApplicationFramework implements this; Editor/Runtime specialize behavior via
 * overrides and registered ISubsystem instances.
 */
class IEngineLoop {
public:
    virtual ~IEngineLoop() = default;

    /// One-time setup (application + subsystems).
    virtual void Initialize() = 0;

    /// Pump OS / platform messages. Returns false when the app should exit.
    virtual bool PollEvents() = 0;

    /// Advance one frame. Returns false when the loop should stop.
    virtual bool Tick(float deltaTime) = 0;

    /// Tear down subsystems and application state.
    virtual void Shutdown() = 0;

    /// Request a graceful exit (processed at a safe point).
    virtual void RequestExit() = 0;

    /// Convenience: Initialize → poll/tick until exit → Shutdown.
    virtual void Run() = 0;

    virtual bool IsRunning() const = 0;
};

} // namespace we::runtime::core
