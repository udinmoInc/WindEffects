#pragma once

#include "Environment/Export.h"

#include "Core/Widget.h"
#include <memory>

namespace we::runtime::renderer {
class SceneRenderer;
}
namespace we::runtime::scene {
class Scene;
}
namespace WindEffects::Editor::UI {
class PropertyEditor;
class TreeView;
}

namespace we::editor::environment {

ENVIRONMENT_API void InitializeEditor(
    const std::shared_ptr<we::runtime::scene::Scene>& scene,
    we::runtime::renderer::SceneRenderer* renderer,
    const std::shared_ptr<WindEffects::Editor::UI::TreeView>& outliner,
    const std::shared_ptr<WindEffects::Editor::UI::PropertyEditor>& details);

ENVIRONMENT_API std::shared_ptr<WindEffects::Editor::UI::Widget> CreateEnvironmentToolbarMenu();
ENVIRONMENT_API void TickEditor();

} // namespace we::editor::environment
