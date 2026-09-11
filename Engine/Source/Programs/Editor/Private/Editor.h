// ==============================================================================
// WindEffects — Editor — Editor
// Internal implementation for the Editor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <filesystem>

#include "Platform/Types.h"
#include "Renderer/Renderer.h"
#include "EditorCamera.h"
#include "Scene/Scene.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Rendering/OverlayRenderer.h"
#include "Widgets/StatusBar.h"
#include "Widgets/TitleBar.h"
#include "EditorWindowHitTest.h"
#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"
#include "EditorShellBuilder.h"
#include "Undo/IUndoRuntime.h"
#include "ViewportEdit/IViewportEditor.h"
#include "WorldOutliner/IWorldOutliner.h"
#include "ContentBrowser/IContentBrowser.h"
#include "Terrain/ITerrainRuntime.h"
#include "PrefabEditor/PrefabEditor.h"
#include "Compilation/Compilation.h"
#include "Serialization/ISerializer.h"

#include "Projects/EditorCommandLine.h"
#include "Framework/IEditorLoopHost.h"

namespace we::programs::editor {
using ::we::editor::services::EditorApplicationContext;
class EditorApplicationFramework;
}

namespace we::programs::editor {
class Editor final : public IEditorLoopHost {
public:
    Editor(we::platform::WindowId window, const we::projects::EditorCommandLine& commandLine);
    ~Editor() override;

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    void Run();

    /// Launch WeLauncher.exe (Project Manager). Returns false if the executable was not found.
    static bool LaunchWeLauncher(const std::vector<std::string>& extraArgs = {});

    // IEditorLoopHost
    we::platform::WindowId GetHostWindow() const override { return m_Window; }
    bool IsHostRunning() const override { return m_Running; }
    void RequestHostStop() override { m_Running = false; }

    we::runtime::renderer::Renderer* GetHostRenderer() override { return m_Renderer.get(); }
    we::runtime::engine::EditorCamera* GetHostCamera() override { return m_Camera.get(); }
    we::runtime::scene::Scene* GetHostScene() override { return m_Scene.get(); }

    std::shared_ptr<we::runtime::kindui::Widget> GetHostRootWidget() override { return m_RootWidget; }
    std::shared_ptr<we::runtime::kindui::Widget> GetHostViewportWidget() override { return m_ViewportWidget; }
    std::shared_ptr<we::runtime::kindui::EventSystem> GetHostUIEventSystem() override { return m_UIEventSystem; }
    std::shared_ptr<we::runtime::kindui::OverlayHost> GetHostOverlayHost() override { return m_OverlayHost; }
    we::runtime::kindui::OverlayRenderer* GetHostOverlayRenderer() override { return m_OverlayRenderer.get(); }

    we::platform::Int2& HostLastSampledMousePos() override { return m_LastSampledMousePos; }
    uint64_t& HostLastSceneCameraHash() override { return m_LastSceneCameraHash; }
    bool& HostHasRenderedScene() override { return m_HasRenderedScene; }
    bool& HostForceSwapchainRecreate() override { return m_ForceSwapchainRecreate; }
    uint64_t& HostLatencyAuditFrameCounter() override { return m_LatencyAuditFrameCounter; }
    bool HostFirstRunAgreementPending() const override { return m_FirstRunAgreementPending; }

    void HostEnsureVisibleSwapchain() override { EnsureVisibleSwapchain(); }
    bool HostSyncViewportFramebufferFromLayout() override { return SyncViewportFramebufferFromLayout(); }
    void HostUpdateUiScaleFromWindow() override { UpdateUiScaleFromWindow(); }
    void HostTickSimulation(float dt) override { TickSimulation(dt); }
    void HostProcessLateInputMouse() override { ProcessLateInputMouse(); }
    void HostMaybeShowFirstRunAgreement() override { MaybeShowFirstRunAgreement(); }
    void HostReloadLayout() override;
    bool HostIsRemoteApiEnabled() const override { return m_CommandLine.enableRemoteApi; }

private:
    void InitializeEngine();
    void EnterProjectWorkspace(const std::filesystem::path& weprojPath);
    void UnloadProjectWorkspace();
    void CreateNewLevel();
    void OpenProjectDialog();
    void OpenWeLauncher();
    void MaybeShowFirstRunAgreement();
    void MainLoop();
    void Shutdown();

    void EnsureVisibleSwapchain();
    bool SyncViewportFramebufferFromLayout();
    void UpdateUiScaleFromWindow();
    void TickSimulation(float dt);
    void ProcessLateInputMouse();
    void LogWidgetTreeLayout(const std::shared_ptr<we::runtime::kindui::Widget>& widget, const std::string& name,
        int depth = 0);
    void SetRootWidget(const std::shared_ptr<we::runtime::kindui::Widget>& root);
    void UpdateWindowTitle();

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    we::projects::EditorCommandLine m_CommandLine{};
    bool m_Running = true;
    bool m_ShutdownComplete = false;
    bool m_FirstRunAgreementPending = false;
    std::string m_StatusMessage;

    std::unique_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::shared_ptr<we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<we::runtime::scene::Scene> m_Scene;

    std::shared_ptr<we::runtime::kindui::Widget> m_RootWidget;
    std::shared_ptr<we::runtime::kindui::OverlayHost> m_OverlayHost;
    std::shared_ptr<we::runtime::kindui::EventSystem> m_UIEventSystem;
    std::unique_ptr<we::runtime::kindui::OverlayRenderer> m_OverlayRenderer;

    std::shared_ptr<we::runtime::kindui::Widget> m_ViewportWidget;
    std::shared_ptr<::we::editor::shell::StatusBar> m_StatusBar;
    std::shared_ptr<::we::editor::shell::TitleBar> m_TitleBar;
    std::unique_ptr<::we::editor::services::EditorApplicationContext> m_UIContext;
    std::unique_ptr<::we::editor::undo::IUndoRuntime> m_UndoRuntime;
    std::shared_ptr<::we::editor::viewportedit::IViewportEditor> m_ViewportEdit;
    std::shared_ptr<::we::editor::outliner::IWorldOutlinerRuntime> m_WorldOutliner;
    std::shared_ptr<::we::editor::contentbrowser::IContentBrowserRuntime> m_ContentBrowser;
    std::shared_ptr<::we::runtime::terrain::ITerrainRuntime> m_TerrainRuntime;
    std::shared_ptr<::we::runtime::prefab::IPrefabRuntime> m_PrefabRuntime;
    std::shared_ptr<::we::editor::prefab::IPrefabEditor> m_PrefabEditor;
    std::shared_ptr<::we::runtime::compilation::ICompilationRuntime> m_CompilationRuntime;

    we::platform::Int2 m_LastSampledMousePos{-1, -1};
    uint64_t m_LatencyAuditFrameCounter = 0;
    uint64_t m_LastSceneCameraHash = 0;
    bool m_HasRenderedScene = false;
    std::unique_ptr<::we::runtime::serialization::ISerializer> m_Serializer;
    ::we::editor::mainframe::EditorWindowHitTestData m_WindowHitTestData{};

    uint32_t m_LastLayoutSwapchainW = 0;
    uint32_t m_LastLayoutSwapchainH = 0;
    bool m_ForceSwapchainRecreate = false;
    bool m_EnsureSwapchainInProgress = false;
    bool m_EnsureSwapchainPending = false;

    std::unique_ptr<EditorApplicationFramework> m_ApplicationFramework;
};
} // namespace we::programs::editor
