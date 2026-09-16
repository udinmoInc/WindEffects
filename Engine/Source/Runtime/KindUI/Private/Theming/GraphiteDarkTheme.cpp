// ==============================================================================
// WindEffects — KindUI — GraphiteDarkTheme
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Theme/GraphiteDarkTheme.h"
#include "KindUI/Theme/TypographySystem.h"
#include "KindUI/Theme/PaletteRuntime.h"
#include "KindUI/Core/ColorSpace.h"

namespace we::runtime::kindui {
namespace CS = ColorSpace;

Color GraphiteDarkTheme::ResolveColor(ColorToken token) const {
    const auto& P = palette::GraphiteDarkLive();
    switch (token) {
    // ── AppBackground (workspace gutters only) ────────────────────────────────
    case ColorToken::WindowBackground:
    case ColorToken::WorkspaceBackground:
    case ColorToken::DockChromeBackground:
    case ColorToken::TabBackground:
        return CS::OpaqueSurface(P.Background);

    // ── Shared PanelSurface — panel chrome / headers / toolbars / body ────────
    case ColorToken::PanelBackground:
    case ColorToken::TabActiveBackground:
    case ColorToken::PanelRaisedBackground:
    case ColorToken::ToolbarBackground:
    case ColorToken::StatusBarBackground:
    case ColorToken::ViewportToolbarBackground:
    case ColorToken::SelectHoverBackground:
    case ColorToken::HeaderBackground:
    case ColorToken::ListLabelBandBackground:
        return CS::OpaqueSurface(P.Panel);

    // ── Category / Foldout — property categorizer headers (distinct from Panel) ─
    case ColorToken::CategoryBackground:
        return CS::OpaqueSurface(
            P.Category.a > 0.0f ? P.Category
            : (P.Foldout.a > 0.0f ? P.Foldout : P.Panel));

    // ── PanelInner / InnerPanel — nested rows, trees, navigation wells ────────
    case ColorToken::SecondarySurface:
        return CS::OpaqueSurface(P.InnerPanel.a > 0.0f ? P.InnerPanel : P.Recessed);

    // ── ButtonSurface (controls only — not large regions) ─────────────────────
    case ColorToken::ControlBackground:
        return CS::OpaqueSurface(P.ButtonPrimary);

    // ── InputSurface (intentional recessed controls only) ─────────────────────
    case ColorToken::InputBackground:
    case ColorToken::PressedBackground:
    case ColorToken::ControlBackgroundPressed:
        return CS::OpaqueSurface(P.Input);

    case ColorToken::DisabledBackground:
    case ColorToken::ControlBackgroundDisabled:
        return CS::OpaqueSurface(P.Input);

    case ColorToken::PopupBackground:
    case ColorToken::CardBackground:
    case ColorToken::GizmoBackground:
        return CS::OpaqueSurface(P.Dropdown);

    case ColorToken::TooltipBackground:
        return P.TooltipBg;
    case ColorToken::DragGhostBackground:
        return P.DragGhost;

    // ── HoverSurface ──────────────────────────────────────────────────────────
    case ColorToken::HoverBackground:
    case ColorToken::ControlBackgroundHover:
        return P.Hover;

    case ColorToken::ScrollbarTrack:
        return CS::OpaqueSurface(P.Input);
    case ColorToken::ScrollbarThumb:
        return P.ScrollbarThumb;
    case ColorToken::ScrollbarThumbHover:
        return P.ScrollbarThumbHover;
    case ColorToken::TextHint:
    case ColorToken::SearchPlaceholder:
    case ColorToken::TextSecondary:
    case ColorToken::TextDisabled:
        // Ordinary secondary UI text — never accent / Primary (selection blue).
        return P.SecondaryText.a > 0.0f ? P.SecondaryText : P.Notifications;
    case ColorToken::InfoColor:
    case ColorToken::PlayForeground:
    case ColorToken::TextPrimary:
        // Ordinary primary UI text — never fall through to accent Primary (#0068D0).
        return P.PrimaryText.a > 0.0f ? P.PrimaryText : P.Foreground;
    case ColorToken::SelectedBackground:
    case ColorToken::ControlBackgroundSelected:
        return P.Select;
    case ColorToken::SelectInactiveBackground:
        return P.SelectInactive;
    case ColorToken::SelectParentBackground:
        return P.SelectParent;
    case ColorToken::Separator:
        return P.BorderSeparator;
    case ColorToken::BorderSubtle:
        return P.BorderSubtle;
    case ColorToken::BorderDefault:
        return P.BorderDefault;
    case ColorToken::InputOutline:
        return P.InputOutline;
    case ColorToken::ContentBrowserFolderEdge:
        return P.InputOutline;
    case ColorToken::BorderLight:
        return P.BorderLight;
    case ColorToken::ContentBrowserFolderHighlight:
        return P.DropdownOutline;
    case ColorToken::BorderFocus:
        return P.BorderFocus;
    case ColorToken::AccentPrimary:
    case ColorToken::LinkForeground:
        return P.Primary;
    case ColorToken::ButtonBevelHighlight:
        return P.ButtonBevelTop;
    case ColorToken::ButtonBevelShadow:
        return P.ButtonBevelBottom;
    case ColorToken::InputInsetInner:
        return P.InputInsetInner;
    case ColorToken::InputInsetOuter:
        return P.InputInsetOuter;
    case ColorToken::IconContactShadow:
        return P.IconContactShadow;
    case ColorToken::ButtonPrimaryBackground:
        return CS::OpaqueSurface(P.ButtonPrimary);
    case ColorToken::ButtonPrimaryHover:
        return P.ButtonPrimaryHover;
    case ColorToken::ButtonPrimaryPressed:
        return P.ButtonPrimaryPress;
    case ColorToken::ButtonDangerBackground:
    case ColorToken::ErrorForeground:
    case ColorToken::CloseButtonHover:
        return P.Error;
    case ColorToken::BorderError:
        return P.BorderError;
    case ColorToken::AxisX:
        return P.AxisX;
    case ColorToken::AxisY:
        return P.AxisY;
    case ColorToken::AxisZ:
        return P.AxisZ;
    case ColorToken::ButtonDangerHover:
        return P.AccentRed;
    case ColorToken::ButtonDangerPressed:
        return P.Error;
    case ColorToken::TextOnAccent:
        return P.ForegroundHover;
    case ColorToken::IconAccent:
    case ColorToken::IconActive:
        return P.IconActiveTint;
    case ColorToken::IconHover:
        return P.IconHoverTint;
    case ColorToken::IconDisabled:
        return P.IconSubdued;
    case ColorToken::IconPrimary:
        return P.PrimaryText;
    case ColorToken::IconSecondary:
        return P.SecondaryText;
    case ColorToken::AccentHover:
        return P.PrimaryHover;
    case ColorToken::AccentOrange:
        return P.AccentOrange;
    case ColorToken::ActiveTabLine:
        return P.ActiveTabLine;
    case ColorToken::SelectionHighlight:
        return P.SelectionHighlight;
    case ColorToken::Success:
        return P.Success;
    case ColorToken::Warning:
        return P.Warning;
    case ColorToken::GizmoAxisX:
        return P.AxisX;
    case ColorToken::GizmoAxisY:
        return P.AxisY;
    case ColorToken::GizmoAxisZ:
        return P.AxisZ;
    case ColorToken::ContentBrowserFolderBody:
        return P.AccentBrown;
    case ColorToken::ContentBrowserFolderTab:
    case ColorToken::ContentBrowserFolderPrimary:
        return P.AccentFolder;
    case ColorToken::HighlightSubtle:
        return P.HighlightSubtle;
    case ColorToken::ShadowPopup:
        return P.ShadowPopup;
    case ColorToken::ShadowSubtle:
        return P.ShadowSubtle;
    case ColorToken::ShadowOverlay:
        return P.ShadowOverlay;
    case ColorToken::ShadowColor:
        return P.ShadowColor;
    case ColorToken::ModalScrim:
        return P.ModalScrim;
    case ColorToken::ContentBrowserFolderShadow:
        return P.FolderShadow;
    case ColorToken::DebugGlyphBounds:
        return P.DebugGlyphBounds;
    default:
        return Color::Transparent();
    }
}

float GraphiteDarkTheme::ResolveMetric(MetricToken token) const {
    const auto& M = palette::GraphiteDarkLiveMetrics();
    switch (token) {
    case MetricToken::CornerRadiusSmall: return M.CornerRadiusSmall;
    case MetricToken::CornerRadiusMedium: return M.CornerRadiusMedium;
    case MetricToken::CornerRadiusLarge: return M.CornerRadiusLarge;
    case MetricToken::WindowCornerRadius: return M.WindowCornerRadius;
    case MetricToken::PanelCornerRadius: return M.PanelCornerRadius;
    case MetricToken::TextSizeMenu: return TypographySystem::GetFontSize(TypographyToken::Menu);
    case MetricToken::TextSizeToolbar: return TypographySystem::GetFontSize(TypographyToken::Toolbar);
    case MetricToken::TextSizeTabs: return TypographySystem::GetFontSize(TypographyToken::Tab);
    case MetricToken::TextSizeNormal: return TypographySystem::GetFontSize(TypographyToken::Body);
    case MetricToken::TextSizeProperty: return TypographySystem::GetFontSize(TypographyToken::PropertyValue);
    case MetricToken::TextSizeCaption: return TypographySystem::GetFontSize(TypographyToken::Caption);
    case MetricToken::TextSizeWindow: return TypographySystem::GetFontSize(TypographyToken::WindowTitle);
    case MetricToken::TextSizeHeader: return TypographySystem::GetFontSize(TypographyToken::SectionTitle);
    case MetricToken::TextSizeBody: return TypographySystem::GetFontSize(TypographyToken::Body);
    case MetricToken::TextSizeSmall: return TypographySystem::GetFontSize(TypographyToken::Caption);
    case MetricToken::TextSizeCategory: return TypographySystem::GetFontSize(TypographyToken::SectionTitle);
    case MetricToken::TextSizeTitle: return TypographySystem::GetFontSize(TypographyToken::PageTitle);
    case MetricToken::TextCharWidthRatio: return 0.56f;
    case MetricToken::BorderWidth: return M.BorderWidth;
    case MetricToken::PanelDividerWidth: return M.PanelDividerWidth;
    case MetricToken::SplitterThickness: return M.SplitterThickness;
    case MetricToken::FocusRingWidth: return M.FocusRingWidth;
    case MetricToken::PanelHeaderHeight: return M.PanelHeaderHeight;
    case MetricToken::PanelTabHeight: return M.PanelTabHeight;
    case MetricToken::PanelToolbarHeight: return M.PanelToolbarHeight;
    case MetricToken::ToolbarHeight: return M.ToolbarHeight;
    case MetricToken::ViewportToolbarHeight: return M.ViewportToolbarHeight;
    case MetricToken::BreadcrumbBarHeight: return M.BreadcrumbBarHeight;
    case MetricToken::HeaderControlHeight: return M.HeaderControlHeight;
    case MetricToken::IconButtonSize: return M.IconButtonSize;
    case MetricToken::ButtonHeight: return M.ButtonHeight;
    case MetricToken::SearchBoxHeight: return M.SearchBoxHeight;
    case MetricToken::NavigationButtonSize: return M.NavigationButtonSize;
    case MetricToken::ToolbarLabeledHeight: return M.ToolbarLabeledHeight;
    case MetricToken::ControlHeightCompact: return M.ControlHeightCompact;
    case MetricToken::ControlHeightLarge: return M.ControlHeightLarge;
    case MetricToken::InputWidthCompact: return M.InputWidthCompact;
    case MetricToken::InputWidthDefault: return M.InputWidthDefault;
    case MetricToken::InputWidthLarge: return M.InputWidthLarge;
    case MetricToken::FormRowHeight: return M.FormRowHeight;
    case MetricToken::MenuItemHeight: return M.MenuItemHeight;
    case MetricToken::PageMargin: return M.PageMargin;
    case MetricToken::SectionGap: return M.SectionGap;
    case MetricToken::CardPadding: return M.CardPadding;
    case MetricToken::ContentGap: return M.ContentGap;
    case MetricToken::FormRowGap: return M.FormRowGap;
    case MetricToken::LabelHintGap: return M.LabelHintGap;
    case MetricToken::ListRowHeight: return M.ListRowHeight;
    case MetricToken::CategoryHeaderHeight: return M.CategoryHeaderHeight;
    case MetricToken::TitleBarHeight: return M.TitleBarHeight;
    case MetricToken::WindowControlWidth: return M.WindowControlWidth;
    case MetricToken::IconSizeSearch: return M.IconSizeSearch;
    case MetricToken::IconSizeTree: return M.IconSizeTree;
    case MetricToken::IconSizeToolbar: return M.IconSizeToolbar;
    case MetricToken::IconSizeNavigation: return M.IconSizeNavigation;
    case MetricToken::IconSizePrimary: return M.IconSizePrimary;
    case MetricToken::IconSizeVerySmall: return M.IconSizeVerySmall;
    case MetricToken::IconSizeWindowControl: return M.IconSizeWindowControl;
    case MetricToken::IconButtonRadius: return M.IconButtonRadius;
    case MetricToken::ButtonPaddingHorizontal: return M.ButtonPaddingHorizontal;
    case MetricToken::Space2: return M.Space2;
    case MetricToken::ButtonSpacing: return M.ButtonSpacing;
    case MetricToken::SpaceXS: return M.SpaceXS;
    case MetricToken::Space1: return M.Space1;
    case MetricToken::SpaceMD: return M.SpaceMD;
    case MetricToken::ButtonGroupSpacing: return M.ButtonGroupSpacing;
    case MetricToken::ScrollbarWidth: return M.ScrollbarWidth;
    case MetricToken::ScrollbarThumbMinHeight: return M.ScrollbarThumbMinHeight;
    case MetricToken::TabTopRadius: return M.TabTopRadius;
    case MetricToken::TabActiveIndicatorHeight: return M.TabActiveIndicatorHeight;
    case MetricToken::StatusBarHeight: return M.StatusBarHeight;
    case MetricToken::TabGap: return M.TabGap;
    case MetricToken::TabIconGap: return M.TabIconGap;
    case MetricToken::TabCloseGap: return M.TabCloseGap;
    case MetricToken::TabMinWidth: return M.TabMinWidth;
    case MetricToken::CloseGlyphSize: return M.CloseGlyphSize;
    case MetricToken::TabStripPadH: return M.TabStripPadH;
    case MetricToken::TabStripPadV: return M.TabStripPadV;
    case MetricToken::TabActiveIndicatorWidth: return M.TabActiveIndicatorWidth;
    case MetricToken::TabPaddingH: return M.TabPaddingH;
    case MetricToken::TabPaddingV: return M.TabPaddingV;
    case MetricToken::DockPanelGap: return M.DockPanelGap;
    case MetricToken::ChromeSeparationGap: return M.ChromeSeparationGap;
    case MetricToken::ChromeSeparationGapWide: return M.ChromeSeparationGapWide;
    case MetricToken::ToolbarSeparatorWidth: return M.ToolbarSeparatorWidth;
    case MetricToken::ToolbarSeparatorHeight: return M.ToolbarSeparatorHeight;
    case MetricToken::ToolbarLabeledMinWidth: return M.ToolbarLabeledMinWidth;
    case MetricToken::PropertyLabelColumnWidth: return M.PropertyLabelColumnWidth;
    case MetricToken::PropertyIndentStep: return M.PropertyIndentStep;
    case MetricToken::TreeIndentWidth: return M.TreeIndentWidth;
    case MetricToken::TreeExpanderHitSize: return M.TreeExpanderHitSize;
    case MetricToken::PopupMinWidth: return M.PopupMinWidth;
    case MetricToken::PopupMaxWidth: return M.PopupMaxWidth;
    case MetricToken::PopupMaxHeight: return M.PopupMaxHeight;
    case MetricToken::TooltipMinWidth: return M.TooltipMinWidth;
    case MetricToken::ToggleTrackWidth: return M.ToggleTrackWidth;
    case MetricToken::ToggleTrackHeight: return M.ToggleTrackHeight;
    case MetricToken::CheckboxGlyphSize: return M.CheckboxGlyphSize;
    case MetricToken::PrimaryButtonHeight: return M.PrimaryButtonHeight;
    case MetricToken::ContentBrowserGridPadding: return M.ContentBrowserGridPadding;
    case MetricToken::ContentBrowserGridHSpacing: return M.ContentBrowserGridHSpacing;
    case MetricToken::ContentBrowserGridVSpacing: return M.ContentBrowserGridVSpacing;
    case MetricToken::ContentBrowserThumbLarge: return M.ContentBrowserThumbLarge;
    case MetricToken::ContentBrowserThumbMedium: return M.ContentBrowserThumbMedium;
    case MetricToken::ContentBrowserThumbSmall: return M.ContentBrowserThumbSmall;
    case MetricToken::ContentBrowserCellLarge: return M.ContentBrowserCellLarge;
    case MetricToken::ContentBrowserCellMedium: return M.ContentBrowserCellMedium;
    case MetricToken::ContentBrowserCellSmall: return M.ContentBrowserCellSmall;
    case MetricToken::DragThreshold: return M.DragThreshold;
    case MetricToken::MenuPadding: return M.MenuPadding;
    case MetricToken::CheckMarkSize: return M.CheckMarkSize;
    case MetricToken::MenuTextIndent: return M.MenuTextIndent;
    case MetricToken::Space3: return M.Space3;
    case MetricToken::Space4: return M.Space4;
    case MetricToken::Space5: return M.Space5;
    case MetricToken::Space6: return M.Space6;
    case MetricToken::HoverAnimationDamping: return M.HoverAnimationDamping;
    case MetricToken::PressAnimationDamping: return M.PressAnimationDamping;
    case MetricToken::PressOffset: return M.PressOffset;
    case MetricToken::ShadowBlurSmall: return M.ShadowBlurSmall;
    case MetricToken::ShadowBlurMedium: return M.ShadowBlurMedium;
    case MetricToken::ShadowSpreadMedium: return M.ShadowSpreadMedium;
    default: return 0.0f;
    }
}

Margin GraphiteDarkTheme::ResolvePadding(PaddingToken token) const {
    switch (token) {
    case PaddingToken::Panel:
    case PaddingToken::PaddingPanelLeft:
    case PaddingToken::PaddingPanelTop:
    case PaddingToken::PaddingPanelRight:
    case PaddingToken::PaddingPanelBottom:
        return {8.0f, 8.0f, 8.0f, 8.0f};
    case PaddingToken::Card: {
        const float p = ResolveMetric(MetricToken::CardPadding);
        return {p, p, p, p};
    }
    case PaddingToken::Page: {
        const float p = ResolveMetric(MetricToken::PageMargin);
        return {p, p, p, p};
    }
    case PaddingToken::Input: {
        const float h = ResolveMetric(MetricToken::SpaceMD);
        const float v = ResolveMetric(MetricToken::Space1);
        return {h, v, h, v};
    }
    case PaddingToken::FormRow: {
        const float g = ResolveMetric(MetricToken::FormRowGap);
        return {0.0f, g * 0.5f, 0.0f, g * 0.5f};
    }
    case PaddingToken::Button:
    case PaddingToken::PaddingButtonLeft:
    case PaddingToken::PaddingButtonTop:
    case PaddingToken::PaddingButtonRight:
    case PaddingToken::PaddingButtonBottom:
        return {6.0f, 3.0f, 6.0f, 3.0f};
    default:
        return {};
    }
}

float GraphiteDarkTheme::ResolveSpacing(SpacingToken token) const {
    switch (token) {
    case SpacingToken::None: return 0.0f;
    case SpacingToken::ExtraSmall: return ResolveMetric(MetricToken::Space1) * 0.5f;
    case SpacingToken::Small: return ResolveMetric(MetricToken::Space1);
    case SpacingToken::Medium: return ResolveMetric(MetricToken::Space2);
    case SpacingToken::Large: return ResolveMetric(MetricToken::Space4);
    case SpacingToken::ExtraLarge: return ResolveMetric(MetricToken::Space6);
    case SpacingToken::Huge: return ResolveMetric(MetricToken::Space6) + ResolveMetric(MetricToken::Space2);
    default: return ResolveMetric(MetricToken::Space2);
    }
}

float GraphiteDarkTheme::ResolveRadius(RadiusToken token) const {
    switch (token) {
    case RadiusToken::None: return 0.0f;
    case RadiusToken::Small: return ResolveMetric(MetricToken::CornerRadiusSmall);
    case RadiusToken::Medium: return ResolveMetric(MetricToken::CornerRadiusMedium);
    case RadiusToken::Large: return ResolveMetric(MetricToken::CornerRadiusLarge);
    case RadiusToken::Full: return 999.0f;
    default: return ResolveMetric(MetricToken::CornerRadiusSmall);
    }
}

float GraphiteDarkTheme::ResolveFontSize(TypographyToken token) const {
    return TypographySystem::GetFontSize(token);
}

int GraphiteDarkTheme::ResolveElevation(ElevationToken token) const {
    switch (token) {
    case ElevationToken::None:
    case ElevationToken::Window:
    case ElevationToken::Panel:
    case ElevationToken::Control:
        return 0;
    case ElevationToken::Card:
        return 1;
    case ElevationToken::Popup:
        return 2;
    case ElevationToken::Overlay:
        return 3;
    default:
        return 0;
    }
}

float GraphiteDarkTheme::ResolveAnimationDuration(AnimationToken token) const {
    switch (token) {
    case AnimationToken::Instant: return 0.0f;
    case AnimationToken::Fast: return 0.12f;
    case AnimationToken::Normal: return 0.20f;
    case AnimationToken::Slow: return 0.35f;
    default: return 0.15f;
    }
}

} // namespace we::runtime::kindui
