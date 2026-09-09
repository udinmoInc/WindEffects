// ==============================================================================
// WindEffects — Viewport — ViewportToolbar
// Public API surface for the Viewport module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Viewport/Export.h"
#include <memory>

namespace we::runtime::kindui {
class Widget;
}

namespace we::programs::editor {

/// Builds the viewport-local control strip (perspective, camera, transform, show).
VIEWPORT_API std::shared_ptr<::we::runtime::kindui::Widget> CreateViewportToolbar();

} // namespace we::programs::editor
