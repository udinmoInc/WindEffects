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
    case ThemeToken::WindowBackground: return MakeColor(0.059f, 0.067f, 0.078f);       // #0F1114
    case ThemeToken::WorkspaceBackground: return MakeColor(0.059f, 0.067f, 0.078f);    // #0F1114
    case ThemeToken::ToolbarBackground: return MakeColor(0.090f, 0.102f, 0.122f);       // #171A1F
    case ThemeToken::PanelBackground: return MakeColor(0.090f, 0.102f, 0.122f);        // #171A1F
    case ThemeToken::HeaderBackground: return MakeColor(0.090f, 0.102f, 0.122f);       // #171A1F
    case ThemeToken::ViewportBackground: return MakeColor(0.059f, 0.067f, 0.078f);       // #0F1114
    case ThemeToken::FooterBackground: return MakeColor(0.090f, 0.102f, 0.122f);       // #171A1F
    case ThemeToken::MenuBarBackground: return MakeColor(0.090f, 0.102f, 0.122f);      // #171A1F
    case ThemeToken::TabBackground: return MakeColor(0.090f, 0.102f, 0.122f);          // #171A1F
    case ThemeToken::PopupBackground: return MakeColor(0.118f, 0.133f, 0.157f);        // #1E2228
    case ThemeToken::ContentBrowserBackground: return MakeColor(0.090f, 0.102f, 0.122f); // #171A1F

    case ThemeToken::BorderLight: return MakeColor(0.200f, 0.220f, 0.250f);
    case ThemeToken::BorderDark: return MakeColor(0.145f, 0.165f, 0.192f);             // #252A31
    case ThemeToken::BorderDefault: return MakeColor(0.169f, 0.188f, 0.220f);          // #2B3038
    case ThemeToken::BorderFocus: return MakeColor(0.267f, 0.573f, 0.961f);            // #4492F5
    case ThemeToken::Separator: return MakeColor(0.169f, 0.188f, 0.220f, 0.50f);      // #2B3038

    case ThemeToken::HoverBackground: return MakeColor(0.118f, 0.133f, 0.157f);          // #1E2228
    case ThemeToken::ActiveBackground: return MakeColor(0.145f, 0.165f, 0.192f);         // #252A31
    case ThemeToken::SelectedBackground: return MakeColor(0.145f, 0.165f, 0.192f);     // #252A31
    case ThemeToken::DisabledBackground: return MakeColor(0.090f, 0.102f, 0.122f);
    case ThemeToken::PressedBackground: return MakeColor(0.145f, 0.165f, 0.192f);        // #252A31

    case ThemeToken::TextPrimary: return MakeColor(0.902f, 0.906f, 0.918f);             // #E6E7EA
    case ThemeToken::TextSecondary: return MakeColor(0.627f, 0.651f, 0.678f);          // #A0A6AD
    case ThemeToken::TextDisabled: return MakeColor(0.400f, 0.420f, 0.450f);
    case ThemeToken::TextMuted: return MakeColor(0.500f, 0.520f, 0.550f);
    case ThemeToken::TextWindowLabel: return MakeColor(0.902f, 0.906f, 0.918f);

    case ThemeToken::IconDefault: return MakeColor(0.627f, 0.651f, 0.678f);            // #A0A6AD
    case ThemeToken::IconHover: return MakeColor(0.902f, 0.906f, 0.918f);
    case ThemeToken::IconActive: return MakeColor(0.902f, 0.906f, 0.918f);
    case ThemeToken::IconDisabled: return MakeColor(0.400f, 0.420f, 0.450f);

    case ThemeToken::AccentPrimary: return MakeColor(0.267f, 0.573f, 0.961f);          // #4492F5
    case ThemeToken::AccentHover: return MakeColor(0.350f, 0.630f, 0.980f);
    case ThemeToken::ActiveTabLine: return MakeColor(0.267f, 0.573f, 0.961f);          // #4492F5
    case ThemeToken::SelectionHighlight: return MakeColor(0.267f, 0.573f, 0.961f, 0.25f);
    case ThemeToken::Success: return MakeColor(0.298f, 0.686f, 0.314f);              // #4CAF50
    case ThemeToken::Warning: return MakeColor(0.925f, 0.659f, 0.0f);                  // #ECA800

    case ThemeToken::InputBackground: return MakeColor(0.059f, 0.067f, 0.078f);        // #0F1114
    case ThemeToken::SearchBoxBackground: return MakeColor(0.059f, 0.067f, 0.078f);
    case ThemeToken::SearchPlaceholder: return MakeColor(0.500f, 0.520f, 0.550f);

    case ThemeToken::ViewportToolbarBackground: return MakeColor(0.090f, 0.102f, 0.122f, 0.92f);
    case ThemeToken::StatusBarBackground: return MakeColor(0.090f, 0.102f, 0.122f);

    case ThemeToken::ButtonPrimaryBackground: return MakeColor(0.118f, 0.133f, 0.157f);
    case ThemeToken::ButtonPrimaryHover: return MakeColor(0.145f, 0.165f, 0.192f);
    case ThemeToken::ButtonPrimaryPressed: return MakeColor(0.145f, 0.165f, 0.192f);

    case ThemeToken::GizmoBackground: return MakeColor(0.12f, 0.12f, 0.12f, 0.85f);
    case ThemeToken::GizmoAxisX: return MakeColor(0.90f, 0.25f, 0.25f, 1.0f);
    case ThemeToken::GizmoAxisY: return MakeColor(0.30f, 0.85f, 0.35f, 1.0f);
    case ThemeToken::GizmoAxisZ: return MakeColor(0.30f, 0.50f, 0.95f, 1.0f);

    case ThemeToken::HighlightSubtle: return MakeColor(1.0f, 1.0f, 1.0f, 0.06f);
    case ThemeToken::ShadowSubtle: return MakeColor(0.0f, 0.0f, 0.0f, 0.25f);
    case ThemeToken::ShadowOverlay: return MakeColor(0.0f, 0.0f, 0.0f, 0.40f);
    case ThemeToken::ModalScrim: return MakeColor(0.0f, 0.0f, 0.0f, 0.65f);

    case ThemeToken::ContentBrowserHoverBackground: return MakeColor(0.118f, 0.133f, 0.157f);
    case ThemeToken::ContentBrowserFolderShadow: return MakeColor(0.0f, 0.0f, 0.0f, 0.35f);
    case ThemeToken::ContentBrowserFolderEdge: return MakeColor(0.169f, 0.188f, 0.220f);
    case ThemeToken::ContentBrowserFolderHighlight: return MakeColor(0.200f, 0.220f, 0.250f);
    case ThemeToken::ContentBrowserFolderTab: return MakeColor(0.145f, 0.165f, 0.192f);
    case ThemeToken::ContentBrowserFolderPrimary: return MakeColor(0.118f, 0.133f, 0.157f);
    case ThemeToken::ContentBrowserFolderBody: return MakeColor(0.090f, 0.102f, 0.122f);

    case ThemeToken::CloseButtonHover: return MakeColor(0.878f, 0.365f, 0.365f);
    case ThemeToken::DragGhostBackground: return MakeColor(0.118f, 0.133f, 0.157f, 0.85f);
    case ThemeToken::TooltipBackground: return MakeColor(0.118f, 0.133f, 0.157f, 0.95f);

    case ThemeToken::ErrorForeground: return MakeColor(0.878f, 0.365f, 0.365f);        // #E05D5D
    case ThemeToken::LinkForeground: return MakeColor(0.4f, 0.7f, 1.0f);
    case ThemeToken::CodeForeground: return MakeColor(0.9f, 0.9f, 0.85f);
    default: return Color::Transparent();
    }
}

