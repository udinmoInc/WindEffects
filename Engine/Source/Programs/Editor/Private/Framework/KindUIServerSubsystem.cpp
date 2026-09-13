// ==============================================================================
// WindEffects — Editor — KindUIServerSubsystem
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Framework/KindUIServerSubsystem.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LoopExecutionTrace.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "Widgets/ViewportWidget.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"

namespace we::programs::editor {

KindUIServerSubsystem::KindUIServerSubsystem(IEditorLoopHost& host)
    : m_Host(host) {}

void KindUIServerSubsystem::Initialize() {
    HE_INFO("[KindUIServerSubsystem] Initialized");
}

void KindUIServerSubsystem::Tick(float /*deltaTime*/) {
    we::runtime::core::LoopExecutionTrace::Scoped scope("KindUIServer.Tick");
    m_LayoutOrResizeThisFrame = false;

    if (!m_Host.GetHostRootWidget()) {
        HE_ERROR("[KindUIServerSubsystem] Root widget is null; requesting host stop.");
        m_Host.RequestHostStop();
        return;
    }

    we::runtime::core::LoopExecutionTrace::GateState(
        "KindUIServer.preLayout",
        we::runtime::kindui::UIRepaintGate::PeekNeedsLayout(),
        we::runtime::kindui::UIRepaintGate::PeekNeedsPaint(),
        0);

    m_Host.HostUpdateUiScaleFromWindow();
    // Layout is owned here (sole ConsumeNeedsLayout consumer for the editor).
    // Skip Measure/Arrange when the gate is clean — widget Tick already ran.
    if (we::runtime::kindui::UIRepaintGate::PeekNeedsLayout()) {
        m_LayoutOrResizeThisFrame = m_Host.HostSyncViewportFramebufferFromLayout();
    }
    ::we::editor::services::EditorPerfStats::Get().Mark("layout");

    we::runtime::core::LoopExecutionTrace::GateState(
        "KindUIServer.postLayout",
        we::runtime::kindui::UIRepaintGate::PeekNeedsLayout(),
        we::runtime::kindui::UIRepaintGate::PeekNeedsPaint(),
        0);

    if (auto ui = m_Host.GetHostUIEventSystem()) {
        if (auto vp = std::dynamic_pointer_cast<::we::editor::viewport::ViewportWidget>(
                m_Host.GetHostViewportWidget())) {
            ui->SetSuppressSystemCursor(vp->IsFlyLookActive());
        }
    }
}

void KindUIServerSubsystem::Shutdown() {
    HE_INFO("[KindUIServerSubsystem] Shutdown");
}

} // namespace we::programs::editor
