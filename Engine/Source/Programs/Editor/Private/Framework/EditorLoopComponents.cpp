// ==============================================================================
// WindEffects — Editor — EditorLoopComponents
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Framework/EditorLoopComponents.h"
#include "Framework/IEditorLoopHost.h"

#include "Core/Logger.h"
#include "Core/DiagnosticMacros.h"
#include "KindUI/Profiling/ScreenRecorder.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "KindUI/Profiling/UiPathDiagnostics.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Rendering/OverlayRenderer.h"
#include "Renderer/Renderer.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "WindEffects/Editor/UI/Widgets/RenderInvestigationModal.h"

#include <chrono>
#include <sstream>

#ifndef WE_DEBUG_UI
#define WE_DEBUG_UI 0
#endif

namespace we::programs::editor {
namespace {

class WorkspaceDockComponent final : public IEditorLoopComponent {
public:
    const char* GetName() const override { return "WorkspaceDock"; }
    void OnPostInput(IEditorLoopHost& /*host*/) override {
        EditorWorkspaceController::Get().FlushPendingDockActions();
    }
};

class FirstRunAgreementComponent final : public IEditorLoopComponent {
public:
    const char* GetName() const override { return "FirstRunAgreement"; }
    void OnPostPresent(IEditorLoopHost& host, bool beganFrame) override {
        if (!m_Shown && beganFrame && host.HostFirstRunAgreementPending()) {
            host.HostMaybeShowFirstRunAgreement();
            m_Shown = true;
        }
    }
private:
    bool m_Shown = false;
};

class DebugHotkeyComponent final : public IEditorLoopComponent {
public:
    const char* GetName() const override { return "DebugHotkeys"; }
    void OnPlatformEvent(IEditorLoopHost& /*host*/, const we::platform::PlatformEvent& event) override {
#if WE_DEBUG_UI
        if (const auto* key = std::get_if<we::platform::KeyEvent>(&event)) {
            if (key->pressed && key->key == we::platform::KeyCode::F9) {
                ::we::editor::panels::RenderInvestigationModalHost::Toggle();
            }
        }
#else
        (void)event;
#endif
    }
};

class LatencyAuditComponent final : public IEditorLoopComponent {
public:
    const char* GetName() const override { return "LatencyAudit"; }
    void OnEndFrame(IEditorLoopHost& host) override {
        if (!we::runtime::kindui::UiInputLatencyAudit::IsEnabled()) {
            return;
        }
        auto& counter = host.HostLatencyAuditFrameCounter();
        ++counter;
        if (counter % 300 == 0) {
            we::runtime::kindui::UiInputLatencyAudit::Get().FlushPendingReport();
        }
    }
};

class LoopPulseDiagnosticsComponent final : public IEditorLoopComponent {
public:
    explicit LoopPulseDiagnosticsComponent(IEditorLoopHost& host) : m_Host(host) {}
    const char* GetName() const override { return "LoopPulseDiagnostics"; }

    void OnBeginFrame(IEditorLoopHost& /*host*/) override {
        we::runtime::kindui::UiPathDiagnostics::Get().BeginFrame();
        ::we::editor::services::EditorPerfStats::Get().BeginFrame();
    }

    void OnEndFrame(IEditorLoopHost& host) override {
        auto* overlay = host.GetHostOverlayRenderer();
        const auto& stats = overlay ? overlay->GetFrameStats() : we::runtime::kindui::UIFrameStats{};
        ::we::editor::services::EditorPerfStats::Get().EndFrame(
            stats.vertices,
            stats.batches,
            stats.opaqueBatches,
            stats.alphaBatches,
            stats.opaqueIndices,
            stats.alphaIndices);
        we::runtime::kindui::UiPathDiagnostics::Get().SetGeometryVertices(stats.vertices);
        we::runtime::kindui::UiPathDiagnostics::Get().EndFrame();

        {
            const auto& perf = ::we::editor::services::EditorPerfStats::Get().Last();
            we::runtime::kindui::ScreenRecorder::FrameMetrics metrics{};
            metrics.frameMs = perf.frameMs;
            metrics.tickMs = perf.tickMs;
            metrics.layoutMs = perf.layoutMs;
            metrics.uiMs = perf.uiBuildMs;
            metrics.sceneMs = perf.sceneMs;
            metrics.presentMs = perf.presentMs;
            metrics.fps = ::we::editor::services::EditorPerfStats::Get().AverageFps();
            metrics.uiVertices = perf.uiVertices;
            metrics.uiBatches = perf.uiBatches;
            metrics.uiIndices = perf.uiOpaqueIndices + perf.uiAlphaIndices;
            we::runtime::kindui::ScreenRecorder::Get().RecordFrame(metrics);
        }

        EmitPulse(host);
    }