float GraphiteDarkTheme::GetMetric(ThemeToken token) const {
    switch (token) {
    case ThemeToken::CornerRadiusSmall: return 4.0f;
    case ThemeToken::CornerRadiusMedium: return 6.0f;
    case ThemeToken::CornerRadiusLarge: return 8.0f;
    case ThemeToken::WindowCornerRadius: return 8.0f;
    case ThemeToken::TextSizeMenu: return 13.0f;
    case ThemeToken::TextSizeToolbar: return 12.0f;
    case ThemeToken::TextSizeTabs: return 12.0f;
    case ThemeToken::TextSizeNormal: return 12.0f;
    case ThemeToken::TextSizeProperty: return 12.0f;
    case ThemeToken::TextSizeCaption: return 10.0f;
    case ThemeToken::TextSizeWindow: return 14.0f;
    case ThemeToken::TextSizeHeader: return 15.0f;
    case ThemeToken::TextSizeBody: return 12.0f;
    case ThemeToken::TextSizeSmall: return 11.0f;
    case ThemeToken::BorderWidth: return 1.0f;
    case ThemeToken::PanelHeaderHeight: return 32.0f;
    case ThemeToken::TitleBarHeight: return 40.0f;
    case ThemeToken::HeaderControlHeight: return 28.0f;
    case ThemeToken::WindowControlWidth: return 46.0f;
    case ThemeToken::ToolbarHeight: return 36.0f;
    case ThemeToken::SearchBoxHeight: return 26.0f;
    case ThemeToken::IconButtonSize: return 28.0f;
    case ThemeToken::ButtonHeight: return 26.0f;
    case ThemeToken::NavigationButtonSize: return 26.0f;
    case ThemeToken::IconSizeSearch: return 14.0f;
    case ThemeToken::IconSizeToolbar: return 20.0f;
    case ThemeToken::IconSizeTree: return 16.0f;
    case ThemeToken::IconSizeNavigation: return 18.0f;
    case ThemeToken::IconButtonRadius: return 4.0f;
    case ThemeToken::ButtonPaddingHorizontal: return 12.0f;
    case ThemeToken::ButtonSpacing: return 4.0f;
    case ThemeToken::ButtonGroupSpacing: return 12.0f;
    case ThemeToken::Space1: return 4.0f;
    case ThemeToken::Space2: return 8.0f;
    case ThemeToken::Space3: return 12.0f;
    case ThemeToken::Space4: return 16.0f;
    case ThemeToken::Space5: return 20.0f;
    case ThemeToken::Space6: return 24.0f;
    case ThemeToken::HoverAnimationDamping: return 12.0f;
    case ThemeToken::PressAnimationDamping: return 12.0f;
    case ThemeToken::PressOffset: return 1.0f;
    case ThemeToken::ShadowBlurSmall: return 4.0f;
    case ThemeToken::ShadowBlurMedium: return 8.0f;
    case ThemeToken::ShadowSpreadMedium: return 16.0f;
    default: return 0.0f;
    }
}

