// ==============================================================================
// WindEffects — EditorShell — EditorTheme
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include <KindUI/EditorUI.h>
namespace we::editor::services {

/// Editor product theme id over GraphiteDark. Color/metric resolution is owned by GraphiteDarkTheme.
class EDITORSHELL_API EditorTheme final : public we::runtime::kindui::GraphiteDarkTheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "Editor"; }
};

} // namespace we::editor::services
