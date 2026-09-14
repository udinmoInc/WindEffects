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
#include <KindUI/EditorUI.h>
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>

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

    // Timed exit for automated FPS / perf measurement (seconds).
    if (const char* autoExit = std::getenv("WE_AUTO_EXIT_SECONDS")) {
        const double limitSec = std::atof(autoExit);
        if (limitSec > 0.0) {
            using clock = std::chrono::steady_clock;
            static const auto s_Start = clock::now();
            const double elapsed = std::chrono::duration<double>(clock::now() - s_Start).count();
            if (elapsed >= limitSec) {
                HE_INFO(std::string("[Startup] WE_AUTO_EXIT_SECONDS=") + autoExit + " reached; exiting.");
                m_Host.RequestHostStop();
                RequestExit();
                return;
            }
        }
    }

    // Memory stress: ExpandAll ↔ CollapseAll under the root (WE_KINDUI_MEM_STRESS=expand).
    if (const char* stress = std::getenv("WE_KINDUI_MEM_STRESS")) {
        if (stress[0] != '\0' && std::strcmp(stress, "0") != 0) {
            using clock = std::chrono::steady_clock;
            static auto s_Last = clock::now();
            static int s_Cycle = 0;
            static bool s_Expand = true;
            const int maxCycles = []() {
                if (const char* n = std::getenv("WE_KINDUI_MEM_STRESS_CYCLES")) {
                    const int v = std::atoi(n);
                    return v > 0 ? v : 20;
                }
                return 20;
            }();
            const double intervalSec = []() {
                if (const char* n = std::getenv("WE_KINDUI_MEM_STRESS_INTERVAL")) {
                    const double v = std::atof(n);
                    return v > 0.05 ? v : 0.5;
                }
                return 0.5;
            }();
            const double since = std::chrono::duration<double>(clock::now() - s_Last).count();
            if (s_Cycle < maxCycles && since >= intervalSec) {
                s_Last = clock::now();
                if (auto root = m_Host.GetHostRootWidget()) {
                    if (std::strcmp(stress, "expand") == 0) {
                        if (s_Expand) {
                            we::runtime::kindui::Expansion::ExpandAllUnder(*root);
                        } else {
                            we::runtime::kindui::Expansion::CollapseAllUnder(*root);
                        }
                        s_Expand = !s_Expand;
                        ++s_Cycle;
                        HE_INFO(std::string("[EditorMemStress] expandCycle=") + std::to_string(s_Cycle)
                            + "/" + std::to_string(maxCycles));
                    } else if (std::strcmp(stress, "dock") == 0) {
                        auto& ws = ::we::programs::editor::EditorWorkspaceController::Get();
                        if (s_Expand) {
                            ws.FloatPanel("Details");
                        } else {
                            ws.DockPanel("Details");
                        }
                        s_Expand = !s_Expand;
                        ++s_Cycle;
                        HE_INFO(std::string("[EditorMemStress] dockCycle=") + std::to_string(s_Cycle)
                            + "/" + std::to_string(maxCycles));
                    } else if (std::strcmp(stress, "panel") == 0) {
                        auto& ws = ::we::programs::editor::EditorWorkspaceController::Get();
                        const bool visible = (s_Cycle % 2) == 0;
                        ws.SetPanelVisible("ContentBrowser", visible);
                        ws.SetPanelVisible("Details", visible);
                        ++s_Cycle;
                        HE_INFO(std::string("[EditorMemStress] panelCycle=") + std::to_string(s_Cycle)
                            + "/" + std::to_string(maxCycles)
                            + " visible=" + (visible ? "1" : "0"));
                    }
                    if (std::strcmp(stress, "expand") == 0
                        || std::strcmp(stress, "dock") == 0
                        || std::strcmp(stress, "panel") == 0) {
                        ::we::editor::services::EditorPerfStats::Get().CaptureMemory(
                            root.get(), m_Host.GetHostOverlayRenderer());
                    }
                }
            }
        }
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
