// ==============================================================================
// WindEffects — KindUI — ControlChrome
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/ControlChrome.h"

#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Theme/ThemeManager.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Theme/DesignToken.h"
#include "KindUI/Theme/SurfaceRole.h"
#include "KindUI/Theme/TypographySpec.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Host/IconMetrics.h"
#include "KindUI/Theme/DesignSystem.h"

#include <algorithm>
#include <string>
#include <string_view>


namespace we::runtime::kindui {
namespace ControlChrome {
using ::we::runtime::kindui::kWindIconNone;

namespace {

enum class ControlBorderMode {
    None,
    Subtle,
    Styled,
};

struct ControlFrameStyle {
    Color normalBackground = Color::Transparent();
    Color hoverBackground = Color::Transparent();
    Color pressedBackground = Color::Transparent();
    Color disabledBackground = Color::Transparent();
    Color selectedBackground = Color::Transparent();
    float cornerRadius = 0.0f;
};

struct ResolvedControlBorder {
    Color color = Color::Transparent();
    float width = 0.0f;
};

Color ResolveControlBackground(const InteractionState& state, const ControlFrameStyle& style) {
    if (state.disabled) {
        return style.disabledBackground.a > 0.01f ? style.disabledBackground : style.normalBackground;
    }
    if (state.selected && style.selectedBackground.a > 0.01f) {
        return style.selectedBackground;
    }

    constexpr float kHoverMix = 0.62f;
    constexpr float kPressMix = 0.50f;
    Color result = style.normalBackground;
    const float hover = std::clamp(state.hoverAnim, 0.0f, 1.0f);
    const float press = std::clamp(state.pressAnim, 0.0f, 1.0f);
    if (hover > 0.001f && style.hoverBackground.a > 0.01f) {
        result = Color::Pick(result, style.hoverBackground, hover * kHoverMix);
    }
    if (press > 0.001f && style.pressedBackground.a > 0.01f) {
        result = Color::Pick(result, style.pressedBackground, press * kPressMix);
    }
    return result;
}

ResolvedControlBorder ResolveControlBorder(
    const InteractionState& state,
    ControlBorderMode mode,
    const Color& styleBorder = Color::Transparent()) {
    (void)mode;
    (void)styleBorder;
    if (state.focused) {
        const float width = ResolveMetric(MetricToken::FocusRingWidth);
        return { ResolveColor(ColorToken::BorderFocus), width };
    }
    return { Color::Transparent(), 0.0f };
}

void PaintControlFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state,
    const ControlFrameStyle& style,
    ControlBorderMode borderMode,
    const Color& styleBorder = Color::Transparent()) {
    if (rect.width <= 0.0f || rect.height <= 0.0f) {
        return;
    }

    Color bg = ResolveControlBackground(state, style);
    const bool bgOpaque = ColorSpace::IsOpaqueAuthoring(bg);
    // Bake ShadowOverlay press darkening into the fill when the face is opaque so we avoid a
    // second full-rect alpha pass with identical Src-over results.
    if (state.pressAnim > 0.01f && bgOpaque) {
        Color pressShadow = ResolveColor(ColorToken::ShadowOverlay);
        pressShadow.a *= state.pressAnim;
        bg = ColorSpace::CompositeSrcOverOpaque(bg, pressShadow);
    }
    context.DrawRoundedRect(rect, bg, style.cornerRadius);

    const ResolvedControlBorder border = ResolveControlBorder(state, borderMode, styleBorder);
    Color borderCol = border.color.a > 0.01f ? border.color : (styleBorder.a > 0.01f ? styleBorder :
        ResolveColor(ColorToken::Separator));
    const float borderW = border.width > 0.0f ? border.width : ResolveMetric(MetricToken::BorderWidth);
    context.DrawRoundedRectOutline(rect, borderCol, borderW, style.cornerRadius);

    if (state.pressAnim > 0.01f && !bgOpaque) {
        Color pressShadow = ResolveColor(ColorToken::ShadowOverlay);
        pressShadow.a *= state.pressAnim;
        context.DrawRoundedRect(rect, pressShadow, style.cornerRadius);
    }
}

float EdgeWidthPx() {
    return std::max(1.0f, ResolveMetric(MetricToken::BorderWidth));
}

}

void PaintSubtleDropShadow(PaintContext& context, const Rect& rect, float radius, float strength) {
    if (strength <= 0.01f) {
        return;
    }
    Color shadow = ResolveColor(ColorToken::ShadowSubtle);
    shadow.a *= 0.22f * strength;
    const float blur = ResolveMetric(MetricToken::ShadowBlurSmall) * 0.65f;
    Rect shadowRect = rect;
    shadowRect.y += EdgeWidthPx();
    context.DrawShadow(shadowRect, shadow, radius, blur);
}

RecessedBorderColors ResolveRecessedBorderColors(float strength) {
    RecessedBorderColors out{};
    const float s = std::clamp(strength, 0.0f, 1.0f);
    const Color panel = ColorSpace::OpaqueSurface(
        ResolveColor(ColorToken::PanelBackground));

    // Soft dark rims only — never light edges. soft = same dark family, lower alpha.
    out.dark = ColorSpace::CompositeSrcOverOpaque(
        panel,
        ResolveColor(ColorToken::InputInsetOuter));
    out.dark = ColorSpace::OpaqueSurface(
        ColorSpace::LerpColor(
            out.dark,
            ResolveColor(ColorToken::Separator),
            0.45f * s));
    out.dark.a = 0.62f + 0.32f * s;

    out.soft = ColorSpace::CompositeSrcOverOpaque(
        panel,
        ResolveColor(ColorToken::InputInsetInner));
    out.soft = ColorSpace::OpaqueSurface(
        ColorSpace::LerpColor(
            out.soft,
            ResolveColor(ColorToken::Separator),
            0.28f * s));
    out.soft.a = 0.42f + 0.30f * s;

    return out;
}

float RecessedBorderThicknessPx() {
    // Single pixel — subtle, blendable, not a heavy frame.
    return EdgeWidthPx();
}

void PaintRecessedBorder(
    PaintContext& context,
    const Rect& rect,
    float radius,
    float strength,
    float thickness)
{
    if (strength <= 0.01f || rect.width < 2.0f || rect.height < 2.0f) {
        return;
    }

    const float edge = thickness > 0.5f ? thickness : RecessedBorderThicknessPx();
    const RecessedBorderColors colors = ResolveRecessedBorderColors(strength);
    const float s = std::clamp(strength, 0.0f, 1.0f);

    if (radius > 0.5f) {
        // Rounded wells: clean control outline using exact inset rounded ring SDF geometry.
        context.DrawControlOutline(rect, colors.dark, edge, radius);
        return;
    }

    // Square panels: dark top/left, softer bottom/right.
    const float x0 = IconMetrics::SnapPx(rect.x);
    const float y0 = IconMetrics::SnapPx(rect.y);
    float x1 = IconMetrics::SnapPx(rect.x + rect.width);
    float y1 = IconMetrics::SnapPx(rect.y + rect.height);
    if (x1 < x0 + edge * 2.0f) {
        x1 = x0 + edge * 2.0f;
    }
    if (y1 < y0 + edge * 2.0f) {
        y1 = y0 + edge * 2.0f;
    }

    const float w = x1 - x0;
    const float h = y1 - y0;
    const float innerH = std::max(0.0f, h - edge * 2.0f);

    context.DrawRect(Rect{ x0, y0, w, edge }, colors.dark);
    if (innerH > 0.0f) {
        context.DrawRect(Rect{ x0, y0 + edge, edge, innerH }, colors.dark);
    }
    context.DrawRect(Rect{ x0, y1 - edge, w, edge }, colors.soft);
    context.DrawRect(Rect{ x1 - edge, y0, edge, h }, colors.soft);
}

void PaintRaisedBevel(PaintContext& context, const Rect& rect, float radius, float strength) {
    if (strength <= 0.01f || rect.width <= 0.0f || rect.height <= 0.0f) {
        return;
    }
    const float trim = radius;
    const float lineW = std::max(0.0f, rect.width - trim * 2.0f);
    if (lineW > 0.0f) {
        Color topHi = ResolveColor(ColorToken::ButtonBevelHighlight);
        topHi.a *= strength;
        context.DrawRect(Rect{ rect.x + trim, rect.y + 1.0f, lineW, 1.0f }, topHi);
    }
}

void PaintSlateButtonBevel(PaintContext& context, const Rect& rect, float strength) {
    PaintRaisedBevel(context, rect, 3.0f, strength);
}

void PaintInsetBevel(PaintContext& context, const Rect& rect, float radius, float strength) {
    if (strength <= 0.01f) {
        return;
    }
    const float w = EdgeWidthPx();
    const float inset = std::max(1.0f, w);
    const float edgeTrim = std::min(radius * 0.42f, std::max(0.0f, rect.width * 0.22f));
    const float lineW = std::max(0.0f, rect.width - edgeTrim * 2.0f);
    if (lineW <= 0.0f) {
        return;
    }

    // Top inner shade — pushes the well down without changing fill/outline colors.
    Color shade = ResolveColor(ColorToken::InputInsetOuter);
    shade.a *= strength;
    context.DrawRect(
        Rect{ rect.x + edgeTrim, rect.y + inset, lineW, w },
        shade);
}

void PaintSubtleBorderDepth(PaintContext& context, const Rect& rect, float radius, float strength) {
    PaintRecessedBorder(context, rect, radius, strength, RecessedBorderThicknessPx());
}

void PaintWellDepth(PaintContext& context, const Rect& rect, float radius, float strength) {
    if (strength <= 0.01f) {
        return;
    }
    // Same recipe as Inspector input wells — dark recessed rim + top inset shade.
    const float s = std::clamp(strength, 0.0f, 1.0f);
    PaintRecessedBorder(context, rect, radius, 0.92f * s, RecessedBorderThicknessPx());
    PaintInsetBevel(context, rect, radius, 0.85f * s);
}

void PaintPanelButtonFace(
    PaintContext& context,
    const Rect& rect,
    const Color& background,
    float radius,
    float hoverAnim,
    float pressAnim,
    bool emphasized) {
    (void)emphasized;

    Color bgIdle = background.a > 0.01f ? background : ResolveColor(ColorToken::ControlBackground);
    Color bgHover = ResolveColor(ColorToken::ControlBackgroundHover);
    Color bgPress = ResolveColor(ColorToken::PressedBackground);

    Color bgColor = bgIdle;
    if (hoverAnim > 0.001f) {
        bgColor = Color::Pick(bgColor, bgHover, std::clamp(hoverAnim, 0.0f, 1.0f));
    }
    if (pressAnim > 0.001f) {
        bgColor = Color::Pick(bgColor, bgPress, std::clamp(pressAnim, 0.0f, 1.0f));
    }

    context.DrawRoundedRect(rect, bgColor, radius);
    PaintRecessedBorder(
        context,
        rect,
        radius,
        pressAnim > 0.001f ? 0.45f : 0.60f,
        RecessedBorderThicknessPx());
}

namespace {

ControlFrameStyle InputFrameStyle() {
    const ResolvedStyle base = Role(StyleRole::Input);
    ControlFrameStyle style;
    style.normalBackground = base.background;
    style.hoverBackground = ResolveColor(ColorToken::ControlBackgroundHover);
    style.disabledBackground = ResolveColor(ColorToken::DisabledBackground);
    style.cornerRadius = base.cornerRadius > 0.0f
        ? base.cornerRadius
        : ResolveMetric(MetricToken::CornerRadiusSmall);
    return style;
}

float SearchInputCornerRadius(const Rect& rect) {
    return rect.height * 0.5f;
}

void PaintInputFrameInternal(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state,
    float cornerRadius) {
    context.PushSurfaceOwner("Input", SurfaceRole::Input);

    SurfaceRole fillRole = SurfaceRole::Input;
    if (state.disabled) {
        fillRole = SurfaceRole::Disabled;
    }
    context.DrawSurface(rect, fillRole, cornerRadius, "Input");

    if (state.focused) {
        PaintFocusRing(context, rect, cornerRadius);
    } else {
        PaintSoftInputBorder(context, rect, cornerRadius, state);
    }

    context.PopSurfaceOwner();
}

}

ResolvedStyle Role(StyleRole role) {
    return ThemeManager::Get().Resolve(role);
}

float HoverDamping() {
    return ResolveMetric(MetricToken::HoverAnimationDamping);
}

float PressDamping() {
    return ResolveMetric(MetricToken::PressAnimationDamping);
}

void PaintElevation(PaintContext& context, const Rect& rect, int elevation, float radius) {
    if (elevation <= 0) {
        return;
    }
    Color shadow = ResolveColor(elevation >= 3 ? ColorToken::ShadowOverlay
        : elevation >= 2 ? ColorToken::ShadowPopup
        : ColorToken::ShadowSubtle);
    const float blur = elevation >= 2
        ? ResolveMetric(MetricToken::ShadowBlurMedium)
        : ResolveMetric(MetricToken::ShadowBlurSmall);
    context.DrawShadow(rect, shadow, radius, blur);
}

void PaintPopupShadow(PaintContext& context, const Rect& rect, float radius) {
    PaintElevation(context, rect, 2, radius);
}

void PaintInteractiveFill(
    PaintContext& context,
    const Rect& rect,
    float cornerRadius,
    float hoverAnim,
    float pressAnim,
    bool selected,
    SurfaceRole surfaceRole)
{
    const Color fill = ResolveInteractiveSurfaceColor(surfaceRole, hoverAnim, pressAnim, selected);
    if (fill.a > 0.001f) {
        context.DrawRoundedRect(rect, fill, cornerRadius);
    }
}

void PaintInteractiveFill(
    PaintContext& context,
    const Rect& rect,
    float cornerRadius,
    float hoverAnim,
    float pressAnim,
    bool selected,
    ColorToken surfaceToken)
{
    PaintInteractiveFill(
        context,
        rect,
        cornerRadius,
        hoverAnim,
        pressAnim,
        selected,
        SurfaceRoleFromColorToken(surfaceToken));
}

void PaintFocusRing(PaintContext& context, const Rect& rect, float radius) {
    const float width = std::max(1.0f, ResolveMetric(MetricToken::FocusRingWidth));
    context.DrawControlOutline(rect, ResolveColor(ColorToken::BorderFocus), width, radius);
}

void PaintFilledButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state,
    StyleRole hoverRole,
    StyleRole pressRole) {
    if (state.disabled) {
        context.DrawRoundedRect(rect, ResolveColor(ColorToken::DisabledBackground), base.cornerRadius);
        return;
    }

    Color bgIdle = base.background.a > 0.01f ? base.background : ResolveColor(ColorToken::ControlBackground);
    Color bgHover = (hoverRole == StyleRole::ButtonPrimary)
        ? ResolveColor(ColorToken::ButtonPrimaryHover)
        : ResolveColor(ColorToken::ControlBackgroundHover);
    Color bgPress = (pressRole == StyleRole::ButtonPrimary)
        ? ResolveColor(ColorToken::ButtonPrimaryPressed)
        : ResolveColor(ColorToken::PressedBackground);

    Color bgColor = bgIdle;
    if (state.hoverAnim > 0.001f) {
        bgColor = Color::Pick(bgColor, bgHover, std::clamp(state.hoverAnim, 0.0f, 1.0f));
    }
    if (state.pressAnim > 0.001f) {
        bgColor = Color::Pick(bgColor, bgPress, std::clamp(state.pressAnim, 0.0f, 1.0f));
    }

    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float radius = base.cornerRadius > 0.0f ? base.cornerRadius : ResolveMetric(MetricToken::CornerRadiusSmall) * scale;
    context.DrawRoundedRect(rect, bgColor, radius);

    PaintRecessedBorder(
        context,
        rect,
        radius,
        state.pressAnim > 0.001f ? 0.45f : 0.60f,
        RecessedBorderThicknessPx());
}

void PaintGhostButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state) {
    if (state.disabled) {
        return;
    }
    const Color fill = ResolveInteractiveBackground(
        state.hoverAnim,
        state.pressAnim,
        state.selected,
        ColorToken::PanelBackground);
    if (fill.a > 0.001f) {
        context.DrawRoundedRect(rect, fill, base.cornerRadius);
    }
    if (state.focused) {
        PaintFocusRing(context, rect, base.cornerRadius);
    }
}

void PaintIconButtonFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state,
    bool active) {
    ResolvedStyle base = Role(active ? StyleRole::IconButtonPressed : StyleRole::IconButton);
    PaintFilledButton(context, rect, base, state, StyleRole::IconButtonHover, StyleRole::IconButtonPressed);
}

void PaintBorderlessIconButton(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    (void)context;
    (void)rect;
    (void)state;
}

void PaintSoftInputBorder(
    PaintContext& context,
    const Rect& rect,
    float radius,
    const InteractionState& state)
{
    if (rect.width < 2.0f || rect.height < 2.0f) {
        return;
    }

    // Crisp 1px hairline — solid edges, no SDF outline glow/depth.
    Color border = ResolveColor(ColorToken::InputOutline);
    if (state.disabled) {
        border.a *= 0.55f;
    }

    if (radius > 1.0f) {
        context.DrawControlOutline(rect, border, 1.0f, radius);
        return;
    }

    const float t = 1.0f;
    const float x0 = IconMetrics::SnapPx(rect.x);
    const float y0 = IconMetrics::SnapPx(rect.y);
    const float x1 = IconMetrics::SnapPx(rect.x + rect.width);
    const float y1 = IconMetrics::SnapPx(rect.y + rect.height);
    const float w = std::max(t, x1 - x0);
    const float h = std::max(t, y1 - y0);

    context.DrawRect(Rect{ x0, y0, w, t }, border);
    context.DrawRect(Rect{ x0, y1 - t, w, t }, border);
    context.DrawRect(Rect{ x0, y0, t, h }, border);
    context.DrawRect(Rect{ x1 - t, y0, t, h }, border);
}

void PaintInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    PaintInputFrameInternal(context, rect, state, InputFrameStyle().cornerRadius);
}

void PaintStatusBarCommandField(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    SurfaceRole fillRole = SurfaceRole::Input;
    if (state.disabled) {
        fillRole = SurfaceRole::Disabled;
    }
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall);
    context.DrawSurface(rect, fillRole, radius, "StatusBarCommandField");
    if (state.focused) {
        PaintFocusRing(context, rect, radius);
    } else {
        PaintSoftInputBorder(context, rect, radius, state);
    }
}

void PaintSearchInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    PaintInputFrameInternal(context, rect, state, SearchInputCornerRadius(rect));
}

void PaintSearchField(
    PaintContext& context,
    const Rect& rect,
    const std::string& placeholder,
    const std::string& text,
    const InteractionState& state,
    bool showCaret,
    const SearchFieldPaintOptions& options) {
    if (!options.toolbarFlat) {
        PaintSearchInputFrame(context, rect, state);
    }

    const float padH = options.toolbarFlat
        ? LayoutMetrics::SearchInputPaddingH() * 0.5f
        : LayoutMetrics::SearchInputPaddingH();
    const uint32_t glyphPx = WindIcons::Search16.sizePx;
    const float iconPx = static_cast<float>(glyphPx);
    const float fontSize = LayoutMetrics::SearchInputFontSize();
    const float iconGap = ResolveMetric(MetricToken::Space1);

    // Search glyph stays pinned to the left padding — never scrolls with typed text.
    const float iconX = rect.x + padH;
    const Rect iconBand{ iconX, rect.y, iconPx, rect.height };
    IconPainter::Draw(context, WindIcons::Search16, iconBand, glyphPx);

    const float textX = iconX + iconPx + iconGap;
    const float clearReserve = (options.showClearButton && !text.empty())
        ? (iconPx + padH)
        : padH;
    const float textMaxW = std::max(0.0f, (rect.x + rect.width - clearReserve) - textX);
    const float textY = LayoutMetrics::AlignTextTopY(rect, fontSize);
    const bool empty = text.empty();

    if (empty) {
        context.DrawText(
            placeholder,
            Point{ textX, textY },
            ResolveColor(ColorToken::TextSecondary),
            fontSize);
    } else {
        context.PushClipRect(Rect{ textX, rect.y, textMaxW, rect.height });
        context.DrawText(text, Point{ textX, textY }, ResolveColor(ColorToken::TextPrimary), fontSize);
        if (state.focused && showCaret) {
            const float caretX = textX + context.GetTextWidth(text, fontSize);
            context.DrawRect(
                Rect{ caretX, textY, ResolveMetric(MetricToken::BorderWidth), LayoutMetrics::TextLineHeight(fontSize) },
                ResolveColor(ColorToken::TextPrimary));
        }
        context.PopClipRect();
    }

    if (options.showClearButton && !empty) {
        const float clearSize = iconPx;
        const float clearX = rect.x + rect.width - clearSize - padH;
        Rect clearBand{ clearX, rect.y, clearSize, rect.height };
        if (options.clearHovered) {
            const float radius = ResolveMetric(MetricToken::IconButtonRadius);
            const Color fill = ResolveInteractiveBackground(
                1.0f, 0.0f, false, ColorToken::PanelBackground);
            if (fill.a > 0.001f) {
                context.DrawRoundedRect(
                    Rect{ clearX, rect.y + (rect.height - clearSize) * 0.5f, clearSize, clearSize },
                    fill,
                    radius);
            }
        }
        IconPainter::Draw(
            context,
            WindIcons::X16,
            clearBand,
            static_cast<uint32_t>(clearSize));
    }
}

