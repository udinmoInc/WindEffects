// ==============================================================================
// WindEffects — KindUI — DesignSystem
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::runtime::kindui::ds {

// Centralized semantic design-system accessors.
// All UI chrome must consume these namespaces — never duplicate metric literals.

namespace Spacing {
[[nodiscard]] inline float None() { return 0.0f; }
[[nodiscard]] inline float Xs() { return ResolveSpacing(SpacingToken::ExtraSmall); }
[[nodiscard]] inline float Sm() { return ResolveSpacing(SpacingToken::Small); }
[[nodiscard]] inline float Md() { return ResolveSpacing(SpacingToken::Medium); }
[[nodiscard]] inline float Lg() { return ResolveSpacing(SpacingToken::Large); }
[[nodiscard]] inline float Xl() { return ResolveSpacing(SpacingToken::ExtraLarge); }
[[nodiscard]] inline float Xxl() { return ResolveSpacing(SpacingToken::Huge); }
[[nodiscard]] inline float Space1() { return ResolveMetric(MetricToken::Space1); }
[[nodiscard]] inline float Space2() { return ResolveMetric(MetricToken::Space2); }
[[nodiscard]] inline float Space3() { return ResolveMetric(MetricToken::Space3); }
[[nodiscard]] inline float SectionGap() { return ResolveMetric(MetricToken::SectionGap); }
[[nodiscard]] inline float ContentGap() { return ResolveMetric(MetricToken::ContentGap); }
[[nodiscard]] inline float FormRowGap() { return ResolveMetric(MetricToken::FormRowGap); }
[[nodiscard]] inline float ButtonSpacing() { return ResolveMetric(MetricToken::ButtonSpacing); }
[[nodiscard]] inline float ButtonGroupSpacing() { return ResolveMetric(MetricToken::ButtonGroupSpacing); }
}

namespace Sizing {
[[nodiscard]] inline float Compact() { return ResolveControlHeight(ControlSize::Compact); }
[[nodiscard]] inline float Normal() { return ResolveControlHeight(ControlSize::Default); }
[[nodiscard]] inline float Large() { return ResolveControlHeight(ControlSize::Large); }
[[nodiscard]] inline float ListRow() { return ResolveMetric(MetricToken::ListRowHeight); }
[[nodiscard]] inline float FormRow() { return ResolveMetric(MetricToken::FormRowHeight); }
[[nodiscard]] inline float Button() { return ResolveMetric(MetricToken::ButtonHeight); }
[[nodiscard]] inline float IconButton() { return ResolveMetric(MetricToken::IconButtonSize); }
[[nodiscard]] inline float Input() { return ResolveMetric(MetricToken::SearchBoxHeight); }
}

namespace Typography {
[[nodiscard]] inline float Menu() { return ResolveFontSize(TypographyToken::Menu); }
[[nodiscard]] inline float Toolbar() { return ResolveFontSize(TypographyToken::Toolbar); }
[[nodiscard]] inline float Body() { return ResolveFontSize(TypographyToken::Body); }
[[nodiscard]] inline float Caption() { return ResolveFontSize(TypographyToken::Caption); }
[[nodiscard]] inline float Small() { return ResolveMetric(MetricToken::TextSizeSmall); }
[[nodiscard]] inline float Tabs() { return ResolveMetric(MetricToken::TextSizeTabs); }
[[nodiscard]] inline float Header() { return ResolveMetric(MetricToken::TextSizeHeader); }
[[nodiscard]] inline float Property() { return ResolveMetric(MetricToken::TextSizeProperty); }
}

namespace IconSizing {
[[nodiscard]] inline float Search() { return ResolveMetric(MetricToken::IconSizeSearch); }
[[nodiscard]] inline float Toolbar() { return ResolveMetric(MetricToken::IconSizeToolbar); }
[[nodiscard]] inline float Primary() { return ResolveMetric(MetricToken::IconSizePrimary); }
[[nodiscard]] inline float Tree() { return ResolveMetric(MetricToken::IconSizeTree); }
[[nodiscard]] inline float Navigation() { return ResolveMetric(MetricToken::IconSizeNavigation); }
[[nodiscard]] inline float VerySmall() { return ResolveMetric(MetricToken::IconSizeVerySmall); }
[[nodiscard]] inline float WindowControl() { return ResolveMetric(MetricToken::IconSizeWindowControl); }
[[nodiscard]] inline float CheckMark() { return ResolveMetric(MetricToken::CheckMarkSize); }
[[nodiscard]] inline Color ContactShadow() { return ResolveColor(ColorToken::IconContactShadow); }
}

namespace Border {
[[nodiscard]] inline float Width() { return ResolveMetric(MetricToken::BorderWidth); }
[[nodiscard]] inline float PanelDivider() { return ResolveMetric(MetricToken::PanelDividerWidth); }
[[nodiscard]] inline float Splitter() { return ResolveMetric(MetricToken::SplitterThickness); }
[[nodiscard]] inline float FocusRing() { return ResolveMetric(MetricToken::FocusRingWidth); }
[[nodiscard]] inline Color Default() { return ResolveSurfaceColor(SurfaceRole::Border); }
[[nodiscard]] inline Color Subtle() { return ResolveColor(ColorToken::BorderSubtle); }
[[nodiscard]] inline Color Light() { return ResolveColor(ColorToken::BorderLight); }
[[nodiscard]] inline Color Focus() { return ResolveColor(ColorToken::BorderFocus); }
[[nodiscard]] inline Color Separator() { return ResolveSurfaceColor(SurfaceRole::Separator); }
}

namespace Radius {
[[nodiscard]] inline float Small() { return ResolveRadius(RadiusToken::Small); }
[[nodiscard]] inline float Medium() { return ResolveRadius(RadiusToken::Medium); }
[[nodiscard]] inline float Large() { return ResolveRadius(RadiusToken::Large); }
[[nodiscard]] inline float Window() { return ResolveMetric(MetricToken::WindowCornerRadius); }
[[nodiscard]] inline float IconButton() { return ResolveMetric(MetricToken::IconButtonRadius); }
}

namespace Surface {
[[nodiscard]] inline Color Window() { return ResolveSurfaceColor(SurfaceRole::Window); }
[[nodiscard]] inline Color Workspace() { return ResolveSurfaceColor(SurfaceRole::Workspace); }
[[nodiscard]] inline Color DockChrome() { return ResolveSurfaceColor(SurfaceRole::DockChrome); }
[[nodiscard]] inline Color Panel() { return ResolveSurfaceColor(SurfaceRole::Panel); }
[[nodiscard]] inline Color Secondary() { return ResolveSurfaceColor(SurfaceRole::Recessed); }
[[nodiscard]] inline Color Header() { return ResolveSurfaceColor(SurfaceRole::PanelHeader); }
[[nodiscard]] inline Color Toolbar() { return ResolveSurfaceColor(SurfaceRole::Toolbar); }
[[nodiscard]] inline Color TabInactive() { return ResolveSurfaceColor(SurfaceRole::TabInactive); }
[[nodiscard]] inline Color TabActive() { return ResolveSurfaceColor(SurfaceRole::TabActive); }
[[nodiscard]] inline Color Card() { return ResolveSurfaceColor(SurfaceRole::Control); }
[[nodiscard]] inline Color Input() { return ResolveSurfaceColor(SurfaceRole::Input); }
[[nodiscard]] inline Color Popup() { return ResolveSurfaceColor(SurfaceRole::Popup); }
[[nodiscard]] inline Color Hover() { return ResolveSurfaceColor(SurfaceRole::ControlHover); }
[[nodiscard]] inline Color Pressed() { return ResolveSurfaceColor(SurfaceRole::ControlPressed); }
[[nodiscard]] inline Color Selected() { return ResolveSurfaceColor(SurfaceRole::Selected); }
[[nodiscard]] inline Color Disabled() { return ResolveSurfaceColor(SurfaceRole::Disabled); }
}

namespace Control {
[[nodiscard]] inline float HeightCompact() { return ResolveMetric(MetricToken::ControlHeightCompact); }
[[nodiscard]] inline float HeightDefault() { return ResolveMetric(MetricToken::ButtonHeight); }
[[nodiscard]] inline float HeightLarge() { return ResolveMetric(MetricToken::ControlHeightLarge); }
[[nodiscard]] inline float PaddingH() { return ResolveMetric(MetricToken::ButtonPaddingHorizontal); }
}

namespace Panel {
[[nodiscard]] inline float HeaderHeight() { return ResolveMetric(MetricToken::PanelHeaderHeight); }
[[nodiscard]] inline float ToolbarHeight() { return ResolveMetric(MetricToken::PanelToolbarHeight); }
[[nodiscard]] inline float Padding() { return ResolvePadding(PaddingToken::Panel).left; }
[[nodiscard]] inline Color Background() { return ResolveSurfaceColor(SurfaceRole::Panel); }
[[nodiscard]] inline Color ContentWellBackground() { return ResolveSurfaceColor(SurfaceRole::Recessed); }
[[nodiscard]] inline Color PrimaryContentBackground() { return ResolveSurfaceColor(SurfaceRole::Panel); }
[[nodiscard]] inline Color NavigationBackground() { return ResolveSurfaceColor(SurfaceRole::Recessed); }
[[nodiscard]] inline Color ToolbarBackground() { return ResolveSurfaceColor(SurfaceRole::Toolbar); }
[[nodiscard]] inline Color ListLabelBandBackground() { return ResolveColor(ColorToken::ListLabelBandBackground); }
}

namespace Header {
[[nodiscard]] inline float Height() { return ResolveMetric(MetricToken::PanelHeaderHeight); }
[[nodiscard]] inline float ControlHeight() { return ResolveMetric(MetricToken::HeaderControlHeight); }
[[nodiscard]] inline Color Background() { return ResolveSurfaceColor(SurfaceRole::PanelHeader); }
[[nodiscard]] inline Color ActiveTabLine() { return ResolveColor(ColorToken::ActiveTabLine); }
}

namespace Tab {
[[nodiscard]] inline float Height() { return ResolveMetric(MetricToken::PanelTabHeight); }
[[nodiscard]] inline float Gap() { return ResolveMetric(MetricToken::TabGap); }
[[nodiscard]] inline float TopRadius() { return ResolveMetric(MetricToken::TabTopRadius); }
[[nodiscard]] inline float ActiveIndicatorHeight() { return ResolveMetric(MetricToken::TabActiveIndicatorHeight); }
[[nodiscard]] inline Color InactiveBackground() { return ResolveSurfaceColor(SurfaceRole::TabInactive); }
[[nodiscard]] inline Color ActiveBackground() { return ResolveSurfaceColor(SurfaceRole::TabActive); }
}

namespace Toolbar {
[[nodiscard]] inline float Height() { return ResolveMetric(MetricToken::ToolbarHeight); }
[[nodiscard]] inline float SeparatorWidth() { return ResolveMetric(MetricToken::ToolbarSeparatorWidth); }
[[nodiscard]] inline float SeparatorHeight() { return ResolveMetric(MetricToken::ToolbarSeparatorHeight); }
[[nodiscard]] inline float LabeledHeight() { return ResolveMetric(MetricToken::ToolbarLabeledHeight); }
[[nodiscard]] inline float LabeledMinWidth() { return ResolveMetric(MetricToken::ToolbarLabeledMinWidth); }
[[nodiscard]] inline Color Background() { return ResolveSurfaceColor(SurfaceRole::Toolbar); }
}

namespace Popup {
[[nodiscard]] inline float MinWidth() { return ResolveMetric(MetricToken::PopupMinWidth); }
[[nodiscard]] inline float MaxWidth() { return ResolveMetric(MetricToken::PopupMaxWidth); }
[[nodiscard]] inline float MenuItemHeight() { return ResolveMetric(MetricToken::MenuItemHeight); }
[[nodiscard]] inline float MenuPadding() { return ResolveMetric(MetricToken::MenuPadding); }
[[nodiscard]] inline Color Background() { return ResolveColor(ColorToken::PopupBackground); }
[[nodiscard]] inline float MaxHeight() { return ResolveMetric(MetricToken::PopupMaxHeight); }
}

namespace Input {
[[nodiscard]] inline float Height() { return ResolveMetric(MetricToken::SearchBoxHeight); }
[[nodiscard]] inline float PaddingH() { return ResolvePadding(PaddingToken::Input).left; }
[[nodiscard]] inline float PaddingV() { return ResolvePadding(PaddingToken::Input).top; }
[[nodiscard]] inline Color Background() { return ResolveSurfaceColor(SurfaceRole::Input); }
[[nodiscard]] inline Color Placeholder() { return ResolveTextColor(TextRole::Hint); }
[[nodiscard]] inline Color InsetInner() { return ResolveColor(ColorToken::InputInsetInner); }
[[nodiscard]] inline Color InsetOuter() { return ResolveColor(ColorToken::InputInsetOuter); }
[[nodiscard]] inline Color Outline() { return ResolveColor(ColorToken::BorderSubtle); }
}

namespace StatusBar {
[[nodiscard]] inline float Height() { return ResolveMetric(MetricToken::StatusBarHeight); }
[[nodiscard]] inline Color Background() { return ResolveSurfaceColor(SurfaceRole::StatusBar); }
}

namespace Breadcrumb {
[[nodiscard]] inline float Height() { return ResolveMetric(MetricToken::BreadcrumbBarHeight); }
}

namespace Property {
[[nodiscard]] inline float LabelColumnWidth() { return ResolveMetric(MetricToken::PropertyLabelColumnWidth); }
[[nodiscard]] inline float IndentStep() { return ResolveMetric(MetricToken::PropertyIndentStep); }
[[nodiscard]] inline float RowHeight() { return ResolveMetric(MetricToken::FormRowHeight); }
[[nodiscard]] inline float RowGap() { return ResolveMetric(MetricToken::FormRowGap); }
}

namespace Tree {
[[nodiscard]] inline float RowHeight() { return ResolveMetric(MetricToken::ListRowHeight); }
[[nodiscard]] inline float IndentWidth() { return ResolveMetric(MetricToken::TreeIndentWidth); }
}

namespace Toggle {
[[nodiscard]] inline float TrackWidth() { return ResolveMetric(MetricToken::ToggleTrackWidth); }
[[nodiscard]] inline float TrackHeight() { return ResolveMetric(MetricToken::ToggleTrackHeight); }
}

namespace Scroll {
[[nodiscard]] inline float WheelStep() { return ResolveMetric(MetricToken::ListRowHeight); }
[[nodiscard]] inline float BarWidth() { return ResolveMetric(MetricToken::ScrollbarWidth); }
}

namespace Chrome {
[[nodiscard]] inline float SeparationGap() { return ResolveMetric(MetricToken::ChromeSeparationGap); }
[[nodiscard]] inline float SeparationGapWide() { return ResolveMetric(MetricToken::ChromeSeparationGapWide); }
}

namespace RowHeight {
/// Unified row-height system for consistent UI across all panels (unscaled base metrics)
[[nodiscard]] inline float Standard() { return ResolveMetric(MetricToken::FormRowHeight); }
[[nodiscard]] inline float Compact() { return ResolveMetric(MetricToken::ControlHeightCompact); }
[[nodiscard]] inline float SectionHeader() { return ResolveMetric(MetricToken::FormRowHeight); }
[[nodiscard]] inline float Toolbar() { return ResolveMetric(MetricToken::PanelToolbarHeight); }
[[nodiscard]] inline float Tab() { return ResolveMetric(MetricToken::PanelTabHeight); }
[[nodiscard]] inline float ListItem() { return ResolveMetric(MetricToken::ListRowHeight); }
[[nodiscard]] inline float ArrayItem() { return ResolveMetric(MetricToken::ListRowHeight); }
[[nodiscard]] inline float Nested() { return ResolveMetric(MetricToken::FormRowHeight); }
}

}
