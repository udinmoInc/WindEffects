#pragma once

#include <volk.h>
#include <SDL3/SDL.h>
#include <memory>
#include <vector>

#include "Renderer/VulkanContext.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderGraph.hpp"
#include "Renderer/GridRenderer.hpp"
#include "Renderer/SceneRenderer.hpp"
#include "Camera/EditorCamera.hpp"
#include "Scene/Scene.hpp"
#include "ThumbnailManager.hpp"

// HouseUI Headers
#include "Core/Widget.hpp"
#include "Core/EventSystem.hpp"
#include "Rendering/UIRenderer.hpp"
#include "Layout/Box.hpp"
#include "Widgets/ViewportWidget.hpp"
#include "Widgets/TreeView.hpp"
#include "Widgets/PropertyEditor.hpp"
#include "Widgets/StatusBar.hpp"
#include "Widgets/TitleBar.hpp"

namespace we::programs::editor {
namespace UI {
    class ContentBrowser;
}

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

    // HouseUI Editor layout construction
    void BuildEditorUI();
    void UpdateOutlinerPanel();
    void UpdateInspectorPanel();
    void UpdateConsolePanel();

    SDL_Window* m_Window = nullptr;
    bool m_Running = true;

    // Core Engine Systems
    std::shared_ptr<VulkanContext> m_Context;
    std::shared_ptr<Renderer> m_Renderer;
    std::shared_ptr<RenderGraph> m_RenderGraph;
    std::shared_ptr<GridRenderer> m_GridRenderer;
    std::shared_ptr<SceneRenderer> m_SceneRenderer;
    std::shared_ptr<EditorCamera> m_Camera;
    std::shared_ptr<Scene> m_Scene;

    // HouseUI System
    std::shared_ptr<UI::Widget> m_RootWidget;
    std::shared_ptr<UI::EventSystem> m_UIEventSystem;
    std::unique_ptr<UI::UIRenderer> m_UIRenderer;
    std::unique_ptr<ThumbnailManager> m_ThumbnailManager;

    // References to dynamic panels for real-time updates
    std::shared_ptr<UI::TreeView> m_OutlinerTreeView;
    std::shared_ptr<UI::PropertyEditor> m_PropertyEditor;
    std::shared_ptr<UI::ContentBrowser> m_ContentBrowser;
    std::shared_ptr<UI::Box> m_ConsoleList;
    std::shared_ptr<UI::StatusBar> m_StatusBar;
    std::shared_ptr<UI::TitleBar> m_TitleBar;
    std::shared_ptr<UI::ViewportWidget> m_ViewportWidget; // for FlushPendingResize()

    size_t m_LastEntityCount = 0;
    int m_LastSelectedEntity = -1;
};

} // namespace we::programs::editor