void PaintListRow(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    PaintListRow(context, rect, state, ColorToken::PanelBackground);
}

void PaintListRow(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state,
    ColorToken underlayToken) {
    const ResolvedStyle base = Role(StyleRole::TableRow);
    const Color underlay = ColorSpace::OpaqueSurface(ResolveColor(underlayToken));

    const Color hoverColor = ResolveColor(ColorToken::HoverBackground);
    const Color targetBg = state.selected ? ResolveColor(ColorToken::SelectedBackground) : hoverColor;
    Color bg = Color::Transparent();
    if (state.selected) {
        bg = targetBg;
    } else if (state.hoverAnim > 0.001f) {
        bg = Color::Pick(underlay, targetBg, std::clamp(state.hoverAnim, 0.0f, 1.0f));
    }

    const float radius = base.cornerRadius > 0.0f ? base.cornerRadius : 3.0f;
    if (bg.a > 0.001f) {
        context.DrawRoundedRect(rect, bg, radius);
    }

    const ResolvedControlBorder border = ResolveControlBorder(
        state,
        state.selected ? ControlBorderMode::Subtle : ControlBorderMode::None,
        base.border);
    if (border.color.a > 0.01f && border.width > 0.0f) {
        context.DrawRoundedRectOutline(rect, border.color, border.width, radius);
    }
}

