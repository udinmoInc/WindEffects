#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"
#include "Platform/Types.h"

#include <functional>
#include <memory>

namespace we::runtime::kindui {
using we::editor::ui::DockLayoutBuildResult;
using we::editor::ui::DockLayoutBuilder;
using we::editor::ui::EditorApplicationContext;
using we::editor::ui::IEditorApplicationContext;
using we::editor::ui::Panel;
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

namespace we::runtime::kindui {

struct EditorShellDependencies {
    we::platform::WindowId window = we::platform::WindowId::Invalid;
    we::runtime::renderer::Renderer* renderer = nullptr;
    std::shared_ptr<we::runtime::scene::Scene> scene;
    std::shared_ptr<we::runtime::engine::EditorCamera> camera;
    we::runtime::kindui::OverlayRenderer* overlayRenderer = nullptr;
    std::shared_ptr<we::runtime::kindui::EventSystem> eventSystem;
    float dpiScale = 1.0f;

    std::function<void()> onCreateNewLevel;
    std::function<void(std::shared_ptr<we::runtime::kindui::Widget>& viewportWidget)> onViewportCreated;
    std::function<void(const DockLayoutBuildResult& layout)> onLayoutBuilt;
};

struct EditorShellResult {
    std::shared_ptr<we::runtime::kindui::Widget> rootWidget;
    std::shared_ptr<we::runtime::kindui::OverlayHost> overlayHost;
    std::shared_ptr<we::runtime::kindui::TitleBar> titleBar;
    std::shared_ptr<we::runtime::kindui::StatusBar> statusBar;
    DockLayoutBuildResult layout;
};

class EditorShellBuilder {
public:
    static EditorShellResult Build(IEditorApplicationContext& context, const EditorShellDependencies& deps);
};

} // namespace we::runtime::kindui
