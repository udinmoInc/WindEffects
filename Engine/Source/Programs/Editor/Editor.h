#pragma once

#include <memory>
#include <string>

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

namespace we::runtime::kindui {
class OverlayHost;
class IconRenderer;
}

namespace we::programs::editor {
class Editor {
public:
    explicit Editor(we::platform::WindowId window);
    ~Editor();

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    void Run();

private:
    void MainLoop();
    void Shutdown();
    void CreateNewLevel();
    void MaybeShowFirstRunAgreement();

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    bool m_Running = true;
    bool m_FirstRunAgreementPending = false;

    std::unique_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::shared_ptr<we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<we::runtime::scene::Scene> m_Scene;

    std::shared_ptr<we::runtime::kindui::Widget> m_RootWidget;
    std::shared_ptr<we::runtime::kindui::OverlayHost> m_OverlayHost;
    std::shared_ptr<we::runtime::kindui::EventSystem> m_UIEventSystem;
    std::unique_ptr<we::runtime::kindui::OverlayRenderer> m_OverlayRenderer;

    std::shared_ptr<we::runtime::kindui::Widget> m_ViewportWidget;
    std::shared_ptr<we::runtime::kindui::StatusBar> m_StatusBar;
    std::shared_ptr<we::runtime::kindui::TitleBar> m_TitleBar;
    std::unique_ptr<we::runtime::kindui::EditorApplicationContext> m_UIContext;
    we::editor::mainframe::EditorWindowHitTestData m_WindowHitTestData{};

    void EnsureVisibleSwapchain();
    void SyncViewportFramebufferFromLayout();
    void UpdateUiScaleFromWindow();
    void LogWidgetTreeLayout(const std::shared_ptr<we::runtime::kindui::Widget>& widget, const std::string& name, int depth = 0);

    uint32_t m_LastLayoutSwapchainW = 0;
    uint32_t m_LastLayoutSwapchainH = 0;
};
} // namespace we::programs::editor
