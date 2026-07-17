#include "KindUI/Theming/StyleFactory.h"

#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

namespace we::runtime::kindui {
namespace {

ShadowStyle ElevationToShadow(int elevation) {
    switch (elevation) {
    case 1: return ShadowStyle::Small();
    case 2: return ShadowStyle::Medium();
    case 3: return ShadowStyle::Large();
    default: return ShadowStyle::None();
    }
}

WidgetStyle FromRole(StyleRole role, StyleRole hoverRole, StyleRole pressRole) {
    const auto& styles = ThemeManager::Get().Styles();
    const ResolvedStyle base = styles.Resolve(role);
    const ResolvedStyle hover = styles.Resolve(hoverRole);
    const ResolvedStyle press = styles.Resolve(pressRole);

    WidgetStyle style;
    style.background = BackgroundStyle{ base.background, base.cornerRadius };
    style.border = BorderStyle{
        base.borderWidth,
        base.border,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius
    };
    style.text = TextStyle{ base.foreground, base.fontSize, base.bold, false };
    style.shadow = ElevationToShadow(base.elevation);
    style.padding = base.padding;
    style.backgroundHover = BackgroundStyle{ hover.background, hover.cornerRadius };
    style.backgroundPressed = BackgroundStyle{ press.background, press.cornerRadius };
    style.borderFocused = BorderStyle{
        ResolveMetric(MetricToken::FocusRingWidth),
        ResolveColor(ColorToken::BorderFocus),
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius,
        base.cornerRadius
    };
    return style;
}

} // namespace

BorderStyle StyleFactory::BorderNone() {
    return BorderStyle{0.0f, Color::Transparent(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

BorderStyle StyleFactory::BorderThin(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::Panel);
    return BorderStyle{
        resolved.borderWidth,
        resolved.border,
        resolved.cornerRadius,
        resolved.cornerRadius,
        resolved.cornerRadius,
        resolved.cornerRadius,
        resolved.cornerRadius
    };
}

BorderStyle StyleFactory::BorderSelected(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::TableRowSelected);
    return BorderStyle{
        ResolveMetric(MetricToken::FocusRingWidth),
        ResolveColor(ColorToken::AccentPrimary),
        resolved.cornerRadius,
        resolved.cornerRadius,
        resolved.cornerRadius,
        resolved.cornerRadius,
        resolved.cornerRadius
    };
}

BackgroundStyle StyleFactory::BackgroundNone() {
    return BackgroundStyle{Color::Transparent(), 0.0f};
}

BackgroundStyle StyleFactory::BackgroundPanel(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::Panel);
    return BackgroundStyle{resolved.background, resolved.cornerRadius};
}

BackgroundStyle StyleFactory::BackgroundToolbar(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::Toolbar);
    return BackgroundStyle{resolved.background, resolved.cornerRadius};
}

BackgroundStyle StyleFactory::BackgroundHover(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::ButtonHover);
    return BackgroundStyle{resolved.background, resolved.cornerRadius};
}

BackgroundStyle StyleFactory::BackgroundSelected(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::TableRowSelected);
    return BackgroundStyle{resolved.background, resolved.cornerRadius};
}

BackgroundStyle StyleFactory::BackgroundInput(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::Input);
    return BackgroundStyle{resolved.background, resolved.cornerRadius};
}

TextStyle StyleFactory::TextMenu(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::TextPrimary);
    return TextStyle{resolved.foreground, resolved.fontSize, false, false};
}

TextStyle StyleFactory::TextToolbar(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::Toolbar);
    return TextStyle{resolved.foreground, resolved.fontSize, false, false};
}

TextStyle StyleFactory::TextHeader(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::SectionHeader);
    return TextStyle{resolved.foreground, resolved.fontSize, true, false};
}

TextStyle StyleFactory::TextBody(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::TextPrimary);
    return TextStyle{resolved.foreground, resolved.fontSize, false, false};
}

TextStyle StyleFactory::TextSmall(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::TextCaption);
    return TextStyle{resolved.foreground, resolved.fontSize, false, false};
}

WidgetStyle StyleFactory::Panel(const IStyleResolver& styles) {
    (void)styles;
    return FromRole(StyleRole::Panel, StyleRole::CardHover, StyleRole::ButtonActive);
}

WidgetStyle StyleFactory::Button(const IStyleResolver& styles) {
    (void)styles;
    return FromRole(StyleRole::ButtonSecondary, StyleRole::ButtonHover, StyleRole::ButtonActive);
}

WidgetStyle StyleFactory::ToolButton(const IStyleResolver& styles) {
    (void)styles;
    return FromRole(StyleRole::ToolbarButton, StyleRole::ButtonHover, StyleRole::ButtonActive);
}

WidgetStyle StyleFactory::TextBox(const IStyleResolver& styles) {
    (void)styles;
    WidgetStyle style = FromRole(StyleRole::Input, StyleRole::Input, StyleRole::Input);
    const auto pad = ThemeManager::Get().Theme().ResolvePadding(PaddingToken::PaddingButtonLeft);
    style.padding = Margin{
        ResolveMetric(MetricToken::Space2),
        ResolveMetric(MetricToken::Space1),
        ResolveMetric(MetricToken::Space2),
        ResolveMetric(MetricToken::Space1)
    };
    (void)pad;
    return style;
}

WidgetStyle StyleFactory::TreeItem(const IStyleResolver& styles) {
    (void)styles;
    return FromRole(StyleRole::TreeItem, StyleRole::TableRowHover, StyleRole::TreeItemSelected);
}

WidgetStyle StyleFactory::PropertyLabel(const IStyleResolver& styles) {
    (void)styles;
    return FromRole(StyleRole::PropertyRow, StyleRole::PropertyRow, StyleRole::PropertyRow);
}

WidgetStyle StyleFactory::Tab(const IStyleResolver& styles) {
    (void)styles;
    return FromRole(StyleRole::Tab, StyleRole::ButtonHover, StyleRole::TabActive);
}

WidgetStyle StyleFactory::TabActive(const IStyleResolver& styles) {
    (void)styles;
    WidgetStyle style = FromRole(StyleRole::TabActive, StyleRole::TabActive, StyleRole::TabActive);
    style.text.bold = true;
    return style;
}

WidgetStyle WidgetStyle::Panel() {
    return StyleFactory::Panel(ThemeManager::Get().Styles());
}

WidgetStyle WidgetStyle::Button() {
    return StyleFactory::Button(ThemeManager::Get().Styles());
}

WidgetStyle WidgetStyle::ToolButton() {
    return StyleFactory::ToolButton(ThemeManager::Get().Styles());
}

WidgetStyle WidgetStyle::TextBox() {
    return StyleFactory::TextBox(ThemeManager::Get().Styles());
}

WidgetStyle WidgetStyle::TreeItem() {
    return StyleFactory::TreeItem(ThemeManager::Get().Styles());
}

} // namespace we::runtime::kindui

// export
