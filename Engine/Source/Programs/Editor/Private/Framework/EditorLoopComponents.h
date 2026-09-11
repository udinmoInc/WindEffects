// ==============================================================================
// WindEffects — Editor — EditorLoopComponents
// Delegated editor-only frame hooks (workspace, diagnostics, debug hotkeys).
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <memory>
#include <vector>
#include <functional>

#include "Platform/Events.h"

namespace we::programs::editor {

class IEditorLoopHost;

/**
 * Optional editor-only participation in the engine loop.
 * Panels / workspace / debug tools register here instead of hardcoding into MainLoop.
 */
class IEditorLoopComponent {
public:
    virtual ~IEditorLoopComponent() = default;

    virtual const char* GetName() const { return "EditorLoopComponent"; }

    virtual void OnBeginFrame(IEditorLoopHost& /*host*/) {}
    virtual void OnPlatformEvent(IEditorLoopHost& /*host*/, const we::platform::PlatformEvent& /*event*/) {}
    virtual void OnPostInput(IEditorLoopHost& /*host*/) {}
    virtual void OnTick(IEditorLoopHost& /*host*/, float /*deltaTime*/) {}
    virtual void OnPostPresent(IEditorLoopHost& /*host*/, bool /*beganFrame*/) {}
    virtual void OnEndFrame(IEditorLoopHost& /*host*/) {}
};

class EditorLoopComponentRegistry {
public:
    void Register(std::unique_ptr<IEditorLoopComponent> component);
    void Clear();

    void BeginFrame(IEditorLoopHost& host);
    void PlatformEvent(IEditorLoopHost& host, const we::platform::PlatformEvent& event);
    void PostInput(IEditorLoopHost& host);
    void Tick(IEditorLoopHost& host, float deltaTime);
    void PostPresent(IEditorLoopHost& host, bool beganFrame);
    void EndFrame(IEditorLoopHost& host);

private:
    std::vector<std::unique_ptr<IEditorLoopComponent>> m_Components;
};

/// Built-in editor components (workspace dock flush, first-run, diagnostics, F9 debug).
void RegisterDefaultEditorLoopComponents(EditorLoopComponentRegistry& registry, IEditorLoopHost& host);

/// Feed frame-result telemetry into the pulse diagnostics component (if registered).
void NotifyLoopPulseDiagnostics(bool minimized, bool beganFrame, bool focused, uint32_t failStreak, size_t eventCount);

} // namespace we::programs::editor
