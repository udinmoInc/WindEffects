#include "KindUI/StylePipeline/StylePipeline.h"

#include "KindUI/Theming/StyleClass.h"

namespace we::runtime::kindui {
namespace {

Color ResolveColorProp(const ColorProperty& prop, const IKindUITheme& theme) {
    if (prop.HasOverride()) {
        return *prop.override;
    }
    return theme.ResolveColor(prop.token);
}

float ResolveSpacingProp(const SpacingProperty& prop, const IKindUITheme& theme, float dpiScale) {
    if (prop.HasOverride()) {
        return *prop.override * dpiScale;
    }
    return theme.ResolveSpacing(prop.token) * dpiScale;
}

float ResolveRadiusProp(const RadiusProperty& prop, const IKindUITheme& theme, float dpiScale) {
    if (prop.HasOverride()) {
        return *prop.override * dpiScale;
    }
    return theme.ResolveRadius(prop.token) * dpiScale;
}

Margin ResolvePadding(const PaddingStyle& padding, const IKindUITheme& theme, float dpiScale) {
    return {
        ResolveSpacingProp(padding.left, theme, dpiScale),
        ResolveSpacingProp(padding.top, theme, dpiScale),
        ResolveSpacingProp(padding.right, theme, dpiScale),
        ResolveSpacingProp(padding.bottom, theme, dpiScale),
    };
}

ColorProperty PickColorProperty(const ColorProperty& base, const ColorProperty& over) {
    return over.HasOverride() || over.token != ColorToken::ControlBackground ? over : base;
}

ButtonStyle MergeColorLayer(const ButtonStyle& base, const ButtonStyle& over) {
    ButtonStyle result = base;
    result.background = PickColorProperty(base.background, over.background);
    result.backgroundHover = PickColorProperty(base.backgroundHover, over.backgroundHover);
    result.backgroundPressed = PickColorProperty(base.backgroundPressed, over.backgroundPressed);
    result.backgroundDisabled = PickColorProperty(base.backgroundDisabled, over.backgroundDisabled);
    result.foreground = PickColorProperty(base.foreground, over.foreground);
    result.foregroundDisabled = PickColorProperty(base.foregroundDisabled, over.foregroundDisabled);
    if (over.cornerRadius.HasOverride() || over.cornerRadius.token != base.cornerRadius.token) {
        result.cornerRadius = over.cornerRadius;
    }
    if (over.elevation.HasOverride() || over.elevation.token != base.elevation.token) {
        result.elevation = over.elevation;
    }
    if (over.transition.HasOverride() || over.transition.token != base.transition.token) {
        result.transition = over.transition;
    }
    return result;
}

ButtonStyle VariantButtonStyle(WidgetVariant variant) {
    ButtonStyle style{};
    switch (variant) {
    case WidgetVariant::Primary:
        style.background = ColorProperty{ColorToken::AccentSurface};
        style.backgroundHover = ColorProperty{ColorToken::ControlBackgroundHover};
        style.foreground = ColorProperty{ColorToken::TextOnAccent};
        break;
    case WidgetVariant::Secondary:
        style.background = ColorProperty{ColorToken::SecondarySurface};
        break;
    case WidgetVariant::Ghost:
    case WidgetVariant::Flat:
        style.background = ColorProperty{ColorToken::PrimarySurface, Color::Transparent()};
        style.border = BorderStyleTokens{};
        break;
    case WidgetVariant::Link:
        style.background = ColorProperty{ColorToken::PrimarySurface, Color::Transparent()};
        style.foreground = ColorProperty{ColorToken::TextLink};
        break;
    case WidgetVariant::Success:
        style.background = ColorProperty{ColorToken::SuccessColor};
        style.foreground = ColorProperty{ColorToken::TextOnAccent};
        break;
    case WidgetVariant::Warning:
        style.background = ColorProperty{ColorToken::WarningColor};
        style.foreground = ColorProperty{ColorToken::TextOnAccent};
        break;
    case WidgetVariant::Danger:
        style.background = ColorProperty{ColorToken::ErrorColor};
        style.foreground = ColorProperty{ColorToken::TextOnAccent};
        break;
    case WidgetVariant::Outline:
        style.background = ColorProperty{ColorToken::PrimarySurface, Color::Transparent()};
        style.border.color = ColorProperty{ColorToken::BorderDefault};
        break;
    case WidgetVariant::Toolbar:
        style.background = ColorProperty{ColorToken::TertiarySurface};
        style.cornerRadius = RadiusProperty{RadiusToken::Small};
        break;
    case WidgetVariant::Accent:
        style.background = ColorProperty{ColorToken::AccentSurface};
        break;
    default:
        break;
    }
    return style;
}

ButtonStyle StyleClassToButtonStyle(std::string_view /*className*/) {
    // Style class layer bridges through legacy StyleClassRegistry + StyleResolve
    // until ThemeToken is fully migrated to ColorToken.
    return {};
}

} // namespace

ButtonStyle StylePipeline::MergeButtonStyles(const ButtonStyle& base, const ButtonStyle& over) {
    return MergeColorLayer(base, over);
}

ButtonStyle StylePipeline::ResolveButtonStyle(const StyleContext& ctx) {
    ButtonStyle resolved{};

    if (ctx.frameworkDefault) {
        resolved = MergeButtonStyles(resolved, *ctx.frameworkDefault);
    }
    if (ctx.applicationDefault) {
        resolved = MergeButtonStyles(resolved, *ctx.applicationDefault);
    }

    // Theme layer: variant overrides are applied here.
    resolved = MergeButtonStyles(resolved, VariantButtonStyle(ctx.variant));

    if (!ctx.styleClass.empty()) {
        resolved = MergeButtonStyles(resolved, StyleClassToButtonStyle(ctx.styleClass));
    }

    if (ctx.localOverride) {
        resolved = MergeButtonStyles(resolved, *ctx.localOverride);
    }

    return resolved;
}

ResolvedVisualStyle StylePipeline::ResolveButtonVisual(
    const ButtonStyle& style,
    const StyleContext& ctx)
{
    if (!ctx.theme) {
        return {};
    }

    const IKindUITheme& theme = *ctx.theme;
    const float dpi = ctx.dpiScale;

    ResolvedVisualStyle visual{};
    visual.background = ResolveColorProp(style.background, theme);
    visual.foreground = ResolveColorProp(style.foreground, theme);
    visual.border = ResolveColorProp(style.border.color, theme);
    visual.cornerRadius = ResolveRadiusProp(style.cornerRadius, theme, dpi);
    visual.borderWidth = ResolveSpacingProp(style.border.width, theme, dpi);
    visual.padding = ResolvePadding(style.padding, theme, dpi);
    visual.fontSize = style.typography.token.HasOverride()
        ? *style.typography.token.override * dpi
        : theme.ResolveFontSize(style.typography.token.token) * dpi;
    visual.height = ResolveSpacingProp(style.minHeight, theme, dpi);
    visual.elevation = style.elevation.HasOverride()
        ? *style.elevation.override
        : theme.ResolveElevation(style.elevation.token);
    visual.bold = style.typography.bold.value_or(false);

    return ApplyInteractionState(visual, style, ctx);
}

ResolvedVisualStyle StylePipeline::ApplyInteractionState(
    const ResolvedVisualStyle& base,
    const ButtonStyle& styleDef,
    const StyleContext& ctx)
{
    if (!ctx.theme) {
        return base;
    }

    ResolvedVisualStyle visual = base;
    const IKindUITheme& theme = *ctx.theme;

    if (ctx.interaction.IsDisabled()) {
        visual.background = ResolveColorProp(styleDef.backgroundDisabled, theme);
        visual.foreground = ResolveColorProp(styleDef.foregroundDisabled, theme);
        visual.opacity = 0.5f;
        return visual;
    }
    if (ctx.interaction.IsPressed()) {
        visual.background = ResolveColorProp(styleDef.backgroundPressed, theme);
    } else if (ctx.interaction.IsHovered()) {
        visual.background = ResolveColorProp(styleDef.backgroundHover, theme);
    }
    if (ctx.interaction.IsSelected()) {
        visual.background = theme.ResolveColor(ColorToken::ControlBackgroundSelected);
    }
    if (HasState(ctx.interaction.flags, InteractionState::Loading)) {
        visual.opacity = 0.7f;
    }
    if (HasState(ctx.interaction.flags, InteractionState::Error)) {
        visual.border = theme.ResolveColor(ColorToken::BorderError);
    }

    return visual;
}

} // namespace we::runtime::kindui
