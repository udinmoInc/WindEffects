// ==============================================================================
// WindEffects — KindUI — ToolbarIconButton
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widgets/ToolbarGlyphButton.h"

#include <functional>

namespace we::runtime::kindui {

class KINDUI_API ToolbarIconButton : public ToolbarGlyphButton {
public:
    explicit ToolbarIconButton(WindIconRef icon, const char* tooltip = nullptr)
        : ToolbarGlyphButton(
            icon,
            StyleRole::IconButton,
            MetricToken::IconButtonSize,
            MetricToken::IconSizeToolbar)
    {
        (void)tooltip;
    }

    void SetOnClicked(std::function<void()> callback);
    void SetEnabled(bool enabled) { Widget::SetEnabled(enabled); }
    void SetSelected(bool selected) { ToolbarGlyphButton::SetSelected(selected); }
    [[nodiscard]] bool IsEnabled() const { return Widget::IsEnabled(); }
    [[nodiscard]] bool IsSelected() const { return Widget::IsSelected(); }
};

} // namespace we::runtime::kindui