void PaintCard(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    const ResolvedStyle base = Role(StyleRole::Card);
    const Color fill = MixInteractiveSurface(
        base.background,
        state.hoverAnim,
        state.pressAnim,
        state.selected,
        state.disabled);
    context.DrawRoundedRect(rect, fill, base.cornerRadius);
}

void PaintCenteredLabel(
    PaintContext& context,
    const Rect& rect,
    const std::string& text,
    const Color& color,
    float fontSize,
    bool bold) {
    const float textW = context.GetTextWidth(text, fontSize, bold);
    context.DrawText(
        text,
        Point{
            rect.x + (rect.width - textW) * 0.5f,
            LayoutMetrics::AlignTextTopY(rect, fontSize)
        },
        color,
        fontSize,
        bold);
}

void PaintSectionHeader(
    PaintContext& context,
    const Rect& rect,
    const std::string& title,
    const std::string& subtitle) {
    const TypographySpec titleSpec = ResolveTypography(TypographyToken::SectionTitle);
    const TypographySpec subtitleSpec = ResolveTypography(TypographyToken::Subtitle);
    float y = rect.y;
    context.DrawText(
        title,
        Point{ rect.x, y },
        titleSpec.color,
        titleSpec.sizePx,
        titleSpec.bold);
    if (!subtitle.empty()) {
        y += titleSpec.lineHeightPx;
        context.DrawText(
            subtitle,
            Point{ rect.x, y },
            subtitleSpec.color,
            subtitleSpec.sizePx,
            subtitleSpec.bold);
    }
}

void PaintDangerButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state) {
    if (state.disabled) {
        context.DrawRoundedRect(rect, ResolveColor(ColorToken::DisabledBackground), base.cornerRadius);
        return;
    }

    PaintElevation(context, rect, base.elevation, base.cornerRadius);

    ControlFrameStyle frame;
    frame.normalBackground = base.background;
    frame.hoverBackground = ResolveColor(ColorToken::ButtonDangerHover);
    frame.pressedBackground = ResolveColor(ColorToken::ButtonDangerPressed);
    frame.cornerRadius = base.cornerRadius;
    const float bevelStrength = std::max(0.0f, 0.60f - state.pressAnim * 0.25f);
    PaintControlFrame(context, rect, state, frame, ControlBorderMode::None, base.border);
    PaintRecessedBorder(context, rect, frame.cornerRadius, bevelStrength);
}

void PaintPopupSurface(PaintContext& context, const Rect& rect) {
    const ResolvedStyle style = Role(StyleRole::Popup);
    PaintElevation(context, rect, style.elevation > 0 ? style.elevation : 2, style.cornerRadius);
    context.DrawRoundedRect(rect, style.background, style.cornerRadius);
    const Color borderCol = style.border.a > 0.01f ? style.border : ResolveColor(ColorToken::BorderLight);
    context.DrawRoundedRectOutline(rect, borderCol, 1.0f, style.cornerRadius);
}

