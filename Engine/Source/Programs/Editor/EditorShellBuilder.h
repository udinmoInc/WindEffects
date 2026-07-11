#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"

#include <functional>
#include <memory>

struct SDL_Window;

namespace WindEffects::Editor::UI {
class Widget;
class OverlayHost;
class OverlayRenderer;
class EventSystem;
class Panel;
class TitleBar;
class StatusBar;
}

namespace we::runtime::renderer {
class Renderer;
}

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::engine {
class EditorCamera;
}

namespace WindEffects::Editor::UI {

struct EditorShellDependencies {
    SDL_Window* window = nullptr;
    we::runtime::renderer::Renderer* renderer = nullptr;
    std::shared_ptr<we::runtime::scene::Scene> scene;
    std::shared_ptr<we::runtime::engine::EditorCamera> camera;
    WindEffects::Editor::UI::OverlayRenderer* overlayRenderer = nullptr;
    std::shared_ptr<WindEffects::Editor::UI::EventSystem> eventSystem;
    float dpiScale = 1.0f;

    std::function<void()> onCreateNewLevel;
    std::function<void(std::shared_ptr<WindEffects::Editor::UI::Widget>& viewportWidget)> onViewportCreated;
    std::function<void(const DockLayoutBuildResult& layout)> onLayoutBuilt;
};

struct EditorShellResult {
    std::shared_ptr<WindEffects::Editor::UI::Widget> rootWidget;
    std::shared_ptr<WindEffects::Editor::UI::OverlayHost> overlayHost;
    std::shared_ptr<WindEffects::Editor::UI::TitleBar> titleBar;
    std::shared_ptr<WindEffects::Editor::UI::StatusBar> statusBar;
    DockLayoutBuildResult layout;
};

class EditorShellBuilder {
public:
    static EditorShellResult Build(IEditorApplicationContext& context, const EditorShellDependencies& deps);
};

} // namespace WindEffects::Editor::UI
