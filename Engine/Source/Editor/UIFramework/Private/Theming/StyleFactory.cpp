#include "WindEffects/Editor/UI/Theming/StyleFactory.h"

namespace WindEffects::Editor::UI {

BorderStyle StyleFactory::BorderNone() {
    return BorderStyle{0.0f, Color::Transparent(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

BorderStyle StyleFactory::BorderThin(const IStyleResolver& styles) {
    const auto& theme = styles;
    const auto resolved = theme.Resolve(StyleRole::Panel);
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
    const auto resolved = styles.Resolve(StyleRole::DockTabActive);
    return BorderStyle{1.5f, resolved.border, resolved.cornerRadius, resolved.cornerRadius, resolved.cornerRadius, resolved.cornerRadius, resolved.cornerRadius};
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
    const auto resolved = styles.Resolve(StyleRole::Button);
    return BackgroundStyle{resolved.background, resolved.cornerRadius};
}

BackgroundStyle StyleFactory::BackgroundSelected(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::Panel);
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
    const auto resolved = styles.Resolve(StyleRole::PanelHeader);
    return TextStyle{resolved.foreground, resolved.fontSize, true, false};
}

TextStyle StyleFactory::TextBody(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::TextPrimary);
    return TextStyle{resolved.foreground, resolved.fontSize, false, false};
}

TextStyle StyleFactory::TextSmall(const IStyleResolver& styles) {
    const auto resolved = styles.Resolve(StyleRole::TextSecondary);
    return TextStyle{resolved.foreground, resolved.fontSize, false, false};
}

WidgetStyle StyleFactory::Panel(const IStyleResolver& styles) {
    WidgetStyle style;
    style.background = BackgroundPanel(styles);
    style.border = BorderThin(styles);
    style.text = TextBody(styles);
    style.shadow = ShadowStyle::None();
    style.padding = styles.Resolve(StyleRole::Panel).padding;
    return style;
}

WidgetStyle StyleFactory::Button(const IStyleResolver& styles) {
    WidgetStyle style;
    style.background = BackgroundNone();
    style.border = BorderNone();
    style.text = TextToolbar(styles);
    style.shadow = ShadowStyle::None();
    style.padding = styles.Resolve(StyleRole::Button).padding;
    style.backgroundHover = BackgroundHover(styles);
    style.backgroundPressed = BackgroundSelected(styles);
    return style;
}

WidgetStyle StyleFactory::ToolButton(const IStyleResolver& styles) {
    return Button(styles);
}

WidgetStyle StyleFactory::TextBox(const IStyleResolver& styles) {
    WidgetStyle style;
    style.background = BackgroundInput(styles);
    style.border = BorderThin(styles);
    style.text = TextBody(styles);
    style.shadow = ShadowStyle::None();
    style.padding = Margin{8.0f, 6.0f, 8.0f, 6.0f};
    style.borderFocused = BorderSelected(styles);
    return style;
}

WidgetStyle StyleFactory::TreeItem(const IStyleResolver& styles) {
    WidgetStyle style;
    style.background = BackgroundNone();
    style.border = BorderNone();
    style.text = TextBody(styles);
    style.shadow = ShadowStyle::None();
    style.padding = Margin{4.0f, 2.0f, 4.0f, 2.0f};
    style.backgroundHover = BackgroundHover(styles);
    style.backgroundPressed = BackgroundSelected(styles);
    return style;
}

WidgetStyle StyleFactory::PropertyLabel(const IStyleResolver& styles) {
    WidgetStyle style;
    style.background = BackgroundNone();
    style.border = BorderNone();
    style.text = TextSmall(styles);
    style.shadow = ShadowStyle::None();
    style.padding = Margin{0.0f, 4.0f, 0.0f, 4.0f};
    return style;
}

WidgetStyle StyleFactory::Tab(const IStyleResolver& styles) {
    WidgetStyle style;
    style.background = BackgroundNone();
    style.border = BorderNone();
    style.text = TextToolbar(styles);
    style.shadow = ShadowStyle::None();
    style.padding = Margin{16.0f, 8.0f, 16.0f, 8.0f};
    style.backgroundHover = BackgroundHover(styles);
    return style;
}

WidgetStyle StyleFactory::TabActive(const IStyleResolver& styles) {
    WidgetStyle style = Tab(styles);
    style.text.bold = true;
    return style;
}

} // namespace WindEffects::Editor::UI
