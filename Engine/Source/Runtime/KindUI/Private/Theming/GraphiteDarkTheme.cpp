#include "KindUI/Theming/GraphiteDarkTheme.h"
#include "KindUI/Theming/PaletteRuntime.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Theming/StyleResolve.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>
#include <array>
#include <unordered_map>

namespace we::runtime::kindui {
namespace CS = ColorSpace;

Color GraphiteDarkTheme::ResolveColor(ColorToken token) const {
    const auto& P = palette::GraphiteDarkLive();
    switch (token) {
    case ColorToken::WindowBackground:
        // Title / menu header strip
        return CS::OpaqueSurface(P.Background);
    case ColorToken::StatusBarBackground:
    case ColorToken::ToolbarBackground:
        return CS::OpaqueSurface(P.Panel);
    case ColorToken::WorkspaceBackground:
        return CS::OpaqueSurface(P.Background);
    case ColorToken::DockChromeBackground:
    case ColorToken::TabBackground:
        return CS::OpaqueSurface(P.Background);
    case ColorToken::ViewportToolbarBackground:
        return CS::OpaqueSurface(P.Panel);
    case ColorToken::PanelBackground:
    case ColorToken::TabActiveBackground:
    case ColorToken::SelectHoverBackground:
        return CS::OpaqueSurface(P.Panel);
    case ColorToken::SecondarySurface:
    case ColorToken::ScrollbarTrack:
        return CS::OpaqueSurface(P.Recessed);
    case ColorToken::HeaderBackground:
    case ColorToken::ListLabelBandBackground:
        return CS::OpaqueSurface(P.Background);
    case ColorToken::ControlBackground:
        return CS::OpaqueSurface(P.Dropdown);
    case ColorToken::InputBackground:
    case ColorToken::PressedBackground:
    case ColorToken::ControlBackgroundPressed:
        return CS::OpaqueSurface(P.Input);
    case ColorToken::DisabledBackground:
    case ColorToken::ControlBackgroundDisabled:
        return CS::OpaqueSurface(P.Foldout);
    case ColorToken::PopupBackground:
    case ColorToken::CardBackground:
    case ColorToken::GizmoBackground:
        return CS::OpaqueSurface(P.Dropdown);
    case ColorToken::TooltipBackground:
        return P.TooltipBg;
    case ColorToken::DragGhostBackground:
        return P.DragGhost;
    case ColorToken::HoverBackground:
    case ColorToken::ControlBackgroundHover:
    case ColorToken::ScrollbarThumb:
        return P.Hover;
    case ColorToken::ScrollbarThumbHover:
    case ColorToken::TextSecondary:
    case ColorToken::TextHint:
    case ColorToken::SearchPlaceholder:
    case ColorToken::InfoColor:
    case ColorToken::PlayForeground:
        return P.Hover2;
    case ColorToken::SelectedBackground:
    case ColorToken::ControlBackgroundSelected:
        return P.Select;
    case ColorToken::SelectInactiveBackground:
        return P.SelectInactive;
    case ColorToken::SelectParentBackground:
        return P.SelectParent;
    case ColorToken::Separator:
        return P.Background;
    case ColorToken::BorderSubtle:
    case ColorToken::BorderDefault:
    case ColorToken::ContentBrowserFolderEdge:
        return P.InputOutline;
    case ColorToken::BorderLight:
    case ColorToken::ContentBrowserFolderHighlight:
        return P.DropdownOutline;
    case ColorToken::BorderFocus:
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
    case ColorToken::ButtonPrimaryBackground:
        return CS::OpaqueSurface(P.Primary);
    case ColorToken::ButtonPrimaryHover:
        return P.PrimaryHover;
    case ColorToken::ButtonPrimaryPressed:
        return P.PrimaryPress;
    case ColorToken::ButtonDangerBackground:
    case ColorToken::BorderError:
    case ColorToken::ErrorForeground:
    case ColorToken::CloseButtonHover:
        return P.Error;
    case ColorToken::ButtonDangerHover:
        return P.AccentRed;
    case ColorToken::ButtonDangerPressed:
        return P.Error;
    case ColorToken::TextPrimary:
        return P.Foreground;
    case ColorToken::TextOnAccent:
        return P.ForegroundHover;
    case ColorToken::IconAccent:
    case ColorToken::IconActive:
        return P.IconActiveTint;
    case ColorToken::IconHover:
        return P.IconHoverTint;
    case ColorToken::TextDisabled:
        return P.Notifications;
    case ColorToken::IconDisabled:
        return P.IconSubdued;
    case ColorToken::IconPrimary:
    case ColorToken::IconSecondary:
        return P.IconNormal;
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
        return P.AccentRed;
    case ColorToken::GizmoAxisY:
        return P.AccentGreen;
    case ColorToken::GizmoAxisZ:
        return P.AccentBlue;
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
    switch (token) {
    case MetricToken::CornerRadiusSmall: return 3.0f;
    case MetricToken::CornerRadiusMedium: return 4.0f;
    case MetricToken::CornerRadiusLarge:
    case MetricToken::WindowCornerRadius: return 10.0f;
    case MetricToken::TextSizeMenu: return 11.0f;
    case MetricToken::TextSizeToolbar: return 11.0f;
    case MetricToken::TextSizeTabs: return 11.0f;
    case MetricToken::TextSizeNormal: return 11.0f;
    case MetricToken::TextSizeProperty: return 11.0f;
    case MetricToken::TextSizeCaption: return 10.0f;
    case MetricToken::TextSizeWindow: return 11.0f;
    case MetricToken::TextSizeHeader: return 12.0f;
    case MetricToken::TextSizeBody: return 12.0f;
    case MetricToken::TextSizeSmall: return 10.0f;
    case MetricToken::TextSizeCategory: return 11.0f;
    case MetricToken::TextSizeTitle: return 33.0f;
    case MetricToken::TextCharWidthRatio: return 0.56f;
    case MetricToken::BorderWidth: return 1.0f;
    case MetricToken::FocusRingWidth: return 1.0f;
    case MetricToken::PanelHeaderHeight: return 30.0f;
    case MetricToken::PanelTabHeight: return 30.0f;
    case MetricToken::PanelToolbarHeight: return 30.0f;
    case MetricToken::HeaderControlHeight: return 28.0f;
    case MetricToken::IconButtonSize: return 28.0f;
    case MetricToken::ButtonHeight: return 28.0f;
    case MetricToken::ControlHeightCompact: return 26.0f;
    case MetricToken::ControlHeightLarge: return 36.0f;
    case MetricToken::FormRowHeight: return 26.0f;
    case MetricToken::MenuItemHeight: return 26.0f;
    case MetricToken::PageMargin: return 16.0f;
    case MetricToken::SectionGap: return 12.0f;
    case MetricToken::CardPadding: return 12.0f;
    case MetricToken::ContentGap: return 8.0f;
    case MetricToken::FormRowGap: return 6.0f;
    case MetricToken::LabelHintGap: return 4.0f;
    case MetricToken::ListRowHeight: return 24.0f;
    case MetricToken::CategoryHeaderHeight: return 28.0f;
    case MetricToken::TitleBarHeight: return 32.0f;
    case MetricToken::WindowControlWidth: return 40.0f;
    case MetricToken::ToolbarHeight: return 38.0f;
    case MetricToken::SearchBoxHeight: return 28.0f;
    case MetricToken::NavigationButtonSize: return 28.0f;
    case MetricToken::IconSizeSearch: return 14.0f;
    case MetricToken::IconSizeTree:
    case MetricToken::IconSizeToolbar:
    case MetricToken::IconSizeNavigation: return 16.0f;
    case MetricToken::IconSizeVerySmall:
    case MetricToken::IconSizeWindowControl: return 12.0f;
    case MetricToken::IconSizePrimary: return 32.0f;
    case MetricToken::IconButtonRadius: return 3.0f;
    case MetricToken::ButtonPaddingHorizontal: return 4.0f;
    case MetricToken::Space2: return 8.0f;
    case MetricToken::ButtonSpacing: return 2.0f;
    case MetricToken::SpaceXS: return 2.0f;
    case MetricToken::Space1: return 4.0f;
    case MetricToken::SpaceMD: return 5.0f;
    case MetricToken::ButtonGroupSpacing: return 8.0f;
    case MetricToken::ScrollbarWidth: return 14.0f;
    case MetricToken::ScrollbarThumbMinHeight: return 20.0f;
    case MetricToken::TabTopRadius: return 6.0f;
    case MetricToken::TabActiveIndicatorHeight: return 2.0f;
    case MetricToken::StatusBarHeight: return 24.0f;
    case MetricToken::TabGap: return 3.0f;
    case MetricToken::TabStripPadH: return 0.0f;
    case MetricToken::TabStripPadV: return 4.0f;
    case MetricToken::TabActiveIndicatorWidth: return 0.0f;
    case MetricToken::TabPaddingH: return 10.0f;
    case MetricToken::TabPaddingV: return 4.0f;
    case MetricToken::DockPanelGap: return 4.0f;
    case MetricToken::ChromeSeparationGap: return 1.0f;
    case MetricToken::ChromeSeparationGapWide: return 4.0f;
    case MetricToken::ViewportToolbarHeight: return 28.0f;
    case MetricToken::ToolbarSeparatorHeight: return 24.0f;
    case MetricToken::ToolbarLabeledHeight: return 28.0f;
    case MetricToken::ToolbarLabeledMinWidth: return 36.0f;
    case MetricToken::BreadcrumbBarHeight: return 32.0f;
    case MetricToken::PropertyLabelColumnWidth: return 120.0f;
    case MetricToken::PropertyIndentStep: return 16.0f;
    case MetricToken::TreeIndentWidth: return 16.0f;
    case MetricToken::TreeExpanderHitSize: return 18.0f;
    case MetricToken::PopupMinWidth: return 140.0f;
    case MetricToken::PopupMaxWidth: return 340.0f;
    case MetricToken::PopupMaxHeight: return 360.0f;
    case MetricToken::TooltipMinWidth: return 180.0f;
    case MetricToken::ToggleTrackWidth: return 34.0f;
    case MetricToken::ToggleTrackHeight: return 18.0f;
    case MetricToken::CheckboxGlyphSize: return 14.0f;
    case MetricToken::PrimaryButtonHeight: return 28.0f;
    case MetricToken::ContentBrowserGridPadding: return 6.0f;
    case MetricToken::ContentBrowserGridHSpacing: return 6.0f;
    case MetricToken::ContentBrowserGridVSpacing: return 6.0f;
    case MetricToken::ContentBrowserThumbLarge: return 96.0f;
    case MetricToken::ContentBrowserThumbMedium: return 72.0f;
    case MetricToken::ContentBrowserThumbSmall: return 48.0f;
    case MetricToken::ContentBrowserCellLarge: return 112.0f;
    case MetricToken::ContentBrowserCellMedium: return 80.0f;
    case MetricToken::ContentBrowserCellSmall: return 56.0f;
    case MetricToken::DragThreshold: return 6.0f;
    case MetricToken::MenuPadding: return 3.0f;
    case MetricToken::CheckMarkSize: return 16.0f;
    case MetricToken::MenuTextIndent: return 20.0f;
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
    switch (token) {
    case TypographyToken::WindowTitle:
    case TypographyToken::Display:
        return ResolveMetric(MetricToken::TextSizeTitle) + 4.0f;
    case TypographyToken::PageTitle:
    case TypographyToken::Heading1:
        return ResolveMetric(MetricToken::TextSizeTitle);
    case TypographyToken::SectionTitle:
    case TypographyToken::DialogTitle:
    case TypographyToken::Heading:
    case TypographyToken::Heading2:
        return ResolveMetric(MetricToken::TextSizeHeader);
    case TypographyToken::CardTitle:
    case TypographyToken::Heading3:
        return ResolveMetric(MetricToken::TextSizeHeader);
    case TypographyToken::Heading4:
    case TypographyToken::Title:
        return ResolveMetric(MetricToken::TextSizeBody);
    case TypographyToken::Heading5:
    case TypographyToken::Subtitle:
        return ResolveMetric(MetricToken::TextSizeBody);
    case TypographyToken::Heading6:
    case TypographyToken::Body:
    case TypographyToken::BodyStrong:
    case TypographyToken::Link:
        return ResolveMetric(MetricToken::TextSizeBody);
    case TypographyToken::Button:
        return ResolveMetric(MetricToken::TextSizeNormal);
    case TypographyToken::Label:
        return ResolveMetric(MetricToken::TextSizeNormal);
    case TypographyToken::Menu:
        return ResolveMetric(MetricToken::TextSizeMenu);
    case TypographyToken::Toolbar:
    case TypographyToken::Navigation:
        return ResolveMetric(MetricToken::TextSizeToolbar);
    case TypographyToken::Caption:
    case TypographyToken::Status:
    case TypographyToken::StatusBar:
    case TypographyToken::Error:
    case TypographyToken::Warning:
    case TypographyToken::Success:
        return ResolveMetric(MetricToken::TextSizeSmall);
    case TypographyToken::Hint:
    case TypographyToken::Tooltip:
    case TypographyToken::Disabled:
    case TypographyToken::CaptionSmall:
        return ResolveMetric(MetricToken::TextSizeCaption);
    case TypographyToken::Code:
    case TypographyToken::Console:
    case TypographyToken::Monospace:
    case TypographyToken::PropertyValue:
    case TypographyToken::TableHeader:
        return ResolveMetric(MetricToken::TextSizeProperty);
    case TypographyToken::PropertyLabel:
        return ResolveMetric(MetricToken::TextSizeCaption);
    default:
        return ResolveMetric(MetricToken::TextSizeBody);
    }
}

int GraphiteDarkTheme::ResolveElevation(ElevationToken token) const {
    // Shadow intensity only — surface brightness comes from ColorToken ladder.
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

Color GraphiteDarkTheme::InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    return ResolveInteractiveBackground(hoverAnim, pressAnim, selected, ColorToken::PanelBackground);
}

Color GraphiteDarkTheme::IconForState(bool hovered, bool active) const {
    if (active) {
        return ResolveColor(ColorToken::IconActive);
    }
    if (hovered) {
        return ResolveColor(ColorToken::IconHover);
    }
    return ResolveColor(ColorToken::IconSecondary);
}

Color GraphiteDarkTheme::TextForState(bool hovered, bool active) const {
    return (active || hovered) ? ResolveColor(ColorToken::TextPrimary) : ResolveColor(ColorToken::TextSecondary);
}

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
        style.iconSize = static_cast<float>(16u);
        break;
    case StyleRole::Panel:
        style.background = ResolveColor(ColorToken::PanelBackground);
        style.border = ResolveColor(ColorToken::BorderSubtle);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
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
        break;
    case StyleRole::Tab:
    case StyleRole::DockTab:
        style.background = ResolveColor(ColorToken::TabBackground);
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        break;
    case StyleRole::TabActive:
    case StyleRole::DockTabActive:
        style.background = ResolveColor(ColorToken::TabActiveBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderLight);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        break;
    case StyleRole::Button:
        style.background = Color::Transparent();
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = static_cast<float>(16u);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonHover:
        style.background = ResolveColor(ColorToken::HoverBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::BorderLight);
        break;
    case StyleRole::ButtonActive:
        style.background = ResolveColor(ColorToken::PressedBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        break;
    case StyleRole::ButtonPrimary:
        style.background = ResolveColor(ColorToken::ButtonPrimaryBackground);
        style.foreground = ResolveColor(ColorToken::TextOnAccent);
        style.border = Color::Transparent();
        style.borderWidth = 0.0f;
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = static_cast<float>(16u);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonSecondary:
        style.background = ResolveColor(ColorToken::ControlBackground);
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.border = ResolveColor(ColorToken::BorderDefault);
        style.borderWidth = 1.0f;
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = static_cast<float>(16u);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::IconButton:
        style.background = Color::Transparent();
        style.icon = ResolveColor(ColorToken::IconSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::IconButtonSize));
        style.iconSize = static_cast<float>(16u);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::IconButtonRadius));
        break;
    case StyleRole::IconButtonHover:
        style.background = ResolveColor(ColorToken::HoverBackground);
        style.icon = ResolveColor(ColorToken::IconHover);
        style.border = ResolveColor(ColorToken::BorderLight);
        break;
    case StyleRole::IconButtonPressed:
        style.background = ResolveColor(ColorToken::PressedBackground);
        style.icon = ResolveColor(ColorToken::IconActive);
        break;
    case StyleRole::NavigationButton:
        style.background = Color::Transparent();
        style.icon = ResolveColor(ColorToken::IconSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::NavigationButtonSize));
        style.iconSize = static_cast<float>(16u);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::IconButtonRadius));
        break;
    case StyleRole::Input:
        style.background = ResolveColor(ColorToken::InputBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::SearchBoxHeight));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
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
        style.border = ResolveColor(ColorToken::BorderSubtle);
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
        style.foreground = ResolveColor(ColorToken::TextHint);
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
        style.iconSize = static_cast<float>(16u);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonDanger:
        style.background = ResolveColor(ColorToken::ButtonDangerBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.border = ResolveColor(ColorToken::ButtonDangerHover);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = static_cast<float>(16u);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ToolbarButton:
        style.background = Color::Transparent();
        style.foreground = ResolveColor(ColorToken::TextSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::HeaderControlHeight));
        style.iconSize = static_cast<float>(16u);
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
        style.border = ResolveColor(ColorToken::BorderLight);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.elevation = theme.ResolveElevation(ElevationToken::Card);
        break;
    case StyleRole::TableHeader:
        style.background = ResolveColor(ColorToken::ListLabelBandBackground);
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
        style.background = ResolveColor(ColorToken::ListLabelBandBackground);
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
        style.iconSize = static_cast<float>(16u);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::SidebarItemActive:
        style.background = ResolveColor(ColorToken::SelectedBackground);
        style.foreground = ResolveColor(ColorToken::TextPrimary);
        style.icon = ResolveColor(ColorToken::AccentPrimary);
        style.border = ResolveColor(ColorToken::AccentPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::FormRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        style.iconSize = static_cast<float>(16u);
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
        style.foreground = ResolveColor(ColorToken::AccentPrimary);
        style.border = ResolveColor(ColorToken::BorderDefault);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.height = Scaled(theme.ResolveMetric(MetricToken::ControlHeightCompact));
        style.iconSize = Scaled(theme.ResolveMetric(MetricToken::CheckboxGlyphSize));
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
        style.iconSize = static_cast<float>(16u);
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

    style.borderWidth = Scaled(theme.ResolveMetric(MetricToken::BorderWidth));
    // Gap-cut chrome: structural borders come from layout spacing, not strokes.
    style.border = Color::Transparent();
    style.borderWidth = 0.0f;
    return style;
}

} // namespace we::runtime::kindui
