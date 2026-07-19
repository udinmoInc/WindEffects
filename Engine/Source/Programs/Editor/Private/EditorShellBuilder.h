#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Rendering/OverlayRenderer.h"
#include "Widgets/TitleBar.h"
#include "Widgets/StatusBar.h"
#include "Platform/Types.h"

#include <functional>
#include <memory>

namespace we::runtime::renderer {
class Renderer;
}

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::programs::editor {
using ::we::editor::shell::DockLayoutBuildResult;
using ::we::editor::shell::DockLayoutBuilder;
using ::we::editor::services::EditorApplicationContext;
using ::we::editor::services::IEditorApplicationContext;
using ::we::editor::panels::Panel;
using ::we::editor::shell::TitleBar;
using ::we::editor::shell::StatusBar;

struct EditorShellDependencies {
    we::platform::WindowId window = we::platform::WindowId::Invalid;
    we::runtime::renderer::Renderer* renderer = nullptr;
    std::shared_ptr<we::runtime::scene::Scene> scene;
    std::shared_ptr<we::runtime::engine::EditorCamera> camera;
    ::we::runtime::kindui::OverlayRenderer* overlayRenderer = nullptr;
    std::shared_ptr<::we::runtime::kindui::EventSystem> eventSystem;
    float dpiScale = 1.0f;

    std::function<void()> onCreateNewLevel;
    std::function<void()> onOpenProject;
    std::function<void()> onOpenProjectManager;
    std::function<void()> onUndo;
    std::function<void()> onRedo;
    std::function<void(std::shared_ptr<::we::runtime::kindui::Widget>& viewportWidget)> onViewportCreated;
    std::function<void(const DockLayoutBuildResult& layout)> onLayoutBuilt;
};

struct EditorShellResult {
    std::shared_ptr<::we::runtime::kindui::Widget> rootWidget;
    std::shared_ptr<::we::runtime::kindui::OverlayHost> overlayHost;
    std::shared_ptr<TitleBar> titleBar;
    std::shared_ptr<StatusBar> statusBar;
    DockLayoutBuildResult layout;
};

class EditorShellBuilder {
public:
    static EditorShellResult Build(IEditorApplicationContext& context, const EditorShellDependencies& deps);
};

} // namespace we::programs::editor
