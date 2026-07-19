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
#include "Serialization/ISerializer.h"

#include "Projects/EditorCommandLine.h"

namespace we::programs::editor {
using ::we::editor::services::EditorApplicationContext;
}

namespace we::programs::editor {
class Editor {
public:
    Editor(we::platform::WindowId window, const we::projects::EditorCommandLine& commandLine);
    ~Editor();

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    void Run();

    /// Launch WeLauncher.exe (Project Manager). Returns false if the executable was not found.
    static bool LaunchWeLauncher(const std::vector<std::string>& extraArgs = {});

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
    void SyncViewportFramebufferFromLayout();
    void UpdateUiScaleFromWindow();
    void LogWidgetTreeLayout(const std::shared_ptr<we::runtime::kindui::Widget>& widget, const std::string& name, int depth = 0);
    void SetRootWidget(const std::shared_ptr<we::runtime::kindui::Widget>& root);
    void UpdateWindowTitle();

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    we::projects::EditorCommandLine m_CommandLine{};
    bool m_Running = true;
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
    std::unique_ptr<::we::runtime::serialization::ISerializer> m_Serializer;
    ::we::editor::mainframe::EditorWindowHitTestData m_WindowHitTestData{};

    uint32_t m_LastLayoutSwapchainW = 0;
    uint32_t m_LastLayoutSwapchainH = 0;
};
} // namespace we::programs::editor
