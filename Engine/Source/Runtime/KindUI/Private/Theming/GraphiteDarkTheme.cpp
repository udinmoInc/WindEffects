#include "KindUI/Theming/GraphiteDarkTheme.h"
#include "KindUI/Theming/StyleResolve.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>
#include <array>
#include <unordered_map>

namespace we::runtime::kindui {
namespace {

Color MakeColor(float r, float g, float b, float a = 1.0f) {
    return {r, g, b, a};
}

Color Hex(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return MakeColor(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

} // namespace

Color GraphiteDarkTheme::ResolveColor(ColorToken token) const {
    switch (token) {
    // ── Material stack (darkest → lightest) ──────────────────────────────────
    case ColorToken::WindowBackground:              return Hex(0x16,0x16,0x16);
    case ColorToken::ViewportBackground:            return Hex(0x16,0x16,0x16);
    case ColorToken::DockAreaBackground:            return Hex(0x16,0x16,0x16);
    case ColorToken::WorkspaceBackground:           return Hex(0x16,0x16,0x16);
    case ColorToken::DisabledBackground:            return Hex(0x16,0x16,0x16);

    case ColorToken::SearchBoxBackground:           return Hex(0x1A,0x1A,0x1A);
    case ColorToken::InputBackground:               return Hex(0x1A,0x1A,0x1A);
    case ColorToken::ScrollbarTrack:                return Hex(0x16,0x16,0x16);

    case ColorToken::PanelBackground:               return Hex(0x1D,0x1D,0x1D);
    case ColorToken::PanelContentBackground:        return Hex(0x1D,0x1D,0x1D);
    case ColorToken::ContentBrowserBackground:      return Hex(0x1D,0x1D,0x1D);
    case ColorToken::ContentBrowserFolderBody:      return Hex(0xB8,0x95,0x48);

    // Floating surfaces — elevation via luminance + shadow tokens
    case ColorToken::PopupBackground:               return Hex(0x3A,0x3A,0x3A);
    case ColorToken::TooltipBackground:             return Hex(0x3A,0x3A,0x3A,0.96f);
    case ColorToken::DragGhostBackground:           return Hex(0x3A,0x3A,0x3A,0.90f);
    case ColorToken::GizmoBackground:               return Hex(0x3A,0x3A,0x3A,0.90f);
    case ColorToken::DialogBackground:              return Hex(0x3D,0x3D,0x3D);
        

    // Panel chrome — header strips inside dock panels (not top window chrome)
    case ColorToken::HeaderBackground:              return Hex(0x1A,0x1A,0x1A);
    case ColorToken::ContentBrowserFolderPrimary:   return Hex(0xC9,0xA9,0x62);
    

    // Tabs share the same surface as dock panel body
    case ColorToken::TabBackground:                 return Hex(0x20,0x21,0x24);
    case ColorToken::PanelTabInactiveBackground:    return Hex(0x20,0x21,0x24);
    case ColorToken::PanelTabActiveBackground:      return Hex(0x20,0x21,0x24);

    // Top chrome — title bar and menu bar share main background
    case ColorToken::MenuBarBackground:             return Hex(0x16,0x17,0x18);
    case ColorToken::StatusBarBackground:           return Hex(0x18,0x18,0x1C);
    case ColorToken::FooterBackground:              return Hex(0x18,0x18,0x1C);    

    // Main editor toolbar only
    case ColorToken::ToolbarBackground:             return Hex(0x1F,0x1F,0x20);
    case ColorToken::PanelToolbarBackground:        return Hex(0x1A,0x1B,0x1E);
    case ColorToken::ViewportToolbarBackground:     return Hex(0x1A,0x1B,0x1E,0.96f);

    // ── Borders (minimal — soft luminance separators) ────────────────────────
    case ColorToken::Separator:                     return Hex(0xFF,0xFF,0xFF,0.08f);
    case ColorToken::BorderDark:                    return Hex(0xFF,0xFF,0xFF,0.04f);
    case ColorToken::BorderDefault:                 return Hex(0x30,0x30,0x30);
    case ColorToken::BorderLight:                   return Hex(0xFF,0xFF,0xFF,0.16f);
    
    case ColorToken::ContentBrowserFolderEdge:      return Hex(0x7A,0x65,0x32);
    case ColorToken::ContentBrowserFolderHighlight: return Hex(0xEB,0xD8,0x9A);

    // ── Interactive material states ────────────────────────────────────────
    case ColorToken::HoverBackground:               return Hex(0x2B,0x2B,0x2B);
    case ColorToken::ContentBrowserHoverBackground: return Hex(0x2B,0x2B,0x2B);
    case ColorToken::ButtonPrimaryBackground:       return Hex(0x23,0x23,0x26);
    
    case ColorToken::PressedBackground:             return Hex(0x34,0x36,0x3B);
    case ColorToken::ButtonPrimaryPressed:          return Hex(0x34,0x36,0x3B);
    case ColorToken::ButtonDangerBackground:        return Hex(0x8B,0x2E,0x2E);
    case ColorToken::ButtonDangerHover:             return Hex(0xA3,0x3A,0x3A);
    case ColorToken::ButtonDangerPressed:           return Hex(0x6E,0x22,0x22);
    
    case ColorToken::SelectedBackground:            return Hex(0x34,0x34,0x34);
    case ColorToken::ActiveBackground:              return Hex(0x23,0x23,0x23);
    case ColorToken::ButtonPrimaryHover:            return Hex(0x2E,0x2F,0x33);
    case ColorToken::ContentBrowserFolderTab:       return Hex(0xD4,0xBC,0x78);

    // ── Typography ─────────────────────────────────────────────────────────
    case ColorToken::TextPrimary:                   return Hex(0xF0,0xF0,0xF0);
    case ColorToken::TextWindowLabel:               return Hex(0xF0,0xF0,0xF0);
    case ColorToken::CodeForeground:                return Hex(0xF0,0xF0,0xF0);

    case ColorToken::TextSecondary:                 return Hex(0xA8,0xA8,0xA8);
    case ColorToken::TextMuted:                     return Hex(0x70,0x70,0x78);
    case ColorToken::SearchPlaceholder:             return Hex(0x70,0x70,0x78);
    case ColorToken::TextDisabled:                  return Hex(0x58,0x58,0x60);

    // ── Icons (Primary / Secondary / Accent semantic roles) ─────────────────
    // Base Graphite uses a muted cool-gray accent. EditorTheme / LauncherTheme
    // override Accent* / IconAccent / BorderFocus for product identity.
    case ColorToken::IconPrimary:                   return Hex(0xCF,0xCF,0xCF);
    case ColorToken::IconSecondary:                 return Hex(0xA8,0xA8,0xA8);
    case ColorToken::IconAccent:                    return Hex(0x8A,0x96,0xA8);
    case ColorToken::IconDefault:                   return Hex(0xCF,0xCF,0xCF);
    case ColorToken::IconHover:                     return Hex(0xE8,0xE8,0xE8);
    case ColorToken::IconActive:                    return Hex(0xF0,0xF0,0xF0);
    case ColorToken::IconDisabled:                  return Hex(0x58,0x58,0x60);
    // ── Accent & semantic (neutral base — specialized by Editor/Launcher) ──
    case ColorToken::AccentPrimary:                 return Hex(0x8A,0x96,0xA8);
    case ColorToken::AccentHover:                   return Hex(0xA0,0xAA,0xB8);
    
    case ColorToken::BorderFocus:                   return Hex(0x8A,0x96,0xA8);
    case ColorToken::ActiveTabLine:                 return Hex(0x8A,0x96,0xA8,0.80f);
    case ColorToken::SelectionHighlight:            return Hex(0x34,0x34,0x34,0.85f);
    case ColorToken::LinkForeground:                return Hex(0x8A,0x96,0xA8);

    case ColorToken::PlayForeground:                return Hex(0x8A,0x96,0xA8);
    case ColorToken::Success:                       return Hex(0x5E,0xAD,0x6E);
    case ColorToken::Warning:                       return Hex(0xE0,0xA2,0x3A);
    case ColorToken::ErrorForeground:               return Hex(0xDD,0x5A,0x5A);
    case ColorToken::CloseButtonHover:              return Hex(0xDD,0x5A,0x5A);

    // ── Scrollbars ─────────────────────────────────────────────────────────
    case ColorToken::ScrollbarThumb:                return Hex(0x55,0x55,0x55);
    case ColorToken::ScrollbarThumbHover:           return Hex(0x66,0x66,0x66);

    // Viewport gizmo axes (functional orientation colors)
    case ColorToken::GizmoAxisX: return MakeColor(0.90f, 0.25f, 0.25f, 1.0f);
    case ColorToken::GizmoAxisY: return MakeColor(0.30f, 0.85f, 0.35f, 1.0f);
    case ColorToken::GizmoAxisZ: return MakeColor(0.30f, 0.50f, 0.95f, 1.0f);

    // ── Depth & elevation (shadows replace heavy borders on floating UI) ─────
    case ColorToken::HighlightSubtle: return MakeColor(1.0f, 1.0f, 1.0f, 0.06f);
    case ColorToken::ShadowPopup: return MakeColor(0.0f, 0.0f, 0.0f, 0.28f);
    case ColorToken::ShadowSubtle: return MakeColor(0.0f, 0.0f, 0.0f, 0.40f);
    case ColorToken::ShadowOverlay: return MakeColor(0.0f, 0.0f, 0.0f, 0.48f);
    case ColorToken::ModalScrim: return MakeColor(0.0f, 0.0f, 0.0f, 0.62f);
    case ColorToken::ContentBrowserFolderShadow: return MakeColor(0.0f, 0.0f, 0.0f, 0.38f);


    // DesignToken aliases used by StylePipeline / DefaultTheme
    case ColorToken::PrimarySurface: return Hex(0x1D,0x1D,0x1D);
    case ColorToken::SecondarySurface: return Hex(0x1D,0x1D,0x1D);
    case ColorToken::TertiarySurface: return Hex(0x23,0x23,0x23);
    case ColorToken::AccentSurface: return Hex(0x8A,0x96,0xA8);
    case ColorToken::ControlBackground: return Hex(0x1A,0x1A,0x1A);
    case ColorToken::ControlBackgroundHover: return Hex(0x2B,0x2B,0x2B);
    case ColorToken::ControlBackgroundPressed: return Hex(0x34,0x36,0x3B);
    case ColorToken::ControlBackgroundDisabled: return Hex(0x16,0x16,0x16);
    case ColorToken::ControlBackgroundSelected: return Hex(0x34,0x34,0x34);
    case ColorToken::BorderSubtle: return Hex(0xFF,0xFF,0xFF,0.04f);
    case ColorToken::BorderFocused: return Hex(0x8A,0x96,0xA8);
    case ColorToken::BorderError: return Hex(0xDD,0x5A,0x5A);
    case ColorToken::TextOnAccent: return Color{1.0f, 1.0f, 1.0f, 1.0f};
    case ColorToken::TextLink: return Hex(0x8A,0x96,0xA8);
    case ColorToken::SuccessColor: return Hex(0x5E,0xAD,0x6E);
    case ColorToken::WarningColor: return Hex(0xE0,0xA2,0x3A);
    case ColorToken::ErrorColor: return Hex(0xDD,0x5A,0x5A);
    case ColorToken::InfoColor: return Hex(0x8A,0x96,0xA8);
    case ColorToken::ScrimOverlay: return MakeColor(0.0f, 0.0f, 0.0f, 0.62f);
    case ColorToken::ShadowColor: return MakeColor(0.0f, 0.0f, 0.0f, 0.40f);

    default: return Color::Transparent();
    }
}

float GraphiteDarkTheme::ResolveMetric(MetricToken token) const {
    switch (token) {
    case MetricToken::CornerRadiusSmall: return 6.0f;
    case MetricToken::CornerRadiusMedium: return 7.0f;
    case MetricToken::CornerRadiusLarge: return 8.0f;
    case MetricToken::WindowCornerRadius: return 8.0f;
    case MetricToken::TextSizeMenu: return 12.0f;
    case MetricToken::TextSizeToolbar: return 12.0f;
    case MetricToken::TextSizeTabs: return 12.0f;
    case MetricToken::TextSizeNormal: return 12.0f;
    case MetricToken::TextSizeProperty: return 12.0f;
    case MetricToken::TextSizeCaption: return 11.0f;
    case MetricToken::TextSizeWindow: return 13.0f;
    case MetricToken::TextSizeHeader: return 18.0f;
    case MetricToken::TextSizeBody: return 13.0f;
    case MetricToken::TextSizeSmall: return 11.0f;
    case MetricToken::TextSizeCategory: return 12.0f;
    case MetricToken::TextSizeTitle: return 22.0f;
    case MetricToken::TextCharWidthRatio: return 0.55f;
    case MetricToken::BorderWidth: return 1.0f;
    case MetricToken::FocusRingWidth: return 2.0f;
    case MetricToken::PanelHeaderHeight: return 32.0f;
    case MetricToken::PanelTabHeight: return 32.0f;
    case MetricToken::PanelToolbarHeight: return 32.0f;
    case MetricToken::ListRowHeight: return 48.0f;
    case MetricToken::CategoryHeaderHeight: return 28.0f;
    case MetricToken::TitleBarHeight: return 34.0f;
    case MetricToken::HeaderControlHeight: return 32.0f;
    case MetricToken::WindowControlWidth: return 40.0f;
    case MetricToken::ToolbarHeight: return 44.0f;
    case MetricToken::SearchBoxHeight: return 28.0f;
    case MetricToken::IconButtonSize: return 32.0f;
    case MetricToken::ButtonHeight: return 32.0f;
    case MetricToken::NavigationButtonSize: return 32.0f;
    case MetricToken::IconSizeSearch: return 16.0f;
    case MetricToken::IconSizeToolbar: return 16.0f;
    case MetricToken::IconSizePrimary: return 16.0f;
    case MetricToken::IconSizeTree: return 16.0f;
    case MetricToken::IconSizeNavigation: return 16.0f;
    case MetricToken::IconButtonRadius: return 6.0f;
    case MetricToken::ButtonPaddingHorizontal: return 8.0f;
    case MetricToken::ButtonSpacing: return 4.0f;
    case MetricToken::ButtonGroupSpacing: return 10.0f;
    case MetricToken::ScrollbarWidth: return 10.0f;
    case MetricToken::Space1: return 4.0f;
    case MetricToken::Space2: return 8.0f;
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
        return ResolveMetric(MetricToken::TextSizeTitle) + 6.0f;
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
        return ResolveMetric(MetricToken::TextSizeHeader) - 2.0f;
    case TypographyToken::Heading4:
    case TypographyToken::Title:
        return ResolveMetric(MetricToken::TextSizeTitle) - 6.0f;
    case TypographyToken::Heading5:
    case TypographyToken::Subtitle:
        return ResolveMetric(MetricToken::TextSizeWindow);
    case TypographyToken::Heading6:
    case TypographyToken::Body:
    case TypographyToken::BodyStrong:
    case TypographyToken::Link:
        return ResolveMetric(MetricToken::TextSizeBody);
    case TypographyToken::Button:
        return ResolveMetric(MetricToken::TextSizeNormal);
    case TypographyToken::Label:
        return ResolveMetric(MetricToken::TextSizeSmall);
    case TypographyToken::Menu:
        return ResolveMetric(MetricToken::TextSizeMenu);
    case TypographyToken::Toolbar:
    case TypographyToken::Navigation:
        return ResolveMetric(MetricToken::TextSizeToolbar);
    case TypographyToken::Tooltip:
    case TypographyToken::Status:
    case TypographyToken::StatusBar:
    case TypographyToken::Caption:
    case TypographyToken::Error:
    case TypographyToken::Warning:
    case TypographyToken::Success:
    case TypographyToken::Disabled:
        return ResolveMetric(MetricToken::TextSizeSmall);
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
    switch (token) {
    case ElevationToken::None: return 0;
    case ElevationToken::Card: return 1;
    case ElevationToken::Popup: return 2;
    case ElevationToken::Window: return 3;
    case ElevationToken::Overlay: return 4;
    default: return 0;
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
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.padding = theme.ResolvePadding(PaddingToken::PaddingPanelLeft);
        for (auto& v : {&style.padding.left, &style.padding.top, &style.padding.right, &style.padding.bottom}) {
            *v = Scaled(*v);
        }
        break;
    case StyleRole::PanelHeader:
        style.background = theme.ResolveColor(ColorToken::PanelBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(theme.ResolveMetric(MetricToken::PanelHeaderHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        break;
    case StyleRole::Tab:
    case StyleRole::DockTab:
        style.background = theme.ResolveColor(ColorToken::PanelTabInactiveBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        break;
    case StyleRole::TabActive:
    case StyleRole::DockTabActive:
        style.background = theme.ResolveColor(ColorToken::PanelTabActiveBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderLight);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeTabs));
        break;
    case StyleRole::Button:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
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
        style.background = theme.ResolveColor(ColorToken::ActiveBackground);
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
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
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
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
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
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.elevation = 2;
        break;
    case StyleRole::Tooltip:
        style.background = theme.ResolveColor(ColorToken::TooltipBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.elevation = 2;
        break;
    case StyleRole::Modal:
        style.background = theme.ResolveColor(ColorToken::DialogBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::WindowCornerRadius));
        break;
    case StyleRole::Gizmo:
        style.background = theme.ResolveColor(ColorToken::GizmoBackground);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::ContentBrowser:
        style.background = theme.ResolveColor(ColorToken::ContentBrowserBackground);
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
        style.foreground = theme.ResolveColor(ColorToken::TextMuted);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeCaption));
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
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
        style.height = Scaled(theme.ResolveMetric(MetricToken::HeaderControlHeight));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeToolbar));
        break;
    case StyleRole::Card:
        style.background = theme.ResolveColor(ColorToken::PanelBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.padding = { Scaled(12.0f), Scaled(12.0f), Scaled(12.0f), Scaled(12.0f) };
        style.elevation = 1;
        break;
    case StyleRole::CardHover:
        style.background = theme.ResolveColor(ColorToken::HoverBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderLight);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusMedium));
        style.elevation = 2;
        break;
    case StyleRole::TableHeader:
        style.background = theme.ResolveColor(ColorToken::PanelToolbarBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextMuted);
        style.height = Scaled(28.0f);
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
        style.height = Scaled(theme.ResolveMetric(MetricToken::ListRowHeight) * 0.75f);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeProperty));
        break;
    case StyleRole::SidebarItem:
        style.background = Color::Transparent();
        style.foreground = theme.ResolveColor(ColorToken::TextSecondary);
        style.icon = theme.ResolveColor(ColorToken::IconSecondary);
        style.height = Scaled(40.0f);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeNavigation));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        break;
    case StyleRole::SidebarItemActive:
        style.background = theme.ResolveColor(ColorToken::SelectedBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.icon = theme.ResolveColor(ColorToken::AccentPrimary);
        style.border = theme.ResolveColor(ColorToken::AccentPrimary);
        style.height = Scaled(40.0f);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeBody));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeNavigation));
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.bold = true;
        break;
    case StyleRole::WindowHeader:
        style.background = theme.ResolveColor(ColorToken::HeaderBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextWindowLabel);
        style.height = Scaled(theme.ResolveMetric(MetricToken::TitleBarHeight));
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeWindow));
        break;
    case StyleRole::Checkbox:
    case StyleRole::ToggleSwitch:
        style.background = theme.ResolveColor(ColorToken::InputBackground);
        style.foreground = theme.ResolveColor(ColorToken::AccentPrimary);
        style.border = theme.ResolveColor(ColorToken::BorderDefault);
        style.cornerRadius = Scaled(theme.ResolveMetric(MetricToken::CornerRadiusSmall));
        style.height = Scaled(22.0f);
        style.iconSize = Scaled(14.0f);
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
        style.height = Scaled(24.0f);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        style.iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeTree));
        break;
    case StyleRole::TreeItemSelected:
        style.background = theme.ResolveColor(ColorToken::SelectedBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextPrimary);
        style.height = Scaled(24.0f);
        style.fontSize = Scaled(theme.ResolveMetric(MetricToken::TextSizeSmall));
        break;
    default:
        break;
    }

    style.borderWidth = Scaled(theme.ResolveMetric(MetricToken::BorderWidth));
    return style;
}

} // namespace we::runtime::kindui
