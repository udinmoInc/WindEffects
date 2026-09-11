// ==============================================================================
// WindEffects — Editor — EditorApplicationFramework
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Framework/EditorApplicationFramework.h"
#include "Framework/PlatformInputSubsystem.h"
#include "Framework/KindUIServerSubsystem.h"
#include "Framework/RenderPipelineSubsystem.h"

#include "Core/DiagnosticMacros.h"
#include "Core/FrameCounter.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"

namespace we::programs::editor {

EditorApplicationFramework::EditorApplicationFramework(IEditorLoopHost& host)
    : m_Host(host) {}

EditorApplicationFramework::~EditorApplicationFramework() = default;

void EditorApplicationFramework::RegisterCoreSubsystems() {
    RegisterSubsystem(std::make_unique<we::runtime::core::WindowStateSubsystem>());
    RegisterSubsystem(std::make_unique<we::runtime::core::SwapchainSubsystem>());

    auto platformInput = std::make_unique<PlatformInputSubsystem>(m_Host, *this, m_Components);
    m_PlatformInput = platformInput.get();
    RegisterSubsystem(std::move(platformInput));

    auto kindUi = std::make_unique<KindUIServerSubsystem>(m_Host);
    m_KindUi = kindUi.get();
    RegisterSubsystem(std::move(kindUi));

    auto render = std::make_unique<RenderPipelineSubsystem>(
        m_Host, *this, m_Components, *m_KindUi, *m_PlatformInput);
    m_RenderPipeline = render.get();
    RegisterSubsystem(std::move(render));
}

void EditorApplicationFramework::InitializeApplication() {
    RegisterDefaultEditorLoopComponents(m_Components, m_Host);
    RegisterCoreSubsystems();
    we::runtime::kindui::UIRepaintGate::Request();
    HE_INFO("[EditorApplicationFramework] Application initialized");
}

void EditorApplicationFramework::TickApplication(float deltaTime) {
    if (!m_Host.IsHostRunning()) {
        RequestExit();
        return;
    }
    if (!m_Host.GetHostRootWidget()) {
        HE_ERROR("[Render] Root widget is null during frame tick; stopping main loop.");
        m_Host.RequestHostStop();
        RequestExit();
        return;
    }

    m_Host.HostProcessLateInputMouse();
    m_Host.HostTickSimulation(deltaTime);
    ::we::editor::services::EditorPerfStats::Get().Mark("tick");
    m_Components.Tick(m_Host, deltaTime);

    we::runtime::core::FrameCounter::Advance();
}

void EditorApplicationFramework::ShutdownApplication() {
    m_Components.Clear();
    m_PlatformInput = nullptr;
    m_KindUi = nullptr;
    m_RenderPipeline = nullptr;
    HE_INFO("[EditorApplicationFramework] Application shutdown");
}

bool EditorApplicationFramework::ShouldContinueRunning() const {
    return ApplicationFramework::ShouldContinueRunning() && m_Host.IsHostRunning();
}

bool EditorApplicationFramework::ProcessPlatformEvents() {
    if (!m_PlatformInput) {
        return m_Host.IsHostRunning();
    }
    return m_PlatformInput->ProcessFrameEvents();
}

void EditorApplicationFramework::BeginFrame() {
    we::runtime::kindui::UIRepaintGate::BeginFrame();
    m_Components.BeginFrame(m_Host);
}

void EditorApplicationFramework::EndFrame() {
    m_Components.EndFrame(m_Host);
}

} // namespace we::programs::editor
