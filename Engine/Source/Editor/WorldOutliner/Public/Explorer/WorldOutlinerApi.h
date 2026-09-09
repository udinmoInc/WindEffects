// ==============================================================================
// WindEffects — WorldOutliner — WorldOutlinerApi
// Public API surface for the WorldOutliner module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WorldOutliner/Export.h"
#include "RHI/Types.h"

#include <memory>

namespace we::editor::contentbrowser { class TreeView; }

namespace we::programs::editor {

WORLDOUTLINER_API void BindExplorerBrandLogo(
    we::rhi::RHIDescriptorSetHandle logoSet,
    float logicalSize = 16.0f);
WORLDOUTLINER_API float GetExplorerDockTabLogoSize();
WORLDOUTLINER_API std::shared_ptr<::we::editor::contentbrowser::TreeView> GetExplorerTreeView();

} // namespace we::programs::editor
