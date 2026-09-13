// ==============================================================================
// WindEffects — Editor — EditorCompositionProbes
// Internal implementation for the Editor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <KindUI/EditorUI.h>
#include <memory>

namespace we::programs::editor {

void RegisterEditorCompositionProbes(const std::shared_ptr<::we::runtime::kindui::Widget>& root);

} // namespace we::programs::editor
