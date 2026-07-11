#include "WindEffects/Editor/UI/Theming/GraphiteDarkTheme.h"

#include <algorithm>
#include <array>
#include <unordered_map>

namespace WindEffects::Editor::UI {
namespace {

Color MakeColor(float r, float g, float b, float a = 1.0f) {
    return {r, g, b, a};
}

} // namespace

Color GraphiteDarkTheme::GetColor(ThemeToken token) const {
    switch (token) {
    case ThemeToken::WindowBackground: return MakeColor(0.090f, 0.090f, 0.090f);
    case ThemeToken::WorkspaceBackground: return MakeColor(0.114f, 0.114f, 0.114f);
    case ThemeToken::ToolbarBackground: return MakeColor(0.145f, 0.145f, 0.145f);
    case ThemeToken::PanelBackground: return MakeColor(0.137f, 0.137f, 0.137f);
    case ThemeToken::HeaderBackground: return MakeColor(0.125f, 0.125f, 0.125f);
    case ThemeToken::ViewportBackground: return MakeColor(0.090f, 0.090f, 0.090f);
    case ThemeToken::FooterBackground: return MakeColor(0.137f, 0.137f, 0.137f);
    case ThemeToken::MenuBarBackground: return MakeColor(0.125f, 0.125f, 0.125f);
    case ThemeToken::TabBackground: return MakeColor(0.114f, 0.114f, 0.114f);
    case ThemeToken::PopupBackground: return MakeColor(0.137f, 0.137f, 0.137f);
    case ThemeToken::ContentBrowserBackground: return MakeColor(0.137f, 0.137f, 0.137f);

    case ThemeToken::BorderLight: return MakeColor(0.227f, 0.227f, 0.227f);
    case ThemeToken::BorderDark: return MakeColor(0.145f, 0.145f, 0.145f);
    case ThemeToken::BorderDefault: return MakeColor(0.188f, 0.188f, 0.188f);
    case ThemeToken::BorderFocus: return MakeColor(0.839f, 0.635f, 0.290f);
    case ThemeToken::Separator: return MakeColor(0.231f, 0.231f, 0.231f, 0.50f);

    case ThemeToken::HoverBackground: return MakeColor(0.176f, 0.176f, 0.176f);
    case ThemeToken::ActiveBackground: return MakeColor(0.290f, 0.290f, 0.290f);
    case ThemeToken::SelectedBackground: return MakeColor(0.227f, 0.227f, 0.227f);
    case ThemeToken::DisabledBackground: return MakeColor(0.125f, 0.125f, 0.125f);
    case ThemeToken::PressedBackground: return MakeColor(0.290f, 0.290f, 0.290f);

    case ThemeToken::TextPrimary: return MakeColor(1.0f, 1.0f, 1.0f);
    case ThemeToken::TextSecondary: return MakeColor(0.722f, 0.745f, 0.780f);
    case ThemeToken::TextDisabled: return MakeColor(0.478f, 0.478f, 0.478f);
    case ThemeToken::TextMuted: return MakeColor(0.550f, 0.550f, 0.550f);
    case ThemeToken::TextWindowLabel: return MakeColor(0.835f, 0.835f, 0.835f);

    case ThemeToken::IconDefault: return MakeColor(0.491f, 0.509f, 0.533f);
    case ThemeToken::IconHover: return MakeColor(1.0f, 1.0f, 1.0f);
    case ThemeToken::IconActive: return MakeColor(1.0f, 1.0f, 1.0f);
    case ThemeToken::IconDisabled: return MakeColor(0.416f, 0.439f, 0.471f);

    case ThemeToken::AccentPrimary: return MakeColor(0.839f, 0.635f, 0.290f);
    case ThemeToken::AccentHover: return MakeColor(0.900f, 0.700f, 0.350f);
    case ThemeToken::ActiveTabLine: return MakeColor(0.839f, 0.635f, 0.290f);
    case ThemeToken::SelectionHighlight: return MakeColor(0.165f, 0.361f, 0.600f);
    case ThemeToken::Success: return MakeColor(0.180f, 0.800f, 0.443f);
    case ThemeToken::Warning: return MakeColor(0.945f, 0.768f, 0.058f);

    case ThemeToken::InputBackground: return MakeColor(0.102f, 0.102f, 0.102f);
    case ThemeToken::SearchBoxBackground: return MakeColor(0.102f, 0.102f, 0.102f);
    case ThemeToken::SearchPlaceholder: return MakeColor(0.478f, 0.478f, 0.478f);

    case ThemeToken::ViewportToolbarBackground: return MakeColor(0.100f, 0.100f, 0.100f, 0.85f);
    case ThemeToken::StatusBarBackground: return MakeColor(0.125f, 0.125f, 0.125f);
    default: return Color::Transparent();
    }
}

float GraphiteDarkTheme::GetMetric(ThemeToken token) const {
    switch (token) {
    case ThemeToken::CornerRadiusSmall: return 4.0f;
    case ThemeToken::CornerRadiusMedium: return 6.0f;
    case ThemeToken::CornerRadiusLarge: return 6.0f;
    case ThemeToken::WindowCornerRadius: return 10.0f;
    case ThemeToken::TextSizeMenu: return 13.0f;
    case ThemeToken::TextSizeToolbar: return 13.0f;
    case ThemeToken::TextSizeTabs: return 13.0f;
    case ThemeToken::TextSizeNormal: return 13.0f;
    case ThemeToken::TextSizeProperty: return 13.0f;
    case ThemeToken::TextSizeCaption: return 12.0f;
    case ThemeToken::TextSizeWindow: return 13.0f;
    case ThemeToken::BorderWidth: return 1.0f;
    case ThemeToken::PanelHeaderHeight: return 32.0f;
    case ThemeToken::ToolbarHeight: return 32.0f;
    case ThemeToken::SearchBoxHeight: return 24.0f;
    case ThemeToken::IconButtonSize: return 24.0f;
    case ThemeToken::ButtonHeight: return 24.0f;
    case ThemeToken::IconSizeSearch: return 14.0f;
    case ThemeToken::IconSizeToolbar: return 16.0f;
    case ThemeToken::IconSizeTree: return 14.0f;
    case ThemeToken::Space1: return 4.0f;
    case ThemeToken::Space2: return 8.0f;
    case ThemeToken::Space3: return 12.0f;
    case ThemeToken::Space4: return 16.0f;
    case ThemeToken::Space5: return 20.0f;
    case ThemeToken::Space6: return 24.0f;
    default: return 0.0f;
    }
}

Margin GraphiteDarkTheme::GetPadding(ThemeToken token) const {
    switch (token) {
    case ThemeToken::PaddingPanelLeft:
    case ThemeToken::PaddingPanelTop:
    case ThemeToken::PaddingPanelRight:
    case ThemeToken::PaddingPanelBottom:
        return {8.0f, 8.0f, 8.0f, 8.0f};
    case ThemeToken::PaddingButtonLeft:
    case ThemeToken::PaddingButtonTop:
    case ThemeToken::PaddingButtonRight:
    case ThemeToken::PaddingButtonBottom:
        return {12.0f, 8.0f, 12.0f, 8.0f};
    default:
        return {};
    }
}

Color GraphiteDarkTheme::InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    if (pressAnim > 0.01f) {
        auto c = GetColor(ThemeToken::PressedBackground);
        c.a *= pressAnim;
        return c;
    }
    if (hoverAnim > 0.01f) {
        return Color::Transparent().Lerp(GetColor(ThemeToken::HoverBackground), hoverAnim);
    }
    if (selected) {
        return GetColor(ThemeToken::SelectedBackground);
    }
    return Color::Transparent();
}