void PaintTooltipSurface(PaintContext& context, const Rect& rect) {
    const ResolvedStyle style = Role(StyleRole::Tooltip);
    PaintElevation(context, rect, style.elevation > 0 ? style.elevation : 2, style.cornerRadius);
    context.DrawRoundedRect(rect, style.background, style.cornerRadius);
    context.DrawRoundedRectOutline(rect, ResolveColor(ColorToken::BorderLight), 1.0f, style.cornerRadius);
}

void PaintCheckbox(
    PaintContext& context,
    const Rect& box,
    bool checked,
    const InteractionState& state) {
    const ResolvedStyle style = Role(StyleRole::Checkbox);
    const float checkSize = (std::min)(box.width, box.height);
    const float centerY = box.y + box.height * 0.5f;
    const Rect drawBox{
        box.x,
        centerY - checkSize * 0.5f,
        checkSize,
        checkSize
    };

    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float radius = (style.cornerRadius > 0.0f) ? style.cornerRadius : (3.0f * scale);

    Color bg = MixInteractiveSurface(
        style.background,
        state.hoverAnim,
        0.0f,
        false,
        state.disabled);
    context.DrawRoundedRect(drawBox, bg, radius);

    const Color borderCol = (state.hoverAnim > 0.01f)
        ? ResolveColor(ColorToken::BorderFocus)
        : ResolveColor(ColorToken::InputOutline);
    context.DrawRoundedRectOutline(drawBox, borderCol, 1.0f, radius);

    if (checked) {
        IconPainter::Draw(
            context,
            WindIcons::Check16,
            drawBox,
            ResolveColor(ColorToken::AccentPrimary));
    }
}

void PaintAxisIndicator(
    PaintContext& context,
    const Rect& fieldRect,
    int axisIndex,
    const InteractionState& state) {
    (void)state;
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float inset = 2.0f * scale;
    const float accentW = 2.0f * scale;
    const Rect accent{
        fieldRect.x + inset,
        fieldRect.y + inset,
        accentW,
        (std::max)(0.0f, fieldRect.height - inset * 2.0f)
    };
    Color color;
    switch (axisIndex) {
        case 0: color = ResolveColor(ColorToken::AxisX); break;
        case 1: color = ResolveColor(ColorToken::AxisY); break;
        case 2: color = ResolveColor(ColorToken::AxisZ); break;
        default: color = ResolveColor(ColorToken::TextSecondary); break;
    }
    context.DrawRoundedRect(accent, color, 1.0f);
}

void PaintPanelTab(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    const InteractionState& state) {
    const float radius = 0.0f;
    if (state.selected) {
        context.DrawRoundedRect(
            bounds,
            ResolveSurfaceColor(SurfaceRole::TabActive),
            radius);
    } else {
        const Color tabBg = ResolveInteractiveBackground(
            0.0f,
            state.pressAnim,
            false,
            ColorToken::TabBackground);
        if (tabBg.a > 0.01f) {
            PaintPanelButtonFace(
                context, bounds, tabBg, radius, 0.0f, state.pressAnim, false);
        }
    }

    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption);
    const Color textColor = state.selected
        ? ResolveColor(ColorToken::TextPrimary)
        : ResolveColor(ColorToken::TextSecondary);
    context.DrawText(
        label,
        Point{
            bounds.x + ResolveMetric(MetricToken::Space3),
            LayoutMetrics::AlignTextTopY(bounds, fontSize)
        },
        textColor,
        fontSize,
        false);
}

void PaintVerticalSeparator(
    PaintContext& context,
    float x,
    float top,
    float bottom,
    float thickness,
    ColorToken colorToken)
{
    const float height = bottom - top;
    if (height <= 0.0f) {
        return;
    }

    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float lineWidth = (std::max)(1.0f, IconMetrics::SnapPx(
        (thickness > 0.0f ? thickness : ResolveMetric(MetricToken::BorderWidth)) * uiScale));
    const float snappedX = IconMetrics::SnapPx(x - lineWidth * 0.5f);
    context.DrawRect(Rect{ snappedX, top, lineWidth, height }, ResolveColor(colorToken));
}

}
}
// Toolchain optimization touch

