// ==============================================================================
// WindEffects — PropertyEditor — DetailsSceneBinding
// Public API surface for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
