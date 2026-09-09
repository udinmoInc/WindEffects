// ==============================================================================
// WindEffects — Editor — Editor
// Internal implementation for the Editor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Editor.h"
#include "FirstRunAgreementPopup.h"
#include "KindUI/Benchmark/KindUIBenchmark.h"
#include "KindUI/Benchmark/KindUIInteractionBenchmark.h"
#include "KindUI/Profiling/UiColorCompositionDiagnostic.h"
#include "KindUI/Profiling/UiColorPipelineDiagnostic.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "EditorCompositionProbes.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "Core/AssetRegistry.h"
#include "Core/Logger.h"
#include "Core/PluginManager.h"
#include "Core/StartupValidator.h"
#include "ContentBrowser/ContentBrowserApi.h"
#include "EditorGridRenderer.h"
#include "Environment/EnvironmentSystem.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Layout/OverlayManager.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "Platform/PlatformSDK.h"
#include "ViewportToolbarState.h"
#include "Projects/EngineContext.h"
#include "Projects/ProjectContext.h"
#include "Projects/RecentProjectsStore.h"
#include "Terrain/Terrain.h"
#include "KindUI/Profiling/ScreenRecorder.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "KindUI/Widgets/ScreenDebugOverlay.h"

#include <cstdlib>

namespace we::programs::editor {

namespace UI = we::runtime::kindui;
using namespace we::runtime::renderer;
using namespace we::runtime::scene;
using namespace we::runtime::engine;

Editor::Editor(we::platform::WindowId window, const we::projects::EditorCommandLine& commandLine)
    : m_Window(window)
    , m_CommandLine(commandLine) {
    HE_INFO("[Startup] === Editor construction begin ===");
    InitializeEngine();

    we::projects::RecentProjectsStore::Get().Load();

    if (!m_CommandLine.projectPath) {
        HE_WARN("[Startup] No project path provided. Generating a temporary workspace...");
        auto tempDir = we::projects::EngineContext::Get().EngineRoot() / "Intermediate" / "TempProject";
        std::filesystem::create_directories(tempDir);
        m_CommandLine.projectPath = tempDir / "TempProject.weproj";
        
        // Use standard C file IO to avoid needing <fstream> include at the top
        FILE* f = fopen(m_CommandLine.projectPath->string().c_str(), "w");
        if (f) {
            std::string content = "{ \"projectVersion\": 1, \"projectName\": \"TempProject\", \"engineVersion\": \"" +
                we::projects::EngineContext::Get().EngineVersion() + "\" }";
            fwrite(content.data(), 1, content.size(), f);
            fclose(f);
        }
    }
    EnterProjectWorkspace(*m_CommandLine.projectPath);

    HE_INFO("[Startup] Swapchain: " + std::to_string(m_Renderer->GetSwapchainWidth())
        + "x" + std::to_string(m_Renderer->GetSwapchainHeight()));
    HE_INFO("[Startup] === WindEffects Engine Editor successfully bootstrapped ===");

    if (const char* bench = std::getenv("WE_UI_BENCH"); bench != nullptr && bench[0] != '\0' && bench[0] != '0') {
        const auto report = UI::RunKindUIBenchmark(500);
        HE_INFO("[UIBench] " + report.summary);
    }
    if (const char* interactBench = std::getenv("WE_UI_INTERACT_BENCH");
        interactBench != nullptr && interactBench[0] != '\0' && interactBench[0] != '0') {
        const auto report = UI::RunKindUIInteractionBenchmark(32);
        HE_INFO("[UIInteractBench] " + report.summary);
        for (const auto& scenario : report.scenarios) {
            HE_INFO("[UIInteractBench] " + scenario.name
                + " peak=" + std::to_string(static_cast<uint64_t>(scenario.peakMs * 1000.0)) + "us"
                + " avg=" + std::to_string(static_cast<uint64_t>(scenario.avgMs * 1000.0)) + "us"
                + " layout=" + std::to_string(scenario.layoutPasses)
                + " paint=" + std::to_string(scenario.paintPasses)
                + " inv=" + std::to_string(scenario.invalidateCount)
                + " layoutInv=" + std::to_string(scenario.layoutInvalidations)
                + " paintInv=" + std::to_string(scenario.paintInvalidations)
                + " widgets=" + std::to_string(scenario.widgetsVisited)
                + " cmds=" + std::to_string(scenario.paintCommands)
                + " cause=" + scenario.rootCause);
        }
    }
    if (const char* latencyBench = std::getenv("WE_UI_LATENCY_BENCH");
        latencyBench != nullptr && latencyBench[0] != '\0' && latencyBench[0] != '0') {
        const auto report = UI::RunUiInputLatencyBenchmark(32);
        HE_INFO("[UILatencyBench] " + report.summary);
        for (const auto& scenario : report.scenarios) {
            HE_INFO("[UILatencyBench] " + scenario.name
                + " in→handler=" + std::to_string(static_cast<uint64_t>(scenario.inputToHandlerMs * 1000.0)) + "us"
                + " handler→paint=" + std::to_string(static_cast<uint64_t>(scenario.handlerToPaintMs * 1000.0)) + "us"
                + " paint→present=" + std::to_string(static_cast<uint64_t>(scenario.paintToPresentMs * 1000.0)) + "us"
                + " total=" + std::to_string(static_cast<uint64_t>(scenario.totalVisibleMs * 1000.0)) + "us"
                + " cause=" + scenario.rootCause);
        }
    }
    if (const char* hitBench = std::getenv("WE_UI_HITTEST_BENCH");
        hitBench != nullptr && hitBench[0] != '\0' && hitBench[0] != '0') {
        const auto report = UI::RunHitTestAudit();
        HE_INFO("[UIHitTestBench] " + report.summary);
        for (const auto& testCase : report.cases) {
            HE_INFO("[UIHitTestBench] " + testCase.name
                + (testCase.passed ? " PASS " : " FAIL ")
                + testCase.detail);
        }
    }
}

void Editor::InitializeEngine() {
    HE_INFO("[Startup] Stage 1/6: Renderer...");
    m_Renderer = std::make_unique<Renderer>();
    m_Renderer->Init(m_Window);

    HE_INFO("[Startup] Stage 2/6: Scene and camera (engine-owned, empty until project loads)...");
    m_Camera = std::make_shared<EditorCamera>();
    BindViewportCamera(m_Camera);
    m_Scene = std::make_shared<Scene>();
    we::runtime::world::environment::EnvironmentSystem::Get().BindScene(m_Scene);
    PlaceActorsPlacement::Get().BindScene(m_Scene, m_Camera);

    {
        auto& startup = we::runtime::core::StartupValidator::Get();
        startup.RegisterCheck("Renderer", [this](std::string& detail) {
            detail = m_Renderer->IsGpuReady() ? "Foundation renderer initialized" : "Renderer GPU not ready";
            return m_Renderer->IsGpuReady();
        });
        startup.RunAll();
        if (!startup.AllPassed()) {
            throw std::runtime_error("Startup validation failed!");
        }
    }

    HE_INFO("[Startup] Stage 3/6: Engine default assets (fonts, shaders, icons, theme)...");
    if (!we::core::AssetRegistry::Get().LoadDefaultEditorAssets()) {
        HE_ERROR("[Startup] Required default assets missing - UI rendering may fail.");
    }

    HE_INFO("[Startup] Stage 4/6: OverlayRenderer init...");
    m_OverlayRenderer = std::make_unique<we::runtime::kindui::OverlayRenderer>();
    const uint32_t uiFramesInFlight = m_Renderer->GetRHIDevice()
        ? m_Renderer->GetRHIDevice()->GetFramesInFlight()
        : 2u;
    if (!m_OverlayRenderer->Init(
            m_Renderer->GetRHIDevice(),
            m_Renderer->GetSwapchainFormat(),
            uiFramesInFlight)) {
        throw std::runtime_error("Failed to initialize OverlayRenderer!");
    }

    m_UIEventSystem = std::make_shared<UI::EventSystem>();
    ::we::programs::editor::EditorWorkspaceController::Get().SetEventSystem(m_UIEventSystem.get());

    if (const char* vsyncEnv = std::getenv("WE_VSYNC"); vsyncEnv != nullptr) {
        we::runtime::kindui::UiInputLatencyAudit::SetVsyncEnabled(vsyncEnv[0] != '\0' && vsyncEnv[0] != '0');
    } else {
        we::runtime::kindui::UiInputLatencyAudit::SetVsyncEnabled(true);
    }
    if (m_Renderer->GetRHIDevice()) {
        const auto& rhiStats = m_Renderer->GetRHIDevice()->GetDiagnostics().lastFrame;
        if (rhiStats.refreshRateHz > 0) {
            we::runtime::kindui::UiInputLatencyAudit::SetRefreshRateHz(rhiStats.refreshRateHz);
        }
    }

    UpdateUiScaleFromWindow();
    m_UIContext = std::make_unique<::we::editor::services::EditorApplicationContext>();
    m_UIContext->Initialize(we::runtime::kindui::DPIContext::GetScale());
    UpdateUiScaleFromWindow();

    if (m_Renderer) {
        const bool isolatedPipelineTest =
            we::runtime::kindui::UiColorPipelineDiagnostic::IsEnabled()
            && !we::runtime::kindui::UiColorCompositionDiagnostic::IsEnabled();
        const we::runtime::kindui::Color clearAuthoring =
            isolatedPipelineTest
                ? we::runtime::kindui::Color::Black()
                : we::runtime::kindui::ResolveColor(we::runtime::kindui::ColorToken::WorkspaceBackground);
        const auto clearColor = we::runtime::kindui::ColorSpace::ClearColorForTarget(
            m_Renderer->GetSwapchainFormat(),
            clearAuthoring);
        m_Renderer->SetSwapchainClearColor({clearColor.r, clearColor.g, clearColor.b, clearColor.a});
        if (isolatedPipelineTest) {
            HE_INFO("[UiColorPipeline] WE_UI_COLOR_PIPELINE_TEST=1 — isolated UI swatch grid (no editor chrome, no 3D).");
        }
        if (we::runtime::kindui::UiColorCompositionDiagnostic::IsEnabled()) {
            we::runtime::kindui::UiColorCompositionDiagnostic::Get().Reset();
            we::runtime::kindui::UiColorCompositionDiagnostic::SetProbeRegistrar(
                [](const std::shared_ptr<we::runtime::kindui::Widget>& root) {
                    we::programs::editor::RegisterEditorCompositionProbes(root);
                });
            HE_INFO("[UiColorComposition] WE_UI_COLOR_COMPOSITION_TEST=1 — real editor composition audit (full shell, diagnostic hooks only).");
        }
    }

    m_FirstRunAgreementPending = !HasAcceptedFirstRunAgreement();
    HE_INFO("[Startup] Engine context ready (project not loaded yet).");
}

void Editor::SetRootWidget(const std::shared_ptr<we::runtime::kindui::Widget>& root) {
    m_RootWidget = root;
    if (m_RootWidget && ::we::runtime::kindui::ScreenRecorder::IsRecordingEnabled()) {
        m_RootWidget->AttachOverlayChild(std::make_shared<::we::runtime::kindui::ScreenDebugOverlay>());
    }
    if (m_UIEventSystem) {
        m_UIEventSystem->SetRootWidget(m_RootWidget);
    }
    m_LastLayoutSwapchainW = 0;
    m_LastLayoutSwapchainH = 0;
    EnsureVisibleSwapchain();
    SyncViewportFramebufferFromLayout();
    we::runtime::kindui::UIRepaintGate::Request();
}

void Editor::UpdateWindowTitle() {
    auto& platform = we::platform::Platform::Get();
    if (we::projects::ProjectContext::Get().IsLoaded()) {
        const auto& desc = we::projects::ProjectContext::Get().Descriptor();
        const std::string name = desc.displayName.empty() ? desc.projectName : desc.displayName;
        platform.SetWindowTitle(m_Window, name + " - WindEffects Editor");
    } else {
        platform.SetWindowTitle(m_Window, "WindEffects Editor");
    }
}

void Editor::Run() {
    MainLoop();
}

void Editor::MaybeShowFirstRunAgreement() {
    if (HasAcceptedFirstRunAgreement()) {
        m_FirstRunAgreementPending = false;
        return;
    }

    if (!m_OverlayHost) {
        HE_WARN("[Startup] First-run agreement skipped: overlay host unavailable.");
        m_FirstRunAgreementPending = false;
        return;
    }

    // During the RHI UI cutover the agreement dialog can be invisible (missing fonts /
    // incomplete GPU UI). Auto-accept so the editor remains usable.
    HE_WARN("[Startup] First-run agreement auto-accepted (UI path still finishing RHI cutover).");
    SetAcceptedFirstRunAgreement(true);
    m_FirstRunAgreementPending = false;
}

Editor::~Editor() {
    Shutdown();
}

void Editor::Shutdown() {
    if (m_Window != we::platform::WindowId::Invalid) {
        we::platform::Platform::Get().SetWindowHitTest(m_Window, nullptr, nullptr);
        we::platform::Platform::Get().SetRelativeMouseMode(m_Window, false);
    }
    m_WindowHitTestData.titleBar.reset();

    if (m_OverlayHost) {
        m_OverlayHost->CloseAllPopups();
    }

    ::we::editor::shell::EditorModeController::Get().ClearModeChangedListeners();

    EditorWorkspaceController::Get().SaveLayout();
    EditorWorkspaceController::Get().Reset();

    we::core::PluginManager::Get().UnloadAllPlugins();

    m_ViewportWidget.reset();
    m_TitleBar.reset();
    m_StatusBar.reset();
    m_OverlayHost.reset();
    m_RootWidget.reset();
    EditorWorkspaceController::Get().ClearLayoutRefs();
    ShutdownContentBrowserService();

    // Tear down terrain GPU resources before destroying the RHI device.
    if (m_Renderer) {
        m_Renderer->ClearTerrainDrawer();
    }
    if (m_TerrainRuntime) {
        m_TerrainRuntime->Shutdown();
        m_TerrainRuntime.reset();
        we::runtime::terrain::SetDefaultTerrainRuntime(nullptr);
    }

    if (we::projects::ProjectContext::Get().IsLoaded()) {
        we::projects::ProjectContext::Get().Unload();
    }
    if (m_OverlayRenderer) {
        m_OverlayRenderer->Shutdown();
        m_OverlayRenderer.reset();
    }
    we::runtime::kindui::UiInputLatencyAudit::Get().FlushPendingReport();

    m_UIEventSystem.reset();

    m_Scene.reset();
    m_Camera.reset();
    ::we::editor::grid::EditorGridRenderer::Get().Shutdown();
    if (m_Renderer) {
        m_Renderer->Shutdown();
        m_Renderer.reset();
    }
}

} // namespace we::programs::editor