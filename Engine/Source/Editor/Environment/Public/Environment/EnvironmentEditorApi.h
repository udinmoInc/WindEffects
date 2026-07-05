#pragma once

#include "Environment/Export.hpp"

#include "Core/Widget.hpp"
#include <memory>

namespace we::runtime::renderer {
class SceneRenderer;
}
namespace we::runtime::scene {
class Scene;
}
namespace we::UI {
class PropertyEditor;
class TreeView;
}

namespace we::editor::environment {

ENVIRONMENT_API void InitializeEditor(
    const std::shared_ptr<we::runtime::scene::Scene>& scene,
    const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer,
    const std::shared_ptr<we::UI::TreeView>& outliner,
    const std::shared_ptr<we::UI::PropertyEditor>& details);

ENVIRONMENT_API std::shared_ptr<we::UI::Widget> CreateEnvironmentToolbarMenu();
ENVIRONMENT_API void TickEditor();

} // namespace we::editor::environment
