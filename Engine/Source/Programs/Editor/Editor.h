#pragma once

#include <volk.h>
#include <SDL3/SDL.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

#include "Renderer/Renderer.h"
#include "EditorCamera.h"
#include "Scene/Scene.h"

#include "Core/Widget.h"
#include "Core/EventSystem.h"
#include "Rendering/OverlayRenderer.h"
namespace we::editor::rendering {
    class EditorCompositor;
}
#include "Widgets/Panel.h"
#include "Widgets/StatusBar.h"
#include "Widgets/TitleBar.h"
#include "EditorWindowHitTest.h"
#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"
#include "WindEffects/Editor/UI/Shell/EditorShellBuilder.h"

namespace we::UI {
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

    void BuildDynamicEditorUI();

    SDL_Window* m_Window = nullptr;
    bool m_Running = true;
    bool m_FirstRunAgreementPending = false;

    std::unique_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::shared_ptr<we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<we::runtime::scene::Scene> m_Scene;

    std::shared_ptr<UI::Widget> m_RootWidget;
    std::shared_ptr<UI::EventSystem> m_UIEventSystem;
    std::unique_ptr<we::UI::OverlayRenderer> m_OverlayRenderer;

    std::shared_ptr<we::UI::Widget> m_ViewportWidget;
    std::shared_ptr<we::UI::StatusBar> m_StatusBar;
    std::shared_ptr<we::UI::TitleBar> m_TitleBar;
    std::shared_ptr<we::UI::RenderDebuggerPanel> m_RenderDebuggerPanel;
    std::unique_ptr<WindEffects::Editor::UI::EditorApplicationContext> m_UIContext;
    we::editor::mainframe::EditorWindowHitTestData m_WindowHitTestData{};

    void EnsureVisibleSwapchain();
    void SyncViewportFramebufferFromLayout();
    void UpdateUiScaleFromWindow();
    void LogWidgetTreeLayout(const std::shared_ptr<UI::Widget>& widget, const std::string& name, int depth = 0);
    void ValidateEditorPanels(const std::unordered_map<std::string, std::function<std::shared_ptr<UI::Panel>()>>& factories);
};

} // namespace we::programs::editor