    void NotifyFrameResult(bool minimized, bool beganFrame, bool focused, uint32_t failStreak, size_t eventCount) {
        m_Minimized = minimized;
        m_BeganFrame = beganFrame;
        m_Focused = focused;
        m_FailStreak = failStreak;
        m_EventCount = eventCount;
    }

private:
    void EmitPulse(IEditorLoopHost& host) {
        auto* renderer = host.GetHostRenderer();
        const auto& perf = ::we::editor::services::EditorPerfStats::Get().Last();
        const uint32_t sw = renderer ? renderer->GetSwapchainWidth() : 0;
        const uint32_t sh = renderer ? renderer->GetSwapchainHeight() : 0;
        const uint64_t paints = we::runtime::kindui::UIRepaintGate::PaintRebuildCount();
        const uint64_t skips = we::runtime::kindui::UIRepaintGate::IdleSkipCount();

        using clock = std::chrono::steady_clock;
        const double nowMs = std::chrono::duration<double, std::milli>(
            clock::now().time_since_epoch()).count();

        const bool changed =
            m_Minimized != m_PrevMin
            || m_BeganFrame != m_PrevBegan
            || m_Focused != m_PrevFocused
            || m_FailStreak != m_PrevFail;
        const bool due = (nowMs - m_LastPulseMs) >= 1000.0;
        if (!changed && !due) {
            return;
        }

        const char* renderState = m_Minimized
            ? "minimized"
            : (m_BeganFrame ? "presented" : (renderer ? "beginFrame-FAIL" : "no-renderer"));
        std::ostringstream line;
        line << "[Loop] state=" << renderState
             << " min=" << (m_Minimized ? 1 : 0)
             << " focus=" << (m_Focused ? 1 : 0)
             << " begin=" << (m_BeganFrame ? 1 : 0)
             << " failStreak=" << m_FailStreak
             << " events=" << m_EventCount
             << " swap=" << sw << "x" << sh
             << " fps=" << ::we::editor::services::EditorPerfStats::Get().AverageFps()
             << " frameMs=" << perf.frameMs
             << " tick=" << perf.tickMs
             << " layout=" << perf.layoutMs
             << " ui=" << perf.uiBuildMs
             << " scene=" << perf.sceneMs
             << " present=" << perf.presentMs
             << " verts=" << perf.uiVertices
             << " batches=" << perf.uiBatches
             << " paints+" << (paints - m_LastPaints)
             << " skips+" << (skips - m_LastSkips)
             << " log=" << we::runtime::core::Logger::GetActiveLogFilePath();
        HE_INFO(line.str());
        we::runtime::kindui::ScreenRecorder::Get().RecordEvent(line.str());
        we::runtime::core::Logger::Flush();
        m_PrevMin = m_Minimized;
        m_PrevBegan = m_BeganFrame;
        m_PrevFocused = m_Focused;
        m_PrevFail = m_FailStreak;
        m_LastPulseMs = nowMs;
        m_LastPaints = paints;
        m_LastSkips = skips;
    }

    IEditorLoopHost& m_Host;
    bool m_Minimized = false;
    bool m_BeganFrame = true;
    bool m_Focused = true;
    uint32_t m_FailStreak = 0;
    size_t m_EventCount = 0;

    bool m_PrevMin = false;
    bool m_PrevBegan = true;
    bool m_PrevFocused = true;
    uint32_t m_PrevFail = 0;
    double m_LastPulseMs = 0.0;
    uint64_t m_LastPaints = 0;
    uint64_t m_LastSkips = 0;
};

// Stored so RenderPipeline can report frame results into diagnostics.
LoopPulseDiagnosticsComponent* g_PulseDiagnostics = nullptr;

} // namespace

void EditorLoopComponentRegistry::Register(std::unique_ptr<IEditorLoopComponent> component) {
    if (component) {
        m_Components.push_back(std::move(component));
    }
}

void EditorLoopComponentRegistry::Clear() {
    m_Components.clear();
}

void EditorLoopComponentRegistry::BeginFrame(IEditorLoopHost& host) {
    for (auto& c : m_Components) {
        c->OnBeginFrame(host);
    }
}

void EditorLoopComponentRegistry::PlatformEvent(IEditorLoopHost& host, const we::platform::PlatformEvent& event) {
    for (auto& c : m_Components) {
        c->OnPlatformEvent(host, event);
    }
}

void EditorLoopComponentRegistry::PostInput(IEditorLoopHost& host) {
    for (auto& c : m_Components) {
        c->OnPostInput(host);
    }
}

void EditorLoopComponentRegistry::Tick(IEditorLoopHost& host, float deltaTime) {
    for (auto& c : m_Components) {
        c->OnTick(host, deltaTime);
    }
}

void EditorLoopComponentRegistry::PostPresent(IEditorLoopHost& host, bool beganFrame) {
    for (auto& c : m_Components) {
        c->OnPostPresent(host, beganFrame);
    }
}

void EditorLoopComponentRegistry::EndFrame(IEditorLoopHost& host) {
    for (auto& c : m_Components) {
        c->OnEndFrame(host);
    }
}

void RegisterDefaultEditorLoopComponents(EditorLoopComponentRegistry& registry, IEditorLoopHost& host) {
    registry.Register(std::make_unique<WorkspaceDockComponent>());
    registry.Register(std::make_unique<DebugHotkeyComponent>());
    registry.Register(std::make_unique<FirstRunAgreementComponent>());
    registry.Register(std::make_unique<LatencyAuditComponent>());
    auto pulse = std::make_unique<LoopPulseDiagnosticsComponent>(host);
    g_PulseDiagnostics = pulse.get();
    registry.Register(std::move(pulse));
    (void)host;
}

void NotifyLoopPulseDiagnostics(bool minimized, bool beganFrame, bool focused, uint32_t failStreak, size_t eventCount) {
    if (g_PulseDiagnostics) {
        g_PulseDiagnostics->NotifyFrameResult(minimized, beganFrame, focused, failStreak, eventCount);
    }
}

} // namespace we::programs::editor
