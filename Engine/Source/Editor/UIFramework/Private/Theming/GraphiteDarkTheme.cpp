#include "WindEffects/Editor/UI/Theming/GraphiteDarkTheme.h"
#include "Rendering/IconMetrics.h"

#include <algorithm>
#include <array>
#include <unordered_map>

namespace WindEffects::Editor::UI {
namespace {

Color MakeColor(float r, float g, float b, float a = 1.0f) {
    return {r, g, b, a};
}

Color Hex(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return MakeColor(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

} // namespace

Color GraphiteDarkTheme::GetColor(ThemeToken token) const {
    switch (token) {
    // ── Material stack (darkest → lightest) ──────────────────────────────────
    case ThemeToken::WindowBackground:              return Hex(0x16,0x16,0x16);
    case ThemeToken::ViewportBackground:            return Hex(0x16,0x16,0x16);
    case ThemeToken::DockAreaBackground:            return Hex(0x16,0x16,0x16);
    case ThemeToken::WorkspaceBackground:           return Hex(0x16,0x16,0x16);
    case ThemeToken::DisabledBackground:            return Hex(0x16,0x16,0x16);

    case ThemeToken::SearchBoxBackground:           return Hex(0x1A,0x1A,0x1A);
    case ThemeToken::InputBackground:               return Hex(0x1A,0x1A,0x1A);
    case ThemeToken::ScrollbarTrack:                return Hex(0x16,0x16,0x16);

    case ThemeToken::PanelBackground:               return Hex(0x1D,0x1D,0x1D);
    case ThemeToken::PanelContentBackground:        return Hex(0x1D,0x1D,0x1D);
    case ThemeToken::ContentBrowserBackground:      return Hex(0x1D,0x1D,0x1D);
    case ThemeToken::ContentBrowserFolderBody:      return Hex(0xB8,0x95,0x48);

    // Floating surfaces — elevation via luminance + shadow tokens
    case ThemeToken::PopupBackground:               return Hex(0x3A,0x3A,0x3A);
    case ThemeToken::TooltipBackground:             return Hex(0x3A,0x3A,0x3A,0.96f);
    case ThemeToken::DragGhostBackground:           return Hex(0x3A,0x3A,0x3A,0.90f);
    case ThemeToken::GizmoBackground:               return Hex(0x3A,0x3A,0x3A,0.90f);
    case ThemeToken::DialogBackground:              return Hex(0x3D,0x3D,0x3D);
        

    // Panel chrome — header strips inside dock panels (not top window chrome)
    case ThemeToken::HeaderBackground:              return Hex(0x1A,0x1A,0x1A);
    case ThemeToken::ContentBrowserFolderPrimary:   return Hex(0xC9,0xA9,0x62);
    

    // Tabs share the same surface as dock panel body
    case ThemeToken::TabBackground:                 return Hex(0x20,0x21,0x24);
    case ThemeToken::PanelTabInactiveBackground:    return Hex(0x20,0x21,0x24);
    case ThemeToken::PanelTabActiveBackground:      return Hex(0x20,0x21,0x24);

    // Top chrome — title bar and menu bar share main background
    case ThemeToken::MenuBarBackground:             return Hex(0x16,0x17,0x18);
    case ThemeToken::StatusBarBackground:           return Hex(0x18,0x18,0x1C);
    case ThemeToken::FooterBackground:              return Hex(0x18,0x18,0x1C);    

    // Main editor toolbar only
    case ThemeToken::ToolbarBackground:             return Hex(0x1F,0x1F,0x20);
    case ThemeToken::PanelToolbarBackground:        return Hex(0x1A,0x1B,0x1E);
    case ThemeToken::ViewportToolbarBackground:     return Hex(0x1A,0x1B,0x1E,0.96f);

    // ── Borders (minimal — soft luminance separators) ────────────────────────
    case ThemeToken::Separator:                     return Hex(0xFF,0xFF,0xFF,0.08f);
    case ThemeToken::BorderDark:                    return Hex(0xFF,0xFF,0xFF,0.04f);
    case ThemeToken::BorderDefault:                 return Hex(0x30,0x30,0x30);
    case ThemeToken::BorderLight:                   return Hex(0xFF,0xFF,0xFF,0.16f);
    
    case ThemeToken::ContentBrowserFolderEdge:      return Hex(0x7A,0x65,0x32);
    case ThemeToken::ContentBrowserFolderHighlight: return Hex(0xEB,0xD8,0x9A);

    // ── Interactive material states ────────────────────────────────────────
    case ThemeToken::HoverBackground:               return Hex(0x2B,0x2B,0x2B);
    case ThemeToken::ContentBrowserHoverBackground: return Hex(0x2B,0x2B,0x2B);
    case ThemeToken::ButtonPrimaryBackground:       return Hex(0x23,0x23,0x26);
    
    case ThemeToken::PressedBackground:             return Hex(0x34,0x36,0x3B);
    case ThemeToken::ButtonPrimaryPressed:          return Hex(0x34,0x36,0x3B);
    
    case ThemeToken::SelectedBackground:            return Hex(0x34,0x34,0x34);
    case ThemeToken::ActiveBackground:              return Hex(0x23,0x23,0x23);
    case ThemeToken::ButtonPrimaryHover:            return Hex(0x2E,0x2F,0x33);
    case ThemeToken::ContentBrowserFolderTab:       return Hex(0xD4,0xBC,0x78);

    // ── Typography ─────────────────────────────────────────────────────────
    case ThemeToken::TextPrimary:                   return Hex(0xF0,0xF0,0xF0);
    case ThemeToken::TextWindowLabel:               return Hex(0xF0,0xF0,0xF0);
    case ThemeToken::CodeForeground:                return Hex(0xF0,0xF0,0xF0);

    case ThemeToken::TextSecondary:                 return Hex(0xA8,0xA8,0xA8);
    case ThemeToken::TextMuted:                     return Hex(0x70,0x70,0x78);
    case ThemeToken::SearchPlaceholder:             return Hex(0x70,0x70,0x78);
    case ThemeToken::TextDisabled:                  return Hex(0x58,0x58,0x60);

    // ── Icons (Primary / Secondary / Accent semantic roles) ─────────────────
    case ThemeToken::IconPrimary:                   return Hex(0xCF,0xCF,0xCF);
    case ThemeToken::IconSecondary:                 return Hex(0xA8,0xA8,0xA8);
    case ThemeToken::IconAccent:                    return Hex(0xF0,0xA4,0x2A);
    case ThemeToken::IconDefault:                   return Hex(0xCF,0xCF,0xCF);
    case ThemeToken::IconHover:                     return Hex(0xE8,0xE8,0xE8);
    case ThemeToken::IconActive:                    return Hex(0xF0,0xF0,0xF0);
    case ThemeToken::IconDisabled:                  return Hex(0x58,0x58,0x60);
    // ── Accent & semantic ──────────────────────────────────────────────────
    case ThemeToken::AccentPrimary:                 return Hex(0xF0,0xA4,0x2A);
    case ThemeToken::AccentHover:                   return Hex(0xF5,0xB8,0x45);
    
    case ThemeToken::BorderFocus:                   return Hex(0xF0,0xA4,0x2A);
    case ThemeToken::ActiveTabLine:                 return Hex(0xF0,0xA4,0x2A,0.80f);
    case ThemeToken::SelectionHighlight:            return Hex(0x34,0x34,0x34,0.85f);
    case ThemeToken::LinkForeground:                return Hex(0xF0,0xA4,0x2A);

    case ThemeToken::PlayForeground:                return Hex(0xF0,0xA4,0x2A);
    case ThemeToken::Success:                       return Hex(0x5E,0xAD,0x6E);
    case ThemeToken::Warning:                       return Hex(0xE0,0xA2,0x3A);
    case ThemeToken::ErrorForeground:               return Hex(0xDD,0x5A,0x5A);
    case ThemeToken::CloseButtonHover:              return Hex(0xDD,0x5A,0x5A);

    // ── Scrollbars ─────────────────────────────────────────────────────────
    case ThemeToken::ScrollbarThumb:                return Hex(0x55,0x55,0x55);
    case ThemeToken::ScrollbarThumbHover:           return Hex(0x66,0x66,0x66);

    // Viewport gizmo axes (functional orientation colors)
    case ThemeToken::GizmoAxisX: return MakeColor(0.90f, 0.25f, 0.25f, 1.0f);
    case ThemeToken::GizmoAxisY: return MakeColor(0.30f, 0.85f, 0.35f, 1.0f);
    case ThemeToken::GizmoAxisZ: return MakeColor(0.30f, 0.50f, 0.95f, 1.0f);

    // ── Depth & elevation (shadows replace heavy borders on floating UI) ─────
    case ThemeToken::HighlightSubtle: return MakeColor(1.0f, 1.0f, 1.0f, 0.06f);
    case ThemeToken::ShadowPopup: return MakeColor(0.0f, 0.0f, 0.0f, 0.28f);
    case ThemeToken::ShadowSubtle: return MakeColor(0.0f, 0.0f, 0.0f, 0.40f);
    case ThemeToken::ShadowOverlay: return MakeColor(0.0f, 0.0f, 0.0f, 0.48f);
    case ThemeToken::ModalScrim: return MakeColor(0.0f, 0.0f, 0.0f, 0.62f);
    case ThemeToken::ContentBrowserFolderShadow: return MakeColor(0.0f, 0.0f, 0.0f, 0.38f);

    default: return Color::Transparent();
    }
}

float GraphiteDarkTheme::GetMetric(ThemeToken token) const {
    switch (token) {
    case ThemeToken::CornerRadiusSmall: return 6.0f;
    case ThemeToken::CornerRadiusMedium: return 7.0f;
    case ThemeToken::CornerRadiusLarge: return 8.0f;
    case ThemeToken::WindowCornerRadius: return 8.0f;
    case ThemeToken::TextSizeMenu: return 12.0f;
    case ThemeToken::TextSizeToolbar: return 12.0f;
    case ThemeToken::TextSizeTabs: return 12.0f;
    case ThemeToken::TextSizeNormal: return 12.0f;
    case ThemeToken::TextSizeProperty: return 12.0f;
    case ThemeToken::TextSizeCaption: return 10.0f;
    case ThemeToken::TextSizeWindow: return 12.0f;
    case ThemeToken::TextSizeHeader: return 12.0f;
    case ThemeToken::TextSizeBody: return 12.0f;
    case ThemeToken::TextSizeSmall: return 11.0f;
    case ThemeToken::TextSizeCategory: return 12.0f;
    case ThemeToken::BorderWidth: return 1.0f;
    case ThemeToken::PanelHeaderHeight: return 32.0f;
    case ThemeToken::PanelTabHeight: return 32.0f;
    case ThemeToken::PanelToolbarHeight: return 32.0f;
    case ThemeToken::ListRowHeight: return 32.0f;
    case ThemeToken::CategoryHeaderHeight: return 28.0f;
    case ThemeToken::TitleBarHeight: return 34.0f;
    case ThemeToken::HeaderControlHeight: return 32.0f;
    case ThemeToken::WindowControlWidth: return 40.0f;
    case ThemeToken::ToolbarHeight: return 44.0f;
    case ThemeToken::SearchBoxHeight: return 28.0f;
    case ThemeToken::IconButtonSize: return 32.0f;
    case ThemeToken::ButtonHeight: return 32.0f;
    case ThemeToken::NavigationButtonSize: return 32.0f;
    case ThemeToken::IconSizeSearch: return 16.0f;
    case ThemeToken::IconSizeToolbar: return 16.0f;
    case ThemeToken::IconSizePrimary: return 16.0f;
    case ThemeToken::IconSizeTree: return 16.0f;
    case ThemeToken::IconSizeNavigation: return 16.0f;
    case ThemeToken::IconButtonRadius: return 6.0f;
    case ThemeToken::ButtonPaddingHorizontal: return 8.0f;
    case ThemeToken::ButtonSpacing: return 4.0f;
    case ThemeToken::ButtonGroupSpacing: return 10.0f;
    case ThemeToken::ScrollbarWidth: return 10.0f;
    case ThemeToken::Space1: return 4.0f;
    case ThemeToken::Space2: return 8.0f;
    case ThemeToken::Space3: return 12.0f;
    case ThemeToken::Space4: return 16.0f;
    case ThemeToken::Space5: return 20.0f;
    case ThemeToken::Space6: return 24.0f;
    case ThemeToken::HoverAnimationDamping: return 10.0f;
    case ThemeToken::PressAnimationDamping: return 14.0f;
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
        return {8.0f, 8.0f, 8.0f, 8.0f};
    case ThemeToken::PaddingButtonLeft:
    case ThemeToken::PaddingButtonTop:
    case ThemeToken::PaddingButtonRight:
    case ThemeToken::PaddingButtonBottom:
        return {8.0f, 4.0f, 8.0f, 4.0f};
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
    if (active) {
        return GetColor(ThemeToken::IconAccent);
    }
    Color base = GetColor(ThemeToken::IconSecondary);
    if (hovered) {
        return Color::Lerp(base, GetColor(ThemeToken::IconPrimary), 1.0f);
    }
    return base;
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
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizeToolbar));
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
        style.background = theme.GetColor(ThemeToken::PanelBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.height = Scaled(theme.GetMetric(ThemeToken::PanelHeaderHeight));
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeTabs));
        break;
    case StyleRole::Tab:
    case StyleRole::DockTab:
        style.background = theme.GetColor(ThemeToken::PanelTabInactiveBackground);
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeTabs));
        break;
    case StyleRole::TabActive:
    case StyleRole::DockTabActive:
        style.background = theme.GetColor(ThemeToken::PanelTabActiveBackground);
        style.foreground = theme.GetColor(ThemeToken::TextPrimary);
        style.border = theme.GetColor(ThemeToken::BorderLight);
        style.fontSize = Scaled(theme.GetMetric(ThemeToken::TextSizeTabs));
        break;
    case StyleRole::Button:
        style.background = Color::Transparent();
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::ButtonHeight));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizeToolbar));
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
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusSmall));
        break;
    case StyleRole::ButtonSecondary:
        style.background = Color::Transparent();
        style.foreground = theme.GetColor(ThemeToken::TextSecondary);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::ButtonHeight));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.GetMetric(ThemeToken::CornerRadiusSmall));
        break;
    case StyleRole::IconButton:
        style.background = Color::Transparent();
        style.icon = theme.GetColor(ThemeToken::IconSecondary);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::IconButtonSize));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizeToolbar));
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
        style.icon = theme.GetColor(ThemeToken::IconSecondary);
        style.border = theme.GetColor(ThemeToken::BorderDefault);
        style.height = Scaled(theme.GetMetric(ThemeToken::NavigationButtonSize));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizeNavigation));
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
        style.background = theme.GetColor(ThemeToken::WindowBackground);
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
        style.background = theme.GetColor(ThemeToken::DialogBackground);
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
