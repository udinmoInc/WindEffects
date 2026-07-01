#pragma once

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

void InitializeEditor(
    const std::shared_ptr<we::runtime::scene::Scene>& scene,
    const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer,
    const std::shared_ptr<we::UI::TreeView>& outliner,
    const std::shared_ptr<we::UI::PropertyEditor>& details);

std::shared_ptr<we::UI::Widget> CreateEnvironmentToolbarMenu();
void TickEditor();

} // namespace we::editor::environment
