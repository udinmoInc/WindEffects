// ==============================================================================
// WindEffects — Editor — EditorApplicationFramework
// Editor specialization of ApplicationFramework / IEngineLoop.
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

class PlatformInputSubsystem;
class KindUIServerSubsystem;
class RenderPipelineSubsystem;

class EditorApplicationFramework final : public we::runtime::core::ApplicationFramework {
public:
    explicit EditorApplicationFramework(IEditorLoopHost& host);
    ~EditorApplicationFramework() override;

    EditorLoopComponentRegistry& Components() { return m_Components; }

protected:
    void InitializeApplication() override;
    void TickApplication(float deltaTime) override;
    void ShutdownApplication() override;
    bool ShouldContinueRunning() const override;
    bool ProcessPlatformEvents() override;
    void BeginFrame() override;
    void EndFrame() override;

private:
    void RegisterCoreSubsystems();

    IEditorLoopHost& m_Host;
    EditorLoopComponentRegistry m_Components;

    PlatformInputSubsystem* m_PlatformInput = nullptr;
    KindUIServerSubsystem* m_KindUi = nullptr;
    RenderPipelineSubsystem* m_RenderPipeline = nullptr;
};

} // namespace we::programs::editor