Margin GraphiteDarkTheme::GetPadding(ThemeToken token) const {
    switch (token) {
    case ThemeToken::PaddingPanelLeft:
    case ThemeToken::PaddingPanelTop:
    case ThemeToken::PaddingPanelRight:
    case ThemeToken::PaddingPanelBottom:
        return {12.0f, 12.0f, 12.0f, 12.0f};
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
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::ButtonHeight));
        style.iconSize = Scaled(theme.GetMetric(ThemeToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonHover:
        style.background = theme.GetColor(ThemeToken::HoverBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.border = theme.GetColor(ThemeToken::BorderLight);
        break;
    case StyleRole::ButtonActive:
        style.background = theme.GetColor(ThemeToken::PressedBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        break;
    case StyleRole::ButtonPrimary:
        style.background = theme.GetColor(ThemeToken::ButtonPrimaryBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::ButtonHeight));
        style.iconSize = Scaled(theme.GetMetric(ThemeToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonSecondary:
        style.background = Color::Transparent();
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::ButtonHeight));
        style.iconSize = Scaled(theme.GetMetric(ThemeToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusSmall));
        break;
    case StyleRole::IconButton:
        style.background = Color::Transparent();
        style.icon = theme.GetColor(ThemeToken::IconDefault);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::IconButtonSize));
        style.iconSize = Scaled(theme.GetMetric(ThemeToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::IconButtonRadius));
        break;
    case StyleRole::IconButtonHover:
        style.background = theme.GetColor(ThemeToken::HoverBackground);
        style.icon = theme.GetColor(ThemeToken::IconHover);
        style.border = theme.GetColor(ThemeToken::BorderLight);
        break;
    case StyleRole::IconButtonPressed:
        style.background = theme.GetColor(ThemeToken::PressedBackground);
        style.icon = theme.GetColor(ThemeToken::IconActive);
        break;
    case StyleRole::NavigationButton:
        style.background = Color::Transparent();
        style.icon = theme.GetColor(ThemeToken::IconDefault);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::NavigationButtonSize));
        style.iconSize = Scaled(theme.GetMetric(ThemeToken::IconSizeNavigation));
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::IconButtonRadius));
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
        style.height = Scaled(30.0f);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeSmall));
        break;
    case StyleRole::MenuBar:
        style.background = theme.GetColor(ThemeToken::MenuBarBackground);
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeMenu));
        break;
    case StyleRole::MenuItem:
        style.background = Color::Transparent();
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeMenu));
        break;
    case StyleRole::Popup:
        style.background = theme.GetColor(ThemeToken::PopupBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusMedium));
        break;
    case StyleRole::Tooltip:
        style.background = theme.GetColor(ThemeToken::TooltipBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeSmall));
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusSmall));
        break;
    case StyleRole::Modal:
        style.background = theme.GetColor(ThemeToken::PopupBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::WindowCornerRadius));
        break;
    case StyleRole::Gizmo:
        style.background = theme.GetColor(ThemeToken::GizmoBackground);
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusSmall));
        break;
    case StyleRole::ContentBrowser:
        style.background = theme.GetColor(ThemeToken::ContentBrowserBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeBody));
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
