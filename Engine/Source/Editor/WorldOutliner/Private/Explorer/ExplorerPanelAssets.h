// ==============================================================================
// WindEffects — WorldOutliner — ExplorerPanelAssets
// Internal implementation for the WorldOutliner module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "RHI/Types.h"

#include <memory>

namespace we::editor::contentbrowser { class TreeView; }

namespace we::programs::editor {

we::rhi::RHIDescriptorSetHandle GetExplorerBrandLogo();
float GetExplorerBrandLogoLogicalSize();
void RegisterExplorerTreeView(const std::shared_ptr<::we::editor::contentbrowser::TreeView>& treeView);

} // namespace we::programs::editor
