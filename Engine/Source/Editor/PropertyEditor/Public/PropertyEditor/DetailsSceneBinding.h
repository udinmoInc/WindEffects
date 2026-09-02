#pragma once

#include "PropertyEditor/Export.h"
#include "PropertyEditor/IDetailsView.h"

namespace we::runtime::scene {
struct Entity;
}

namespace we::editor::property {

/// Populate the Details panel from a scene entity, including environment component bindings.
PROPERTYEDITOR_API void PopulateDetailsFromSceneEntity(IDetailsView& details, we::runtime::scene::Entity* entity);

} // namespace we::editor::property
