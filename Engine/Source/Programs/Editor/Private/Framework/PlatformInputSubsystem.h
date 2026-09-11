// ==============================================================================
// WindEffects — Editor — PlatformInputSubsystem
// OS message routing into KindUI + deferred window-state commands.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/ApplicationFramework.h"
#include "Framework/EditorLoopComponents.h"
#include "Framework/IEditorLoopHost.h"

namespace we::programs::editor {

class PlatformInputSubsystem final : public we::runtime::core::ISubsystem {
public:
    PlatformInputSubsystem(
        IEditorLoopHost& host,
        we::runtime::core::ApplicationFramework& framework,
        EditorLoopComponentRegistry& components);

    void Initialize() override;
    void Tick(float deltaTime) override;
    void Shutdown() override;

    const char* GetName() const override { return "PlatformInputSubsystem"; }
    int GetPriority() const override { return 90; }

    /// Poll platform + route frame events. Returns false on quit.
    bool ProcessFrameEvents();

    size_t LastEventCount() const { return m_LastEventCount; }

private:
    void HandleWindowLifecycle(const we::platform::PlatformEvent& event, bool& requestUiLayout, bool& requestUiPaint);
    void HandlePointerKeyboard(const we::platform::PlatformEvent& event, bool& requestUiPaint);

    /// Drop capture/hover/focus + transient popups so modal clicks cannot resume after focus/minimize.
    void ClearUiInputCapture();
    /// Mark force-recreate + enqueue one coalesced SwapchainRecreateRequest (no sync Ensure).
    void RequestDeferredSwapchainRefresh();

    IEditorLoopHost& m_Host;
    we::runtime::core::ApplicationFramework& m_Framework;
    EditorLoopComponentRegistry& m_Components;
    size_t m_LastEventCount = 0;
};

} // namespace we::programs::editor
