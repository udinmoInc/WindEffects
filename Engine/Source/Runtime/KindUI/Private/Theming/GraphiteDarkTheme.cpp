#include "KindUI/Theming/GraphiteDarkTheme.h"
#include "KindUI/Theming/Palette.h"
#include "KindUI/Theming/StyleResolve.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>
#include <array>
#include <unordered_map>

namespace we::runtime::kindui {
using P = palette::GraphiteDark;

Color GraphiteDarkTheme::ResolveColor(ColorToken token) const {
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
        return P::Secondary;
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
        return P::Header;
    case ColorToken::ToolbarBackground:
        return P::Panel;
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
        return P::Disabled;
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

float GraphiteDarkTheme::ResolveMetric(MetricToken token) const {
    switch (token) {
    case MetricToken::CornerRadiusSmall: return 3.0f;
    case MetricToken::CornerRadiusMedium: return 4.0f;
    case MetricToken::CornerRadiusLarge:
    case MetricToken::WindowCornerRadius: return 10.0f;
    case MetricToken::TextSizeMenu: return 12.0f;
    case MetricToken::TextSizeToolbar: return 12.0f;
    case MetricToken::TextSizeTabs: return 12.0f;
    case MetricToken::TextSizeNormal: return 13.0f;
    case MetricToken::TextSizeProperty: return 12.0f;
    case MetricToken::TextSizeCaption: return 12.0f;
    case MetricToken::TextSizeWindow: return 13.0f;
    case MetricToken::TextSizeHeader: return 16.5f;
    case MetricToken::TextSizeBody: return 14.0f;
    case MetricToken::TextSizeSmall: return 12.0f;
    case MetricToken::TextSizeCategory: return 12.0f;
    case MetricToken::TextSizeTitle: return 33.0f;
    case MetricToken::TextCharWidthRatio: return 0.55f;
    case MetricToken::BorderWidth: return 1.0f;
    case MetricToken::FocusRingWidth: return 1.0f;
    case MetricToken::PanelHeaderHeight:
    case MetricToken::PanelTabHeight: return 28.0f;
    case MetricToken::PanelToolbarHeight: return 28.0f;
    case MetricToken::HeaderControlHeight: return 24.0f;
    case MetricToken::IconButtonSize: return 24.0f;
    case MetricToken::ButtonHeight: return 24.0f;
    case MetricToken::ControlHeightCompact: return 24.0f;
    case MetricToken::ControlHeightLarge: return 40.0f;
    case MetricToken::FormRowHeight: return 28.0f;
    case MetricToken::MenuItemHeight: return 24.0f;
    case MetricToken::PageMargin: return 16.0f;
    case MetricToken::SectionGap: return 24.0f;
    case MetricToken::CardPadding: return 12.0f;
    case MetricToken::ContentGap: return 12.0f;
    case MetricToken::FormRowGap: return 13.0f;
    case MetricToken::LabelHintGap: return 5.0f;
    case MetricToken::ListRowHeight: return 28.0f;
    case MetricToken::CategoryHeaderHeight: return 26.0f;
    case MetricToken::TitleBarHeight: return 34.0f;
    case MetricToken::WindowControlWidth: return 40.0f;
    case MetricToken::ToolbarHeight: return 40.0f;
    case MetricToken::SearchBoxHeight: return 24.0f;
    case MetricToken::NavigationButtonSize: return 32.0f;
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

Margin GraphiteDarkTheme::ResolvePadding(PaddingToken token) const {
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
        return ResolveMetric(MetricToken::TextSizeHeader) - 1.0f;
    case TypographyToken::Heading4:
    case TypographyToken::Title:
        return ResolveMetric(MetricToken::TextSizeBody) + 1.0f;
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
        return ResolveMetric(MetricToken::TextSizeBody);
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
    if (pressAnim > 0.01f) {
        auto c = ResolveColor(ColorToken::PressedBackground);
        c.a *= pressAnim;
        return c;
    }
    if (hoverAnim > 0.01f) {
        return Color::Transparent().Lerp(ResolveColor(ColorToken::HoverBackground), hoverAnim);
    }
    if (selected) {
        return ResolveColor(ColorToken::SelectedBackground);
    }
    return Color::Transparent();
}

Color GraphiteDarkTheme::IconForState(bool hovered, bool active) const {
    if (active) {
        return ResolveColor(ColorToken::IconAccent);
    }
    Color base = ResolveColor(ColorToken::IconSecondary);
    if (hovered) {
        return Color::Lerp(base, ResolveColor(ColorToken::IconPrimary), 1.0f);
    }
    return base;
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
        style.background = theme.ResolveColor(ColorToken::WindowBackground);
        break;
    case StyleRole::Workspace:
        style.background = theme.ResolveColor(ColorToken::WorkspaceBackground);
        break;
    case StyleRole::Toolbar:
        style.background = theme.ResolveColor(ColorToken::ToolbarBackground);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ToolbarHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeToolbar));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
        break;
    case StyleRole::Panel:
        style.background = theme.ResolveColor(ColorToken::PanelBackground);
        style.border = theme.ResolveColor(ColorToken::BorderSubtle);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.padding = theme.ResolvePadding(PaddingToken::PaddingPanelLeft);
        for (auto& v : {&style.padding.left, &style.padding.top, &style.padding.right, &style.padding.bottom}) {
            *v = Scaled(*v);
        }
        break;
    case StyleRole::PanelHeader:
        style.background = theme.ResolveColor(ColorToken::HeaderBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::PanelHeaderHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        break;
    case StyleRole::Tab:
    case StyleRole::DockTab:
        style.background = theme.ResolveColor(ColorToken::TabBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        break;
    case StyleRole::TabActive:
    case StyleRole::DockTabActive:
        style.background = theme.ResolveColor(ColorToken::HeaderBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderLight);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        break;
    case StyleRole::Button:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonHover:
        style.background = theme.ResolveColor(ColorToken::HoverBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderLight);
        break;
    case StyleRole::ButtonActive:
        style.background = theme.ResolveColor(ColorToken::PressedBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        break;
    case StyleRole::ButtonPrimary:
        style.background = theme.ResolveColor(ColorToken::ButtonPrimaryBackground);
        style.foreground = Color{ 1.0f, 1.0f, 1.0f, 1.0f };
        style.border = Color::Transparent();
        style.borderWidth = 0.0f;
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonSecondary:
        style.background = theme.ResolveColor(ColorToken::HoverBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
        style.borderWidth = 1.0f;
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::IconButton:
        style.background = Color::Transparent();
        style.icon = theme.ResolveColor(ColorToken::IconSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::IconButtonSize));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::IconButtonRadius));
        break;
    case StyleRole::IconButtonHover:
        style.background = theme.ResolveColor(ColorToken::HoverBackground);
        style.icon = theme.ResolveColor(ColorToken::IconHover);
        style.border = theme.ResolveColor(ColorToken::BorderLight);
        break;
    case StyleRole::IconButtonPressed:
        style.background = theme.ResolveColor(ColorToken::PressedBackground);
        style.icon = theme.ResolveColor(ColorToken::IconActive);
        break;
    case StyleRole::NavigationButton:
        style.background = Color::Transparent();
        style.icon = theme.ResolveColor(ColorToken::IconSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::NavigationButtonSize));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeNavigation));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::IconButtonRadius));
        break;
    case StyleRole::Input:
    case StyleRole::SearchBox:
        style.background = theme.ResolveColor(ColorToken::InputBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::SearchBoxHeight));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::StatusBar:
        style.background = theme.ResolveColor(ColorToken::StatusBarBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::Space6) + theme.ResolveMetric(MetricToken::Space2));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        break;
    case StyleRole::MenuBar:
        style.background = theme.ResolveColor(ColorToken::WindowBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeMenu));
        break;
    case StyleRole::MenuItem:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeMenu));
        break;
    case StyleRole::Popup:
        style.background = theme.ResolveColor(ColorToken::PopupBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderSubtle);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.elevation = theme.ResolveElevation(ElevationToken::Popup);
        break;
    case StyleRole::Tooltip:
        style.background = theme.ResolveColor(ColorToken::TooltipBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeCaption));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.elevation = theme.ResolveElevation(ElevationToken::Popup);
        break;
    case StyleRole::Modal:
        style.background = theme.ResolveColor(ColorToken::PopupBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderSubtle);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::WindowCornerRadius));
        style.elevation = theme.ResolveElevation(ElevationToken::Overlay);
        break;
    case StyleRole::Gizmo:
        style.background = theme.ResolveColor(ColorToken::GizmoBackground);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ContentBrowser:
        style.background = theme.ResolveColor(ColorToken::PanelBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        break;
    case StyleRole::Splitter:
        style.background = theme.ResolveColor(ColorToken::BorderDefault);
        break;
    case StyleRole::Separator:
        style.background = theme.ResolveColor(ColorToken::Separator);
        break;
    case StyleRole::TextPrimary:
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        break;
    case StyleRole::TextSecondary:
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        break;
    case StyleRole::TextCaption:
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        break;
    case StyleRole::TextHint:
        style.foreground = theme.ResolveColor(ColorToken::TextHint);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeCaption));
        break;
    case StyleRole::TextDisabled:
        style.foreground = theme.ResolveColor(ColorToken::TextDisabled);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        break;
    case StyleRole::ButtonGhost:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonDanger:
        style.background = theme.ResolveColor(ColorToken::ButtonDangerBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::ButtonDangerHover);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ButtonHeight));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ToolbarButton:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.border = Color::Transparent();
        style.height = Scaled(theme.ResolveMetric(MetricToken::HeaderControlHeight));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeToolbar));
        break;
    case StyleRole::Card:
        style.background = theme.ResolveColor(ColorToken::CardBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderSubtle);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        {
            const float pad = Scaled(theme.ResolveMetric(MetricToken::CardPadding));
            style.padding = { pad, pad, pad, pad };
        }
        style.elevation = theme.ResolveElevation(ElevationToken::Card);
        break;
    case StyleRole::CardHover:
        style.background = theme.ResolveColor(ColorToken::HoverBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderLight);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.elevation = theme.ResolveElevation(ElevationToken::Card);
        break;
    case StyleRole::TableHeader:
        style.background = theme.ResolveColor(ColorToken::ToolbarBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::MenuItemHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeCaption));
        break;
    case StyleRole::TableRow:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        style.cornerRadius = 0.0f;
        break;
    case StyleRole::TableRowHover:
        style.background = theme.ResolveColor(ColorToken::HoverBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.cornerRadius = 0.0f;
        break;
    case StyleRole::TableRowSelected:
        style.background = theme.ResolveColor(ColorToken::SelectedBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = Color::Transparent();
        style.borderWidth = 0.0f;
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.cornerRadius = 0.0f;
        break;
    case StyleRole::SectionHeader:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeHeader));
        style.bold = true;
        style.height = Scaled(theme.ResolveMetric(MetricToken::CategoryHeaderHeight));
        break;
    case StyleRole::PropertyRow:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::FormRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeProperty));
        break;
    case StyleRole::SidebarItem:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.icon = theme.ResolveColor(ColorToken::IconSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::FormRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeNavigation));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::SidebarItemActive:
        style.background = theme.ResolveColor(ColorToken::SelectedBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.icon = theme.ResolveColor(ColorToken::AccentPrimary);
        style.border = theme.ResolveColor(ColorToken::AccentPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::FormRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeNavigation));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.bold = true;
        break;
    case StyleRole::WindowHeader:
        style.background = theme.ResolveColor(ColorToken::HeaderBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::TitleBarHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeWindow));
        break;
    case StyleRole::Checkbox:
    case StyleRole::ToggleSwitch:
        style.background = theme.ResolveColor(ColorToken::InputBackground);
        style.foreground = theme.ResolveColor(ColorToken::AccentPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.height = Scaled(theme.ResolveMetric(MetricToken::ControlHeightCompact));
        style.iconSize = Scaled(theme.ResolveMetric(MetricToken::CheckboxGlyphSize));
        break;
    case StyleRole::Scrollbar:
        style.background = theme.ResolveColor(ColorToken::ScrollbarTrack);
        style.foreground = theme.ResolveColor(ColorToken::ScrollbarThumb);
        style.border = theme.ResolveColor(ColorToken::ScrollbarThumbHover);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ScrollbarWidth));
        break;
    case StyleRole::TreeItem:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeTree));
        break;
    case StyleRole::TreeItemSelected:
        style.background = theme.ResolveColor(ColorToken::SelectedBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        break;
    default:
        break;
    }

    style.borderWidth = Scaled(theme.ResolveMetric(MetricToken::BorderWidth));
    return style;
}

} // namespace we::runtime::kindui
