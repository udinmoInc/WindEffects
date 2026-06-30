#pragma once

#include <volk.h>
#include <SDL3/SDL.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

#include "Renderer/VulkanContext.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderGraph.hpp"
#include "Renderer/SceneRenderer.hpp"
#include "EditorCamera.hpp"
#include "Scene/Scene.hpp"

// HouseUI Headers
#include "Core/Widget.hpp"
#include "Core/EventSystem.hpp"
#include "Rendering/UIRenderer.hpp"
#include "Widgets/Panel.hpp"
#include "Widgets/StatusBar.hpp"

namespace we::programs::editor {
class Editor {
public:
    Editor(SDL_Window* window);
    ~Editor();

    // Prevent copying
    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    void Run();

private:
    void MainLoop();
    void Shutdown();

    // Dynamically builds the UI from EditorRegistry
    void BuildDynamicEditorUI();

    SDL_Window* m_Window = nullptr;
    bool m_Running = true;

    // Core Engine Systems
    std::shared_ptr<we::runtime::renderer::VulkanContext> m_Context;
    std::shared_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::shared_ptr<we::runtime::renderer::RenderGraph> m_RenderGraph;
    std::shared_ptr<we::runtime::renderer::SceneRenderer> m_SceneRenderer;
    std::shared_ptr<we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<we::runtime::scene::Scene> m_Scene;

    // HouseUI System
    std::shared_ptr<UI::Widget> m_RootWidget;
    std::shared_ptr<UI::EventSystem> m_UIEventSystem;
    std::unique_ptr<we::UI::UIRenderer> m_UIRenderer;

    // Viewport widget (needs special handling for resize flush)
    std::shared_ptr<we::UI::Widget> m_ViewportWidget;
    std::shared_ptr<we::UI::StatusBar> m_StatusBar;

    void EnsureVisibleSwapchain();
    void LogWidgetTreeLayout(const std::shared_ptr<UI::Widget>& widget, const std::string& name, int depth = 0);
    void ValidateEditorPanels(const std::unordered_map<std::string, std::function<std::shared_ptr<UI::Panel>()>>& factories);

};

} // namespace we::programs::editor