Color GraphiteDarkTheme::IconForState(bool hovered, bool active) const {
    if (active) return GetColor(ThemeToken::IconActive);
    return hovered ? GetColor(ThemeToken::IconHover) : GetColor(ThemeToken::IconDefault);
}

Color GraphiteDarkTheme::TextForState(bool hovered, bool active) const {
    return (active || hovered) ? GetColor(ThemeToken::TextPrimary) : GetColor(ThemeToken::TextSecondary);
}

StyleResolver::StyleResolver(std::shared_ptr<IThemeProvider> theme)
    : m_Theme(std::move(theme)) {}

void StyleResolver::SetDpiScale(float scale) {
    m_DpiScale = std::clamp(scale, 1.0f, 3.0f);
}

float StyleResolver::Scaled(float logicalValue) const {
    return logicalValue * m_DpiScale;
}

ResolvedStyle StyleResolver::Resolve(StyleRole role) const {
    ResolvedStyle style{};
    const auto& theme = *m_Theme;

    switch (role) {
    case StyleRole::Window:
        style.background = theme.GetColor(ThemeToken::WindowBackground);
        break;
    case StyleRole::Workspace:
        style.background = theme.GetColor(ThemeToken::WorkspaceBackground);
        break;
    case StyleRole::Toolbar:
        style.background = theme.GetColor(ThemeToken::ToolbarBackground);
        style.height = Scaled(theme.GetMetric(ThemeToken::ToolbarHeight));
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeToolbar));
        style.iconSize = Scaled(theme.GetMetric(ThemeToken::IconSizeToolbar));
        break;
    case StyleRole::Panel:
        style.background = theme.GetColor(ThemeToken::PanelBackground);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusMedium));
        style.padding = theme.GetPadding(ThemeToken::PaddingPanelLeft);
        for (auto& v : {&style.padding.left, &style.padding.top, &style.padding.right, &style.padding.bottom}) {
            *v = Scaled(*v);
        }
        break;
    case StyleRole::PanelHeader:
        style.background = theme.GetColor(ThemeToken::HeaderBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.height = Scaled(theme.GetMetric(ThemeToken::PanelHeaderHeight));
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeTabs));
        break;
    case StyleRole::Tab:
        style.background = theme.GetColor(ThemeToken::TabBackground);
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeTabs));
        break;
    case StyleRole::TabActive:
    case StyleRole::DockTabActive:
        style.background = theme.GetColor(ThemeToken::TabBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.border = theme.GetColor(ThemeToken::ActiveTabLine);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeTabs));
        break;
    case StyleRole::DockTab:
        style.background = theme.GetColor(ThemeToken::TabBackground);
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeTabs));
        break;
    case StyleRole::Button:
        style.background = Color::Transparent();
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.height = Scaled(theme.GetMetric(ThemeToken::ButtonHeight));
        style.iconSize = Scaled(theme.GetMetric(ThemeToken::IconSizeToolbar));
        break;
    case StyleRole::IconButton:
        style.background = Color::Transparent();
        style.icon = theme.GetColor(ThemeToken::IconDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::IconButtonSize));
        style.iconSize = Scaled(theme.GetMetric(ThemeToken::IconSizeToolbar));
        break;
    case StyleRole::Input:
    case StyleRole::SearchBox:
        style.background = theme.GetColor(ThemeToken::InputBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.height = Scaled(theme.GetMetric(ThemeToken::SearchBoxHeight));
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusSmall));
        break;
    case StyleRole::StatusBar:
        style.background = theme.GetColor(ThemeToken::StatusBarBackground);
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.height = Scaled(theme.GetMetric(ThemeToken::TextSizeCaption));
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeCaption));
        break;
    case StyleRole::MenuBar:
        style.background = theme.GetColor(ThemeToken::MenuBarBackground);
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeMenu));
        break;
    case StyleRole::Splitter:
        style.background = theme.GetColor(ThemeToken::BorderDefault);
        break;
    case StyleRole::Separator:
        style.background = theme.GetColor(ThemeToken::Separator);
        break;
    case StyleRole::TextPrimary:
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeNormal));
        break;
    case StyleRole::TextSecondary:
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeNormal));
        break;
    case StyleRole::TextCaption:
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeCaption));
        break;
    default:
        break;
    }

    style.borderWidth = Scaled(theme.GetMetric(ThemeToken::BorderWidth));
    return style;
}

} // namespace WindEffects::Editor::UI
