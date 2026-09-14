// ==============================================================================
// WindEffects — KindUI — StyleFactory
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/Style.h"

#include "KindUI/Theme/ThemeManager.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Theme/DesignToken.h"
#include "KindUI/Theme/StyleRole.h"

namespace we::runtime::kindui {
namespace {

ShadowStyle ElevationToShadow(int elevation) {
    switch (elevation) {
    case 1: return ShadowStyle::Small();
    case 2: return ShadowStyle::Medium();
    case 3: return ShadowStyle::Large();
    default: return ShadowStyle::None();
    }
}

/// Adapter: StyleRole bundles → WidgetStyle. Resolution stays in StyleResolver.
WidgetStyle FromRole(StyleRole role, StyleRole hoverRole, StyleRole pressRole) {
    const auto& styles = ThemeManager::Get().Styles();
    const ResolvedStyle base = styles.Resolve(role);
    const ResolvedStyle hover = styles.Resolve(hoverRole);
    const ResolvedStyle press = styles.Resolve(pressRole);

    WidgetStyle style;
    style.background = BackgroundStyle{ base.background, base.cornerRadius };
    style.border = BorderStyle{
        base.borderWidth,
        base.border,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius
    };
    style.text = TextStyle{ base.foreground, base.fontSize, base.bold, false };
    style.shadow = ElevationToShadow(base.elevation);
    style.padding = base.padding;
    style.backgroundHover = BackgroundStyle{ hover.background, hover.cornerRadius };
    style.backgroundPressed = BackgroundStyle{ press.background, press.cornerRadius };
    style.borderFocused = BorderStyle{
        ResolveMetric(MetricToken::FocusRingWidth),
        ResolveColor(ColorToken::BorderFocus),
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius
    };
    return style;
}

} // namespace

WidgetStyle WidgetStyle::Panel() {
    return FromRole(StyleRole::Panel, StyleRole::CardHover, StyleRole::ButtonActive);
}

WidgetStyle WidgetStyle::Button() {
    return FromRole(StyleRole::ButtonSecondary, StyleRole::ButtonHover, StyleRole::ButtonActive);
}

WidgetStyle WidgetStyle::ToolButton() {
    return FromRole(StyleRole::ToolbarButton, StyleRole::ButtonHover, StyleRole::ButtonActive);
}

WidgetStyle WidgetStyle::TextBox() {
    WidgetStyle style = FromRole(StyleRole::Input, StyleRole::Input, StyleRole::Input);
    style.padding = Margin{
        ResolveMetric(MetricToken::Space2),
        ResolveMetric(MetricToken::Space1),
        ResolveMetric(MetricToken::Space2),
        ResolveMetric(MetricToken::Space1)
    };
    return style;
}

WidgetStyle WidgetStyle::TreeItem() {
    return FromRole(StyleRole::TreeItem, StyleRole::TableRowHover, StyleRole::TreeItemSelected);
}

} // namespace we::runtime::kindui
