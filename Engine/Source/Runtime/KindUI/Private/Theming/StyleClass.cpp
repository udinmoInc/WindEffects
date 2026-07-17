#include "KindUI/Theming/StyleClass.h"

namespace we::runtime::kindui {

StyleClassRegistry& StyleClassRegistry::Get() {
    static StyleClassRegistry instance;
    return instance;
}

void StyleClassRegistry::Register(StyleClass styleClass) {
    m_Classes[styleClass.name] = std::move(styleClass);
}

const StyleClass* StyleClassRegistry::Find(std::string_view name) const {
    auto it = m_Classes.find(std::string(name));
    return it != m_Classes.end() ? &it->second : nullptr;
}

StyleClass StyleClassRegistry::Resolve(std::string_view name) const {
    StyleClass resolved;
    resolved.name = std::string(name);

    std::vector<const StyleClass*> chain;
    std::string current(name);
    for (int guard = 0; guard < 16 && !current.empty(); ++guard) {
        const StyleClass* found = Find(current);
        if (!found) break;
        chain.push_back(found);
        current = found->parentName;
    }

    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        const StyleClass& c = **it;
        resolved.background = c.background;
        resolved.foreground = c.foreground;
        resolved.border = c.border;
        resolved.hoverBackground = c.hoverBackground;
        resolved.pressedBackground = c.pressedBackground;
        resolved.disabledBackground = c.disabledBackground;
        resolved.paddingToken = c.paddingToken;
        resolved.radiusToken = c.radiusToken;
        resolved.fontSizeToken = c.fontSizeToken;
        resolved.heightToken = c.heightToken;
        resolved.animDurationToken = c.animDurationToken;
        resolved.opacity = c.opacity;
        resolved.bold = c.bold;
        if (!c.parentName.empty()) {
            resolved.parentName = c.parentName;
        }
    }
    return resolved;
}

void StyleClassRegistry::RegisterDefaults() {
    StyleClass button;
    button.name = "Button";
    button.background = ThemeToken::PanelBackground;
    button.foreground = ThemeToken::TextPrimary;
    button.border = ThemeToken::BorderDefault;
    button.hoverBackground = ThemeToken::HoverBackground;
    button.pressedBackground = ThemeToken::PressedBackground;
    button.disabledBackground = ThemeToken::DisabledBackground;
    button.heightToken = ThemeToken::ButtonHeight;
    button.radiusToken = ThemeToken::CornerRadiusMedium;
    button.paddingToken = ThemeToken::Space3;
    button.fontSizeToken = ThemeToken::TextSizeBody;
    button.animDurationToken = ThemeToken::Space1; // placeholder metric until motion tokens exist
    Register(button);

    StyleClass primary = button;
    primary.name = "PrimaryButton";
    primary.parentName = "Button";
    primary.background = ThemeToken::ButtonPrimaryBackground;
    primary.hoverBackground = ThemeToken::ButtonPrimaryHover;
    primary.pressedBackground = ThemeToken::ButtonPrimaryPressed;
    primary.foreground = ThemeToken::TextPrimary;
    Register(primary);

    StyleClass secondary = button;
    secondary.name = "SecondaryButton";
    secondary.parentName = "Button";
    Register(secondary);

    StyleClass toolbar = button;
    toolbar.name = "ToolbarButton";
    toolbar.parentName = "Button";
    toolbar.heightToken = ThemeToken::ToolbarHeight;
    Register(toolbar);

    StyleClass iconBtn = button;
    iconBtn.name = "IconButton";
    iconBtn.parentName = "Button";
    iconBtn.paddingToken = ThemeToken::Space2;
    iconBtn.heightToken = ThemeToken::IconButtonSize;
    Register(iconBtn);

    StyleClass launcherBtn = primary;
    launcherBtn.name = "LauncherButton";
    launcherBtn.parentName = "PrimaryButton";
    Register(launcherBtn);

    StyleClass page;
    page.name = "Page";
    page.background = ThemeToken::WindowBackground;
    page.paddingToken = ThemeToken::Space5;
    Register(page);

    StyleClass card;
    card.name = "Card";
    card.background = ThemeToken::PanelBackground;
    card.border = ThemeToken::BorderDefault;
    card.radiusToken = ThemeToken::CornerRadiusLarge;
    card.paddingToken = ThemeToken::Space3;
    Register(card);

    StyleClass search;
    search.name = "SearchBar";
    search.parentName = "Button";
    search.background = ThemeToken::SearchBoxBackground;
    search.heightToken = ThemeToken::SearchBoxHeight;
    Register(search);
}

} // namespace we::runtime::kindui
