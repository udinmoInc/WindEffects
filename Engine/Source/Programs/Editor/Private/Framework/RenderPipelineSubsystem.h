// ==============================================================================
// WindEffects — Editor — RenderPipelineSubsystem
// Swapchain ensure, uniform upload, UI overlay record, scene/present.
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

class KindUIServerSubsystem;
class PlatformInputSubsystem;

class RenderPipelineSubsystem final : public we::runtime::core::ISubsystem {
public:
    RenderPipelineSubsystem(
        IEditorLoopHost& host,
        we::runtime::core::ApplicationFramework& framework,
        EditorLoopComponentRegistry& components,
        KindUIServerSubsystem& kindUi,
        PlatformInputSubsystem& platformInput);

    void Initialize() override;
    void Tick(float deltaTime) override;
    void Shutdown() override;
    void ProcessCommand(const we::runtime::core::ApplicationCommand& command) override;

    const char* GetName() const override { return "RenderPipelineSubsystem"; }
    /// After KindUI layout.
    int GetPriority() const override { return 40; }

    bool BeganFrame() const { return m_BeganFrame; }

private:
    IEditorLoopHost& m_Host;
    we::runtime::core::ApplicationFramework& m_Framework;
    EditorLoopComponentRegistry& m_Components;
    KindUIServerSubsystem& m_KindUi;
    PlatformInputSubsystem& m_PlatformInput;

    bool m_FirstFrame = true;
    bool m_BeganFrame = false;
    uint32_t m_BeginFrameFailStreak = 0;
};

} // namespace we::programs::editor
