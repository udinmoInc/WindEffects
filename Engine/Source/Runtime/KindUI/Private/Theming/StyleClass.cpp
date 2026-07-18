#include "KindUI/Theming/StyleClass.h"

namespace we::runtime::kindui {

StyleClassRegistry& StyleClassRegistry::Get() {
    static StyleClassRegistry instance;
    return instance;
}

void StyleClassRegistry::Register(StyleClass styleClass) {
    std::scoped_lock lock(m_Mutex);
    m_Classes[styleClass.name] = std::move(styleClass);
}

const StyleClass* StyleClassRegistry::Find(std::string_view name) const {
    // Returns a pointer into the registry map. Callers must not retain it across
    // Register/RegisterDefaults. Prefer Resolve() for thread-safe copies.
    std::scoped_lock lock(m_Mutex);
    auto it = m_Classes.find(std::string(name));
    return it != m_Classes.end() ? &it->second : nullptr;
}

StyleClass StyleClassRegistry::Resolve(std::string_view name) const {
    std::scoped_lock lock(m_Mutex);

    StyleClass resolved;
    resolved.name = std::string(name);

    std::vector<const StyleClass*> chain;
    std::string current(name);
    for (int guard = 0; guard < 16 && !current.empty(); ++guard) {
        auto it = m_Classes.find(current);
        if (it == m_Classes.end()) {
            break;
        }
        chain.push_back(&it->second);
        current = it->second.parentName;
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
    button.background = ColorToken::PanelBackground;
    button.foreground = ColorToken::TextPrimary;
    button.border = ColorToken::BorderDefault;
    button.hoverBackground = ColorToken::HoverBackground;
    button.pressedBackground = ColorToken::PressedBackground;
    button.disabledBackground = ColorToken::DisabledBackground;
    button.heightToken = MetricToken::ButtonHeight;
    button.radiusToken = MetricToken::CornerRadiusMedium;
    button.paddingToken = PaddingToken::Button;
    button.fontSizeToken = MetricToken::TextSizeBody;
    button.animDurationToken = MetricToken::Space1; // placeholder metric until motion tokens exist
    Register(button);

    StyleClass primary = button;
    primary.name = "PrimaryButton";
    primary.parentName = "Button";
    primary.background = ColorToken::ButtonPrimaryBackground;
    primary.hoverBackground = ColorToken::ButtonPrimaryHover;
    primary.pressedBackground = ColorToken::ButtonPrimaryPressed;
    primary.foreground = ColorToken::TextPrimary;
    Register(primary);

    StyleClass secondary = button;
    secondary.name = "SecondaryButton";
    secondary.parentName = "Button";
    Register(secondary);

    StyleClass toolbar = button;
    toolbar.name = "ToolbarButton";
    toolbar.parentName = "Button";
    toolbar.heightToken = MetricToken::ToolbarHeight;
    Register(toolbar);

    StyleClass iconBtn = button;
    iconBtn.name = "IconButton";
    iconBtn.parentName = "Button";
    iconBtn.paddingToken = PaddingToken::Button;
    iconBtn.heightToken = MetricToken::IconButtonSize;
    Register(iconBtn);

    StyleClass launcherBtn = primary;
    launcherBtn.name = "LauncherButton";
    launcherBtn.parentName = "PrimaryButton";
    Register(launcherBtn);

    StyleClass page;
    page.name = "Page";
    page.background = ColorToken::WindowBackground;
    page.paddingToken = PaddingToken::Panel;
    Register(page);

    StyleClass card;
    card.name = "Card";
    card.background = ColorToken::PanelBackground;
    card.border = ColorToken::BorderDefault;
    card.radiusToken = MetricToken::CornerRadiusLarge;
    card.paddingToken = PaddingToken::Panel;
    Register(card);

    StyleClass search;
    search.name = "SearchBar";
    search.parentName = "Button";
    search.background = ColorToken::SearchBoxBackground;
    search.heightToken = MetricToken::SearchBoxHeight;
    Register(search);
}

} // namespace we::runtime::kindui
