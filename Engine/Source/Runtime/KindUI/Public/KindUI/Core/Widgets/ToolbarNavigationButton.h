// ==============================================================================
// WindEffects — KindUI — ToolbarNavigationButton
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
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

class KINDUI_API ToolbarNavigationButton : public ToolbarGlyphButton {
public:
    explicit ToolbarNavigationButton(WindIconRef icon, const char* tooltip = nullptr)
        : ToolbarGlyphButton(
            icon,
            StyleRole::NavigationButton,
            MetricToken::NavigationButtonSize,
            MetricToken::IconSizeWindowControl)
    {
        (void)tooltip;
    }

    void SetOnClicked(std::function<void()> callback);
    void SetEnabled(bool enabled);
    void SetSelected(bool selected);
    [[nodiscard]] bool IsEnabled() const { return Widget::IsEnabled(); }
    [[nodiscard]] bool IsSelected() const { return Widget::IsSelected(); }
};

} // namespace we::runtime::kindui
