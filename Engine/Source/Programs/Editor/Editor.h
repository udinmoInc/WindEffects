#pragma once

#include <volk.h>
#include <SDL3/SDL.h>
#include <memory>

#include "Renderer/Renderer.h"
#include "EditorCamera.h"
#include "Scene/Scene.h"

#include "Core/Widget.h"
#include "Core/EventSystem.h"
#include "Rendering/OverlayRenderer.h"
#include "Widgets/StatusBar.h"
#include "Widgets/TitleBar.h"
#include "EditorWindowHitTest.h"
#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"
#include "EditorShellBuilder.h"

namespace WindEffects::Editor::UI {
class OverlayHost;
class RenderDebuggerPanel;
}

namespace we::programs::editor {
class Editor {
public:
    Editor(SDL_Window* window);
    ~Editor();

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    void Run();

private:
    void MainLoop();
    void Shutdown();
    void CreateNewLevel();
    void MaybeShowFirstRunAgreement();

    SDL_Window* m_Window = nullptr;
    bool m_Running = true;
    bool m_FirstRunAgreementPending = false;

    std::unique_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::shared_ptr<we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<we::runtime::scene::Scene> m_Scene;

    std::shared_ptr<WindEffects::Editor::UI::Widget> m_RootWidget;
    std::shared_ptr<WindEffects::Editor::UI::OverlayHost> m_OverlayHost;
    std::shared_ptr<WindEffects::Editor::UI::EventSystem> m_UIEventSystem;
    std::unique_ptr<WindEffects::Editor::UI::OverlayRenderer> m_OverlayRenderer;

    std::shared_ptr<WindEffects::Editor::UI::Widget> m_ViewportWidget;
    std::shared_ptr<WindEffects::Editor::UI::StatusBar> m_StatusBar;
    std::shared_ptr<WindEffects::Editor::UI::TitleBar> m_TitleBar;
    std::shared_ptr<WindEffects::Editor::UI::RenderDebuggerPanel> m_RenderDebuggerPanel;
    std::unique_ptr<WindEffects::Editor::UI::EditorApplicationContext> m_UIContext;
    we::editor::mainframe::EditorWindowHitTestData m_WindowHitTestData{};

    void EnsureVisibleSwapchain();
    void SyncViewportFramebufferFromLayout();
    void UpdateUiScaleFromWindow();
    void LogWidgetTreeLayout(const std::shared_ptr<WindEffects::Editor::UI::Widget>& widget, const std::string& name, int depth = 0);
};

} // namespace we::programs::editor
