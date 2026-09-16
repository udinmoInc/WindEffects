// ==============================================================================
// WindEffects — KindUI — StyleResolver
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Theme/StyleResolver.h"
#include "Theming/StyleClass.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Theme/DesignToken.h"

#include <algorithm>

namespace we::runtime::kindui {

StyleResolver::StyleResolver(std::shared_ptr<IKindUITheme> theme)
    : m_Theme(std::move(theme)) {}

void StyleResolver::SetDpiScale(float scale) {
    m_DpiScale = std::clamp(scale, 1.0f, 3.0f);
}

float StyleResolver::Scaled(float logicalValue) const {
    return logicalValue * m_DpiScale;
}

ResolvedStyle StyleResolver::ResolveClass(std::string_view className) const {
    return StyleResolve::FromClass(className, *m_Theme, m_DpiScale);
}

ResolvedStyle StyleResolver::Resolve(StyleRole role) const {
    const uint32_t roleIdx = static_cast<uint32_t>(role);
    const uint64_t currentVer = GetThemeCacheVersion();
    if (m_CachedThemeVersion == currentVer && m_CachedDpiScale == m_DpiScale && roleIdx < 64 && m_StyleCacheValid[roleIdx]) {
        return m_StyleCache[roleIdx];
    }

    if (m_CachedThemeVersion != currentVer || m_CachedDpiScale != m_DpiScale) {
        m_StyleCacheValid.fill(false);
        m_CachedThemeVersion = currentVer;
        m_CachedDpiScale = m_DpiScale;
    }

    ResolvedStyle style{};
    const auto& theme = *m_Theme;

    switch (role) {
    case StyleRole::Window:
        style.background = ResolveColor(ColorToken::WindowBackground);
        break;
    case StyleRole::Workspace:
        style.background = ResolveColor(ColorToken::WorkspaceBackground);
        break;
    case StyleRole::Toolbar:
        style.background = ResolveColor(ColorToken::ToolbarBackground);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ToolbarHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeToolbar));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        break;
    case StyleRole::Panel:
        style.background = ResolveColor(ColorToken::PanelBackground);
        style.border = ResolveColor(ColorToken::BorderSubtle);
        // Docked panels are always square — never inherit TabTopRadius / control radii.
        style.cornerRadius = 0.0f;
        style.padding = theme.ResolvePadding(PaddingToken::PaddingPanelLeft);
        for (auto& v : {&style.padding.left, &style.padding.top, &style.padding.right, &style.padding.bottom}) {
            *v = Scaled(*v);
        }
        break;
    case StyleRole::PanelHeader:
        style.background = ResolveColor(ColorToken::HeaderBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::PanelHeaderHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        style.cornerRadius = 0.0f;
        break;
    case StyleRole::Tab:
    case StyleRole::DockTab:
        style.background = ResolveColor(ColorToken::TabBackground);
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        // Tab chip top radius only — not applied to panel containers.
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::TabTopRadius));
        break;
    case StyleRole::TabActive:
    case StyleRole::DockTabActive:
        style.background = ResolveColor(ColorToken::TabActiveBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderLight);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::TabTopRadius));
        break;
    case StyleRole::Button:
    case StyleRole::ButtonSecondary:
        style.background = ResolveColor(ColorToken::ControlBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderDefault);
        style.borderWidth = 1.0f;
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonHover:
        style.background = ResolveColor(ColorToken::HoverBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderLight);
        break;
    case StyleRole::IconButton:
        style.background = Color::Transparent();
        style.icon = ResolveColor(ColorToken::IconSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::IconButtonSize));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::IconButtonRadius));
        break;
    case StyleRole::IconButtonHover:
        style.background = Color::Transparent();
        style.icon = ResolveColor(ColorToken::IconSecondary);
        style.border = Color::Transparent();
        break;
    case StyleRole::IconButtonPressed:
        style.background = Color::Transparent();
        style.icon = ResolveColor(ColorToken::IconSecondary);
        break;
    case StyleRole::NavigationButton:
        style.background = Color::Transparent();
        style.icon = ResolveColor(ColorToken::IconSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::NavigationButtonSize));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::IconButtonRadius));
        break;
    case StyleRole::Input:
        style.background = ResolveColor(ColorToken::InputBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::SearchBoxHeight));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        break;
    case StyleRole::SearchBox:
        style.background = ResolveColor(ColorToken::InputBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::SearchBoxHeight));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::SearchBoxHeight)) * 0.5f;
        break;
    case StyleRole::StatusBar:
        style.background = ResolveColor(ColorToken::StatusBarBackground);
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::StatusBarHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        break;
    case StyleRole::MenuBar:
        style.background = ResolveColor(ColorToken::WindowBackground);
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeMenu));
        break;
    case StyleRole::MenuItem:
        style.background = Color::Transparent();
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeMenu));
        break;
    case StyleRole::Popup:
        style.background = ResolveColor(ColorToken::PopupBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderLight);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.elevation = theme.ResolveElevation(ElevationToken::Popup);
        break;
    case StyleRole::Tooltip:
        style.background = ResolveColor(ColorToken::TooltipBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeCaption));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.elevation = theme.ResolveElevation(ElevationToken::Popup);
        break;
    case StyleRole::Modal:
        style.background = ResolveColor(ColorToken::PopupBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderSubtle);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::WindowCornerRadius));
        style.elevation = theme.ResolveElevation(ElevationToken::Overlay);
        break;
    case StyleRole::Gizmo:
        style.background = ResolveColor(ColorToken::GizmoBackground);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ContentBrowser:
        style.background = ResolveColor(ColorToken::PanelBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        break;
    case StyleRole::Splitter:
        style.background = ResolveColor(ColorToken::BorderDefault);
        break;
    case StyleRole::Separator:
        style.background = ResolveColor(ColorToken::Separator);
        break;
    case StyleRole::TextPrimary:
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        break;
    case StyleRole::TextSecondary:
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        break;
    case StyleRole::TextCaption:
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        break;
    case StyleRole::TextHint:
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeCaption));
        break;
    case StyleRole::TextDisabled:
        style.foreground = ResolveColor(ColorToken::TextDisabled);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        break;
    case StyleRole::ButtonGhost:
        style.background = Color::Transparent();
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonDanger:
        style.background = ResolveColor(ColorToken::ButtonDangerBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::ButtonDangerHover);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ToolbarButton:
        style.background = ResolveColor(ColorToken::ControlBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderDefault);
        style.borderWidth = 1.0f;
        style.height = Scaled(theme.ResolveMetric(MetricToken::HeaderControlHeight));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeToolbar));
        break;
    case StyleRole::Card:
        style.background = ResolveColor(ColorToken::CardBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderSubtle);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        {
            const float pad = Scaled(theme.ResolveMetric(MetricToken::CardPadding));
            style.padding = { pad, pad, pad, pad };
        }
        style.elevation = theme.ResolveElevation(ElevationToken::Card);
        break;
    case StyleRole::CardHover:
        style.background = ResolveColor(ColorToken::HoverBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderSubtle);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.elevation = theme.ResolveElevation(ElevationToken::Card);
        break;
    case StyleRole::TableHeader:
        style.background = ResolveColor(ColorToken::CategoryBackground);
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::MenuItemHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeCaption));
        break;
    case StyleRole::TableRow:
        style.background = Color::Transparent();
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        style.cornerRadius = 0.0f;
        break;
    case StyleRole::TableRowHover:
        style.background = ResolveColor(ColorToken::HoverBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.cornerRadius = 0.0f;
        break;
    case StyleRole::TableRowSelected:
        style.background = ResolveColor(ColorToken::SelectedBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = Color::Transparent();
        style.borderWidth = 0.0f;
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.cornerRadius = 0.0f;
        break;
    case StyleRole::SectionHeader:
        style.background = ResolveColor(ColorToken::CategoryBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeCategory));
        style.bold = false;
        style.height = Scaled(theme.ResolveMetric(MetricToken::CategoryHeaderHeight));
        break;
    case StyleRole::PropertyRow:
        style.background = Color::Transparent();
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::FormRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeProperty));
        break;
    case StyleRole::SidebarItem:
        style.background = Color::Transparent();
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.icon = ResolveColor(ColorToken::IconSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::FormRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::SidebarItemActive:
        style.background = ResolveColor(ColorToken::SelectedBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.icon = ResolveColor(ColorToken::IconSecondary);
        style.border = ResolveColor(ColorToken::IconSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::FormRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.bold = false;
        break;
    case StyleRole::WindowHeader:
        style.background = ResolveColor(ColorToken::HeaderBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::TitleBarHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeWindow));
        break;
    case StyleRole::Checkbox:
    case StyleRole::ToggleSwitch:
        style.background = ResolveColor(ColorToken::InputBackground);
        // Label/text beside checkbox stays neutral; accent is fill-only at paint time.
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderDefault);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.height = Scaled(theme.ResolveMetric(MetricToken::ControlHeightCompact));
        style.iconSize = theme.ResolveMetric(MetricToken::CheckboxGlyphSize);
        break;
    case StyleRole::Scrollbar:
        style.background = ResolveColor(ColorToken::ScrollbarTrack);
        style.foreground = ResolveColor(ColorToken::ScrollbarThumb);
        style.border = ResolveColor(ColorToken::ScrollbarThumbHover);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ScrollbarWidth));
        break;
    case StyleRole::TreeItem:
        style.background = Color::Transparent();
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        style.iconSize = theme.ResolveMetric(MetricToken::IconSizeToolbar);
        break;
    case StyleRole::TreeItemSelected:
        style.background = ResolveColor(ColorToken::SelectedBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        break;
    default:
        break;
    }

    if (style.border.a > 0.01f && style.borderWidth <= 0.0f) {
        style.borderWidth = Scaled(theme.ResolveMetric(MetricToken::BorderWidth));
    }
    if (roleIdx < 64) {
        m_StyleCache[roleIdx] = style;
        m_StyleCacheValid[roleIdx] = true;
    }
    return style;
}

} // namespace we::runtime::kindui

