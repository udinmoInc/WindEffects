#include "Core/Theme.h"

#include "WindEffects/Editor/UI/Theming/GraphiteDarkTheme.h"

namespace WindEffects::Editor::UI {
namespace {

Color MakeColor(float r, float g, float b, float a = 1.0f) {
    return {r, g, b, a};
}

Theme BuildThemeSnapshot() {
    static GraphiteDarkTheme provider;
    Theme theme;

    auto color = [&](ThemeToken token) { return provider.GetColor(token); };
    auto metric = [&](ThemeToken token) { return provider.GetMetric(token); };

    theme.WindowBackground = color(ThemeToken::WindowBackground);
    theme.WorkspaceBackground = color(ThemeToken::WorkspaceBackground);
    theme.ToolbarBackground = color(ThemeToken::ToolbarBackground);
    theme.PanelBackground = color(ThemeToken::PanelBackground);
    theme.HeaderBackground = color(ThemeToken::HeaderBackground);
    theme.ViewportBackground = color(ThemeToken::ViewportBackground);
    theme.FooterBackground = color(ThemeToken::StatusBarBackground);
    theme.MenuBarBackground = color(ThemeToken::MenuBarBackground);
    theme.TabBackground = color(ThemeToken::TabBackground);
    theme.PopupBackground = color(ThemeToken::PopupBackground);
    theme.ContentBrowserBackground = color(ThemeToken::ContentBrowserBackground);

    theme.BorderLight = color(ThemeToken::BorderLight);
    theme.BorderDark = color(ThemeToken::BorderDark);
    theme.BorderDefault = color(ThemeToken::BorderDefault);
    theme.BorderSecondary = color(ThemeToken::BorderDark);
    theme.BorderFocus = color(ThemeToken::BorderFocus);
    theme.Separator = color(ThemeToken::Separator);

    theme.HoverBackground = color(ThemeToken::HoverBackground);
    theme.HoverBg = color(ThemeToken::HoverBackground);
    theme.HoverOverlay = color(ThemeToken::HoverBackground);
    theme.HoverMenu = color(ThemeToken::HoverBackground);
    theme.HoverButton = color(ThemeToken::HoverBackground);
    theme.ActiveBackground = color(ThemeToken::ActiveBackground);
    theme.SelectedBackground = color(ThemeToken::SelectedBackground);
    theme.SelectedBg = color(ThemeToken::SelectedBackground);
    theme.DisabledBackground = color(ThemeToken::DisabledBackground);
    theme.PressedBackground = color(ThemeToken::PressedBackground);
    theme.PressedOverlay = color(ThemeToken::PressedBackground);

    theme.TextPrimary = color(ThemeToken::TextPrimary);
    theme.TextSecondary = color(ThemeToken::TextSecondary);
    theme.TextDisabled = color(ThemeToken::TextDisabled);
    theme.TextMuted = color(ThemeToken::TextMuted);
    theme.TextWindowLabel = color(ThemeToken::TextWindowLabel);

    theme.IconDefault = color(ThemeToken::IconDefault);
    theme.IconHover = color(ThemeToken::IconHover);
    theme.IconActive = color(ThemeToken::IconActive);
    theme.IconDisabled = color(ThemeToken::IconDisabled);
    theme.IconMuted = color(ThemeToken::IconDefault);
    theme.SearchIcon = color(ThemeToken::IconDefault);

    theme.AccentPrimary = color(ThemeToken::AccentPrimary);
    theme.AccentHover = color(ThemeToken::AccentHover);
    theme.ButtonPrimaryBackground = color(ThemeToken::ToolbarBackground);
    theme.ButtonPrimaryHover = color(ThemeToken::HoverBackground);
    theme.ButtonPrimaryPressed = color(ThemeToken::PressedBackground);
    theme.ActiveTabLine = color(ThemeToken::ActiveTabLine);
    theme.SelectionHighlight = color(ThemeToken::SelectionHighlight);
    theme.SelectedAccent = color(ThemeToken::AccentPrimary);
    theme.Success = color(ThemeToken::Success);
    theme.Warning = color(ThemeToken::Warning);

    theme.InputBackground = color(ThemeToken::InputBackground);
    theme.SearchBoxBackground = color(ThemeToken::SearchBoxBackground);
    theme.SearchBoxBg = color(ThemeToken::SearchBoxBackground);
    theme.SearchPlaceholder = color(ThemeToken::SearchPlaceholder);

    theme.ViewportToolbarBackground = color(ThemeToken::ViewportToolbarBackground);
    theme.StatusBarBackground = color(ThemeToken::StatusBarBackground);

    theme.ContentBrowserHoverBg = color(ThemeToken::HoverBackground);
    theme.ContentBrowserItemLabel = color(ThemeToken::TextPrimary);
    theme.ContentBrowserFolderShadow = MakeColor(0.0f, 0.0f, 0.0f, 0.35f);
    theme.ContentBrowserFolderEdge = color(ThemeToken::BorderDefault);
    theme.ContentBrowserFolderHighlight = MakeColor(0.45f, 0.48f, 0.52f);
    theme.ContentBrowserFolderTab = MakeColor(0.32f, 0.35f, 0.39f);
    theme.ContentBrowserFolderPrimary = MakeColor(0.28f, 0.31f, 0.35f);
    theme.ContentBrowserFolderBody = MakeColor(0.24f, 0.27f, 0.31f);
    theme.SidebarIconDefault = color(ThemeToken::IconDefault);
    theme.SidebarIconHover = color(ThemeToken::IconHover);
    theme.TreeArrow = color(ThemeToken::TextSecondary);

    theme.GizmoBackground = MakeColor(0.12f, 0.12f, 0.12f, 0.85f);
    theme.GizmoAxisX = MakeColor(0.90f, 0.25f, 0.25f, 1.0f);
    theme.GizmoAxisY = MakeColor(0.30f, 0.85f, 0.35f, 1.0f);
    theme.GizmoAxisZ = MakeColor(0.30f, 0.50f, 0.95f, 1.0f);

    theme.HighlightSubtle = MakeColor(1.0f, 1.0f, 1.0f, 0.06f);
    theme.ShadowSubtle = MakeColor(0.0f, 0.0f, 0.0f, 0.25f);

    theme.CornerRadiusSmall = metric(ThemeToken::CornerRadiusSmall);
    theme.CornerRadiusMedium = metric(ThemeToken::CornerRadiusMedium);
    theme.CornerRadiusLarge = metric(ThemeToken::CornerRadiusLarge);
    theme.WindowCornerRadius = metric(ThemeToken::WindowCornerRadius);
    theme.TextSizeMenu = metric(ThemeToken::TextSizeMenu);
    theme.TextSizeToolbar = metric(ThemeToken::TextSizeToolbar);
    theme.TextSizeTabs = metric(ThemeToken::TextSizeTabs);
    theme.TextSizeNormal = metric(ThemeToken::TextSizeNormal);
    theme.TextSizeBody = metric(ThemeToken::TextSizeNormal);
    theme.TextSizeProperty = metric(ThemeToken::TextSizeProperty);
    theme.TextSizeHeader = metric(ThemeToken::TextSizeProperty);
    theme.TextSizeCaption = metric(ThemeToken::TextSizeCaption);
    theme.TextSizeSmall = metric(ThemeToken::TextSizeCaption);
    theme.TextSizeWindow = metric(ThemeToken::TextSizeWindow);
    theme.BorderWidth = metric(ThemeToken::BorderWidth);
    theme.PanelHeaderHeight = metric(ThemeToken::PanelHeaderHeight);
    theme.ToolbarHeight = metric(ThemeToken::ToolbarHeight);
    theme.SearchBoxHeight = metric(ThemeToken::SearchBoxHeight);
    theme.IconButtonSize = metric(ThemeToken::IconButtonSize);
    theme.ButtonHeight = metric(ThemeToken::ButtonHeight);
    theme.IconSizeSearch = metric(ThemeToken::IconSizeSearch);
    theme.IconSizeToolbar = metric(ThemeToken::IconSizeToolbar);
    theme.IconSizeTree = metric(ThemeToken::IconSizeTree);
    theme.Space1 = metric(ThemeToken::Space1);
    theme.Space2 = metric(ThemeToken::Space2);
    theme.Space3 = metric(ThemeToken::Space3);
    theme.Space4 = metric(ThemeToken::Space4);
    theme.Space5 = metric(ThemeToken::Space5);
    theme.Space6 = metric(ThemeToken::Space6);

    return theme;
}

} // namespace

Color Theme::TextForState(bool hovered, bool active) const {
    if (active) return TextPrimary;
    if (hovered) return TextPrimary;
    return TextSecondary;
}

Color Theme::IconForState(bool hovered, bool active) const {
    if (active) return IconActive;
    if (hovered) return IconHover;
    return IconDefault;
}

Color Theme::InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    if (pressAnim > 0.01f) {
        Color c = PressedBackground;
        c.a *= pressAnim;
        return c;
    }
    if (hoverAnim > 0.01f) {
        return Color::Transparent().Lerp(HoverBackground, hoverAnim);
    }
    if (selected) {
        return SelectedBackground;
    }
    return Color::Transparent();
}

const Theme& Theme::Get() {
    static Theme snapshot = BuildThemeSnapshot();
    return snapshot;
}

} // namespace WindEffects::Editor::UI
