// ==============================================================================
// WindEffects — KindUI — StyleClass
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Theming/StyleClass.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Theme/ThemeAccess.h"

namespace we::runtime::kindui {
namespace {

bool g_DefaultsRegistered = false;

float ScaleMetric(float value, float dpiScale) {
    return value * dpiScale;
}

Margin ScalePadding(const Margin& padding, float dpiScale) {
    return Margin{
        ScaleMetric(padding.left, dpiScale),
        ScaleMetric(padding.top, dpiScale),
        ScaleMetric(padding.right, dpiScale),
        ScaleMetric(padding.bottom, dpiScale)
    };
}

ResolvedStyle BuildFromClass(const StyleClass& cls, const IKindUITheme& theme, float dpiScale) {
    using namespace ColorSpace;
    ResolvedStyle style{};
    style.background = OpaqueSurface(ResolveColor(cls.background));
    style.foreground = ResolveColor(cls.foreground);
    style.border = ResolveColor(cls.border);
    style.cornerRadius = ScaleMetric(theme.ResolveMetric(cls.radiusToken), dpiScale);
    style.fontSize = ScaleMetric(theme.ResolveMetric(cls.fontSizeToken), dpiScale);
    style.height = ScaleMetric(theme.ResolveMetric(cls.heightToken), dpiScale);
    style.padding = ScalePadding(theme.ResolvePadding(cls.paddingToken), dpiScale);
    style.bold = cls.bold;
    if (cls.opacity < 0.999f) {
        style.foreground = ResolveColor(ColorToken::TextDisabled);
    }
    return style;
}

} // namespace

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
    if (g_DefaultsRegistered) {
        return;
    }
    g_DefaultsRegistered = true;

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
    button.animDurationToken = MetricToken::Space1;
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
    search.background = ColorToken::InputBackground;
    search.border = ColorToken::BorderSubtle;
    // Pill radius is applied at paint time from control height; keep a medium
    // token here so style-class consumers still prefer a rounded search look.
    search.radiusToken = MetricToken::CornerRadiusMedium;
    search.heightToken = MetricToken::SearchBoxHeight;
    Register(search);
}

ResolvedStyle StyleResolve::FromClass(
    std::string_view className,
    const IKindUITheme& theme,
    float dpiScale)
{
    if (className.empty()) {
        return {};
    }
    const StyleClass resolved = StyleClassRegistry::Get().Resolve(className);
    return BuildFromClass(resolved, theme, dpiScale);
}

ResolvedStyle StyleResolve::ApplyState(
    const ResolvedStyle& base,
    const StyleClass& cls,
    const IKindUITheme& theme,
    float dpiScale,
    bool hovered,
    bool pressed,
    bool disabled,
    bool selected)
{
    (void)selected;
    using namespace ColorSpace;
    ResolvedStyle style = base;
    if (disabled) {
        style.background = OpaqueSurface(ResolveColor(cls.disabledBackground));
        style.foreground = ResolveColor(ColorToken::TextDisabled);
        style.border = ResolveColor(ColorToken::Separator);
        return style;
    }
    if (pressed) {
        style.background = OpaqueSurface(ResolveColor(cls.pressedBackground));
    } else if (hovered) {
        style.background = OpaqueSurface(ResolveColor(cls.hoverBackground));
    }
    style.cornerRadius = ScaleMetric(theme.ResolveMetric(cls.radiusToken), dpiScale);
    return style;
}

ResolvedStyle StyleResolve::ResolveWithState(
    std::string_view className,
    const IKindUITheme& theme,
    float dpiScale,
    bool hovered,
    bool pressed,
    bool disabled,
    bool selected)
{
    if (className.empty()) {
        return {};
    }
    const StyleClass cls = StyleClassRegistry::Get().Resolve(className);
    const ResolvedStyle base = BuildFromClass(cls, theme, dpiScale);
    return ApplyState(base, cls, theme, dpiScale, hovered, pressed, disabled, selected);
}

} // namespace we::runtime::kindui
