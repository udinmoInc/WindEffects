// ==============================================================================
// WindEffects — ToolsPanel — ToolsPanelState
// Public API surface for the ToolsPanel module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <string>
#include <unordered_map>

namespace we::programs::editor {

struct ToolsPanelState {
    std::unordered_map<std::string, bool> categoryExpanded;

    void Load();
    void Save() const;
};

} // namespace we::programs::editor
