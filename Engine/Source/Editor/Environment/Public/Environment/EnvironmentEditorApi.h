#pragma once

#include "Environment/Export.h"

#include "KindUI/Core/Widget.h"
#include <memory>

namespace we::runtime::scene {
class Scene;
}

namespace we::editor::contentbrowser { class TreeView; }
namespace we::editor::property { class PropertyEditor; }

namespace we::editor::environment {

ENVIRONMENT_API void InitializeEditor(
    const std::shared_ptr<we::runtime::scene::Scene>& scene,
    const std::shared_ptr<::we::editor::contentbrowser::TreeView>& outliner,
    const std::shared_ptr<::we::editor::property::PropertyEditor>& details);

ENVIRONMENT_API std::shared_ptr<we::runtime::kindui::Widget> CreateEnvironmentToolbarMenu();
ENVIRONMENT_API void TickEditor();

} // namespace we::editor::environment
