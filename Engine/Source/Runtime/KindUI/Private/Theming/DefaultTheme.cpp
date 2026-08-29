#include "KindUI/Theming/DefaultTheme.h"
#include "KindUI/Theming/Palette.h"

namespace we::runtime::kindui {
using P = palette::GraphiteDark;

Color DefaultTheme::ResolveColor(ColorToken token) const {
    switch (token) {
    case ColorToken::WindowBackground:
    case ColorToken::WorkspaceBackground:
    case ColorToken::StatusBarBackground:
        return P::Window;
    case ColorToken::ScrollbarTrack:
        return P::ScrollbarTrack;
    case ColorToken::PanelBackground:
        return P::Panel;
    case ColorToken::SecondarySurface:
    case ColorToken::ButtonPrimaryBackground:
        return P::Secondary;
    case ColorToken::CardBackground:
    case ColorToken::PopupBackground:
    case ColorToken::GizmoBackground:
        return P::Card;
    case ColorToken::TooltipBackground:
        return P::TooltipBg;
    case ColorToken::DragGhostBackground:
        return P::DragGhost;
    case ColorToken::HeaderBackground:
    case ColorToken::ToolbarBackground:
        return P::Header;
    case ColorToken::ViewportToolbarBackground:
        return P::ViewportToolbar;
    case ColorToken::TabBackground:
        return P::TabInactive;
    case ColorToken::InputBackground:
    case ColorToken::ControlBackground:
    case ColorToken::PressedBackground:
    case ColorToken::ButtonPrimaryPressed:
    case ColorToken::ControlBackgroundPressed:
        return P::Input;
    case ColorToken::DisabledBackground:
    case ColorToken::ControlBackgroundDisabled:
        return {0.067f, 0.071f, 0.078f, 1.0f};
    case ColorToken::ContentBrowserFolderBody:
        return P::FolderBody;
    case ColorToken::HoverBackground:
    case ColorToken::ButtonPrimaryHover:
        return P::Hover;
    case ColorToken::ControlBackgroundHover:
        return P::ControlHover;
    case ColorToken::SelectedBackground:
    case ColorToken::ContentBrowserFolderTab:
    case ColorToken::ContentBrowserFolderPrimary:
        return P::Selected;
    case ColorToken::ControlBackgroundSelected:
        return P::ControlSelected;
    case ColorToken::Separator:
        return P::Separator;
    case ColorToken::BorderSubtle:
    case ColorToken::BorderDefault:
    case ColorToken::ContentBrowserFolderEdge:
        return P::Border;
    case ColorToken::BorderLight:
    case ColorToken::ContentBrowserFolderHighlight:
    case ColorToken::AccentPrimary:
    case ColorToken::BorderFocus:
        return P::BorderLight;
    case ColorToken::BorderError:
    case ColorToken::ButtonDangerBackground:
    case ColorToken::ErrorForeground:
    case ColorToken::CloseButtonHover:
        return P::Error;
    case ColorToken::ButtonDangerHover:
        return P::DangerHover;
    case ColorToken::ButtonDangerPressed:
        return P::DangerPressed;
    case ColorToken::TextPrimary:
    case ColorToken::TextOnAccent:
    case ColorToken::LinkForeground:
        return P::Text;
    case ColorToken::TextSecondary:
    case ColorToken::InfoColor:
    case ColorToken::PlayForeground:
        return P::TextSecondary;
    case ColorToken::TextHint:
    case ColorToken::SearchPlaceholder:
        return P::TextHint;
    case ColorToken::TextDisabled:
        return P::TextDisabled;
    case ColorToken::IconPrimary:
    case ColorToken::IconSecondary:
        return P::TextSecondary;
    case ColorToken::IconAccent:
    case ColorToken::IconHover:
    case ColorToken::IconActive:
        return P::Text;
    case ColorToken::IconDisabled:
        return P::TextDisabled;
    case ColorToken::AccentHover:
        return P::AccentHover;
    case ColorToken::ActiveTabLine:
        return P::ActiveTabLine;
    case ColorToken::SelectionHighlight:
        return P::SelectionHighlight;
    case ColorToken::Success:
        return P::Success;
    case ColorToken::Warning:
        return P::Warning;
    case ColorToken::ScrollbarThumb:
        return P::ScrollThumb;
    case ColorToken::ScrollbarThumbHover:
        return P::ScrollThumbHover;
    case ColorToken::GizmoAxisX:
        return P::GizmoAxisX;
    case ColorToken::GizmoAxisY:
        return P::GizmoAxisY;
    case ColorToken::GizmoAxisZ:
        return P::GizmoAxisZ;
    case ColorToken::HighlightSubtle:
        return P::HighlightSubtle;
    case ColorToken::ShadowPopup:
        return P::ShadowPopup;
    case ColorToken::ShadowSubtle:
        return P::ShadowSubtle;
    case ColorToken::ShadowOverlay:
        return P::ShadowOverlay;
    case ColorToken::ShadowColor:
        return P::ShadowColor;
    case ColorToken::ModalScrim:
        return P::ModalScrim;
    case ColorToken::ContentBrowserFolderShadow:
        return P::FolderShadow;
    default:
        return Color::Transparent();
    }
}

float DefaultTheme::ResolveMetric(MetricToken token) const {
    switch (token) {
    case MetricToken::CornerRadiusSmall: return 3.0f;
    case MetricToken::CornerRadiusMedium: return 4.0f;
    case MetricToken::CornerRadiusLarge:
    case MetricToken::WindowCornerRadius: return 10.0f;
    case MetricToken::TextSizeMenu:
    case MetricToken::TextSizeToolbar:
    case MetricToken::TextSizeTabs:
    case MetricToken::TextSizeProperty:
    case MetricToken::TextSizeCategory: return 12.0f;
    case MetricToken::TextSizeNormal:
    case MetricToken::TextSizeWindow:
    case MetricToken::TextSizeBody: return 14.0f;
    case MetricToken::TextSizeCaption: return 12.0f;
    case MetricToken::TextSizeSmall: return 12.0f;
    case MetricToken::TextSizeHeader: return 16.5f;
    case MetricToken::TextSizeTitle: return 33.0f;
    case MetricToken::TextCharWidthRatio: return 0.55f;
    case MetricToken::BorderWidth: return 1.0f;
    case MetricToken::FocusRingWidth: return 1.0f;
    case MetricToken::PanelHeaderHeight:
    case MetricToken::PanelTabHeight: return 24.0f;
    case MetricToken::PanelToolbarHeight: return 28.0f;
    case MetricToken::HeaderControlHeight: return 24.0f;
    case MetricToken::IconButtonSize: return 24.0f;
    case MetricToken::ButtonHeight: return 24.0f;
    case MetricToken::NavigationButtonSize: return 32.0f;
    case MetricToken::ControlHeightCompact: return 24.0f;
    case MetricToken::ControlHeightLarge: return 40.0f;
    case MetricToken::FormRowHeight: return 24.0f;
    case MetricToken::MenuItemHeight: return 24.0f;
    case MetricToken::PageMargin: return 16.0f;
    case MetricToken::SectionGap: return 24.0f;
    case MetricToken::CardPadding: return 12.0f;
    case MetricToken::ContentGap: return 12.0f;
    case MetricToken::FormRowGap: return 13.0f;
    case MetricToken::LabelHintGap: return 5.0f;
    case MetricToken::ListRowHeight: return 28.0f;
    case MetricToken::CategoryHeaderHeight: return 24.0f;
    case MetricToken::TitleBarHeight: return 34.0f;
    case MetricToken::WindowControlWidth: return 40.0f;
    case MetricToken::ToolbarHeight: return 40.0f;
    case MetricToken::SearchBoxHeight: return 24.0f;
    case MetricToken::IconSizeSearch:
    case MetricToken::IconSizeToolbar:
    case MetricToken::IconSizePrimary:
    case MetricToken::IconSizeTree:
    case MetricToken::IconSizeNavigation: return 16.0f;
    case MetricToken::IconButtonRadius: return 3.0f;
    case MetricToken::ButtonPaddingHorizontal:
    case MetricToken::Space2: return 8.0f;
    case MetricToken::ButtonSpacing: return 6.0f;
    case MetricToken::Space1: return 4.0f;
    case MetricToken::ButtonGroupSpacing: return 14.0f;
    case MetricToken::ScrollbarWidth: return 14.0f;
    case MetricToken::ScrollbarThumbMinHeight: return 20.0f;
    case MetricToken::TabTopRadius: return 2.0f;
    case MetricToken::TabActiveIndicatorHeight: return 2.0f;
    case MetricToken::StatusBarHeight: return 24.0f;
    case MetricToken::TabGap: return 2.0f;
    case MetricToken::TabStripPadH: return 0.0f;
    case MetricToken::ToolbarSeparatorHeight: return 22.0f;
    case MetricToken::ToolbarLabeledHeight: return 34.0f;
    case MetricToken::ToolbarLabeledMinWidth: return 48.0f;
    case MetricToken::BreadcrumbBarHeight: return 26.0f;
    case MetricToken::PropertyLabelColumnWidth: return 140.0f;
    case MetricToken::PropertyIndentStep: return 12.0f;
    case MetricToken::TreeIndentWidth: return 16.0f;
    case MetricToken::PopupMinWidth: return 140.0f;
    case MetricToken::PopupMaxWidth: return 340.0f;
    case MetricToken::PopupMaxHeight: return 360.0f;
    case MetricToken::TooltipMinWidth: return 180.0f;
    case MetricToken::ToggleTrackWidth: return 34.0f;
    case MetricToken::ToggleTrackHeight: return 18.0f;
    case MetricToken::CheckboxGlyphSize: return 14.0f;
    case MetricToken::PrimaryButtonHeight: return 34.0f;
    case MetricToken::ContentBrowserGridPadding: return 8.0f;
    case MetricToken::ContentBrowserGridHSpacing: return 6.0f;
    case MetricToken::ContentBrowserGridVSpacing: return 6.0f;
    case MetricToken::ContentBrowserThumbLarge: return 104.0f;
    case MetricToken::ContentBrowserThumbMedium: return 72.0f;
    case MetricToken::ContentBrowserThumbSmall: return 48.0f;
    case MetricToken::ContentBrowserCellLarge: return 112.0f;
    case MetricToken::ContentBrowserCellMedium: return 80.0f;
    case MetricToken::ContentBrowserCellSmall: return 56.0f;
    case MetricToken::DragThreshold: return 6.0f;
    case MetricToken::MenuPadding: return 4.0f;
    case MetricToken::CheckMarkSize: return 16.0f;
    case MetricToken::MenuTextIndent: return 24.0f;
    case MetricToken::Space3: return 12.0f;
    case MetricToken::Space4: return 16.0f;
    case MetricToken::Space5: return 20.0f;
    case MetricToken::Space6: return 24.0f;
    case MetricToken::HoverAnimationDamping: return 10.0f;
    case MetricToken::PressAnimationDamping: return 14.0f;
    case MetricToken::PressOffset: return 1.0f;
    case MetricToken::ShadowBlurSmall: return 4.0f;
    case MetricToken::ShadowBlurMedium: return 8.0f;
    case MetricToken::ShadowSpreadMedium: return 16.0f;
    default: return 0.0f;
    }
}

Margin DefaultTheme::ResolvePadding(PaddingToken token) const {
    switch (token) {
    case PaddingToken::Panel:
    case PaddingToken::PaddingPanelLeft:
    case PaddingToken::PaddingPanelTop:
    case PaddingToken::PaddingPanelRight:
    case PaddingToken::PaddingPanelBottom:
        return {12.0f, 12.0f, 12.0f, 12.0f};
    case PaddingToken::Card: {
        const float p = ResolveMetric(MetricToken::CardPadding);
        return {p, p, p, p};
    }
    case PaddingToken::Page: {
        const float p = ResolveMetric(MetricToken::PageMargin);
        return {p, p, p, p};
    }
    case PaddingToken::Input: {
        const float h = ResolveMetric(MetricToken::Space2);
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
        return {8.0f, 4.0f, 8.0f, 4.0f};
    default:
        return {};
    }
}

float DefaultTheme::ResolveSpacing(SpacingToken token) const {
    switch (token) {
    case SpacingToken::None:       return 0.0f;
    case SpacingToken::ExtraSmall: return 2.0f;
    case SpacingToken::Small:      return 4.0f;
    case SpacingToken::Medium:     return 8.0f;
    case SpacingToken::Large:      return 16.0f;
    case SpacingToken::ExtraLarge: return 24.0f;
    case SpacingToken::Huge:       return 32.0f;
    default:                       return 8.0f;
    }
}

float DefaultTheme::ResolveRadius(RadiusToken token) const {
    switch (token) {
    case RadiusToken::None:   return 0.0f;
    case RadiusToken::Small:   return 3.0f;
    case RadiusToken::Medium:  return 6.0f;
    case RadiusToken::Large:   return 10.0f;
    case RadiusToken::Full:    return 999.0f;
    default:                   return 4.0f;
    }
}

float DefaultTheme::ResolveFontSize(TypographyToken token) const {
    switch (token) {
    case TypographyToken::WindowTitle:
    case TypographyToken::Display:
        return ResolveMetric(MetricToken::TextSizeTitle) + 1.0f;
    case TypographyToken::PageTitle:
    case TypographyToken::Heading1:
        return ResolveMetric(MetricToken::TextSizeTitle);
    case TypographyToken::SectionTitle:
    case TypographyToken::DialogTitle:
    case TypographyToken::Heading2:
    case TypographyToken::Heading:
        return ResolveMetric(MetricToken::TextSizeHeader);
    case TypographyToken::CardTitle:
    case TypographyToken::Heading3:
        return ResolveMetric(MetricToken::TextSizeHeader) - 1.0f;
    case TypographyToken::Heading4:
    case TypographyToken::Title:
        return ResolveMetric(MetricToken::TextSizeBody) + 1.0f;
    case TypographyToken::Heading5:
    case TypographyToken::Subtitle:
    case TypographyToken::Heading6:
    case TypographyToken::Body:
    case TypographyToken::BodyStrong:
    case TypographyToken::Button:
    case TypographyToken::Link:
    case TypographyToken::Label:
        return ResolveMetric(MetricToken::TextSizeBody);
    case TypographyToken::Menu:
    case TypographyToken::Toolbar:
    case TypographyToken::Navigation:
    case TypographyToken::TableHeader:
    case TypographyToken::PropertyValue:
    case TypographyToken::Caption:
    case TypographyToken::Code:
    case TypographyToken::Console:
    case TypographyToken::Monospace:
        return ResolveMetric(MetricToken::TextSizeSmall);
    case TypographyToken::Status:
    case TypographyToken::StatusBar:
    case TypographyToken::PropertyLabel:
    case TypographyToken::Error:
    case TypographyToken::Warning:
    case TypographyToken::Success:
    case TypographyToken::Hint:
    case TypographyToken::Tooltip:
    case TypographyToken::Disabled:
    case TypographyToken::CaptionSmall:
        return ResolveMetric(MetricToken::TextSizeCaption);
    default:
        return ResolveMetric(MetricToken::TextSizeBody);
    }
}

int DefaultTheme::ResolveElevation(ElevationToken token) const {
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

float DefaultTheme::ResolveAnimationDuration(AnimationToken token) const {
    switch (token) {
    case AnimationToken::Instant: return 0.0f;
    case AnimationToken::Fast:    return 0.12f;
    case AnimationToken::Normal:  return 0.20f;
    case AnimationToken::Slow:    return 0.35f;
    default:                      return 0.15f;
    }
}

} // namespace we::runtime::kindui
