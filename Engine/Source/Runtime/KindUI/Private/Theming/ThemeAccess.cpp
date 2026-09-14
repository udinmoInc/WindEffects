// ==============================================================================
// WindEffects — KindUI — ThemeAccess
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Theme/ThemeAccess.h"

#include "KindUI/Core/ColorSpace.h"
#include "Profiling/UiColorDebug.h"
#include "KindUI/Theme/PaletteRuntime.h"
#include "KindUI/Theme/ThemeManager.h"
#include "KindUI/Theme/TypographySpec.h"

#include <algorithm>

namespace we::runtime::kindui {
namespace {

bool IsCompositeColorToken(ColorToken token) {
    switch (token) {
    case ColorToken::TooltipBackground:
    case ColorToken::DragGhostBackground:
    case ColorToken::ActiveTabLine:
    case ColorToken::SelectionHighlight:
    case ColorToken::HighlightSubtle:
    case ColorToken::ModalScrim:
    case ColorToken::ShadowPopup:
    case ColorToken::ShadowSubtle:
    case ColorToken::ShadowOverlay:
    case ColorToken::ShadowColor:
    case ColorToken::ContentBrowserFolderShadow:
    case ColorToken::InputInsetInner:
    case ColorToken::InputInsetOuter:
    case ColorToken::IconContactShadow:
        return true;
    default:
        return false;
    }
}

constexpr float kHoverMix = 0.62f;
constexpr float kPressMix = 0.50f;

float Clamp01(float t) {
    return std::clamp(t, 0.0f, 1.0f);
}

Color MixInteractiveSurfaceImpl(
    Color base,
    float hoverAnim,
    float pressAnim,
    bool selected,
    bool disabled,
    Color opaqueUnderlay)
{
    if (disabled) {
        return ColorSpace::OpaqueSurface(ResolveColor(ColorToken::DisabledBackground));
    }
    if (selected) {
        return ColorSpace::OpaqueSurface(ResolveColor(ColorToken::SelectedBackground));
    }

    const float hover = Clamp01(hoverAnim);
    const float press = Clamp01(pressAnim);
    const bool canBakeOverlay = ColorSpace::IsOpaqueAuthoring(opaqueUnderlay);

    if (base.a < 0.01f) {
        if (press > 0.001f && press >= hover) {
            Color fill = ColorSpace::OpaqueSurface(ResolveColor(ColorToken::PressedBackground));
            fill.a = press * kPressMix;
            return canBakeOverlay ? ColorSpace::CompositeSrcOverOpaque(opaqueUnderlay, fill) : fill;
        }
        if (hover > 0.001f) {
            Color fill = ColorSpace::OpaqueSurface(ResolveColor(ColorToken::HoverBackground));
            fill.a = hover * kHoverMix;
            return canBakeOverlay ? ColorSpace::CompositeSrcOverOpaque(opaqueUnderlay, fill) : fill;
        }
        return Color::Transparent();
    }

    Color result = base;
    if (hover > 0.001f) {
        result = ColorSpace::LerpColor(
            result,
            ColorSpace::OpaqueSurface(ResolveColor(ColorToken::HoverBackground)),
            hover * kHoverMix);
    }
    if (press > 0.001f) {
        result = ColorSpace::LerpColor(
            result,
            ColorSpace::OpaqueSurface(ResolveColor(ColorToken::PressedBackground)),
            press * kPressMix);
    }
    return ColorSpace::OpaqueSurface(result);
}

Color ResolveInteractiveBackgroundImpl(
    float hoverAnim,
    float pressAnim,
    bool selected,
    ColorToken surfaceToken)
{
    if (selected) {
        return ColorSpace::OpaqueSurface(ResolveColor(ColorToken::SelectedBackground));
    }
    if (Clamp01(hoverAnim) < 0.001f && Clamp01(pressAnim) < 0.001f) {
        return Color::Transparent();
    }
    const Color base = ColorSpace::OpaqueSurface(ResolveColor(surfaceToken));
    return MixInteractiveSurfaceImpl(base, hoverAnim, pressAnim, false, false, Color::Transparent());
}

}

#include <array>
#include <atomic>
#include <mutex>

namespace {
std::atomic<uint64_t> g_GlobalThemeVersion{1};

constexpr size_t kMaxColorTokens = 256;
constexpr size_t kMaxMetricTokens = 256;
constexpr size_t kMaxPaddingTokens = 32;
constexpr size_t kMaxSpacingTokens = 16;
constexpr size_t kMaxRadiusTokens = 16;
constexpr size_t kMaxTypographyTokens = 64;

struct GlobalThemeTokenCache {
    uint64_t version{0};
    std::array<Color, kMaxColorTokens> colors{};
    std::array<float, kMaxMetricTokens> metrics{};
    std::array<Margin, kMaxPaddingTokens> paddings{};
    std::array<float, kMaxSpacingTokens> spacings{};
    std::array<float, kMaxRadiusTokens> radii{};
    std::array<float, kMaxTypographyTokens> fontSizes{};
    bool initialized{false};
};

GlobalThemeTokenCache g_ThemeCache{};
std::mutex g_ThemeCacheMutex;

void RebuildTokenCacheLocked(uint64_t targetVersion) {
    auto& theme = ThemeManager::Get().Theme();
    for (uint32_t i = 0; i < kMaxColorTokens; ++i) {
        ColorToken tok = static_cast<ColorToken>(i);
        Color resolved = theme.ResolveColor(tok);
        if (!IsCompositeColorToken(tok)) {
            resolved = ColorSpace::OpaqueSurface(resolved);
        }
        g_ThemeCache.colors[i] = resolved;
    }
    for (uint32_t i = 0; i < kMaxMetricTokens; ++i) {
        MetricToken tok = static_cast<MetricToken>(i);
        g_ThemeCache.metrics[i] = theme.ResolveMetric(tok);
    }
    for (uint32_t i = 0; i < kMaxPaddingTokens; ++i) {
        PaddingToken tok = static_cast<PaddingToken>(i);
        g_ThemeCache.paddings[i] = theme.ResolvePadding(tok);
    }
    for (uint32_t i = 0; i < kMaxSpacingTokens; ++i) {
        SpacingToken tok = static_cast<SpacingToken>(i);
        g_ThemeCache.spacings[i] = theme.ResolveSpacing(tok);
    }
    for (uint32_t i = 0; i < kMaxRadiusTokens; ++i) {
        RadiusToken tok = static_cast<RadiusToken>(i);
        g_ThemeCache.radii[i] = theme.ResolveRadius(tok);
    }
    for (uint32_t i = 0; i < kMaxTypographyTokens; ++i) {
        TypographyToken tok = static_cast<TypographyToken>(i);
        g_ThemeCache.fontSizes[i] = theme.ResolveFontSize(tok);
    }
    g_ThemeCache.version = targetVersion;
    g_ThemeCache.initialized = true;
}

void EnsureCacheValid() {
    palette::ReloadGraphiteDarkPaletteIfChanged();
    const uint64_t targetVer = g_GlobalThemeVersion.load(std::memory_order_relaxed);
    if (g_ThemeCache.version == targetVer && g_ThemeCache.initialized) {
        return;
    }
    std::lock_guard lock(g_ThemeCacheMutex);
    const uint64_t currVer = g_GlobalThemeVersion.load(std::memory_order_relaxed);
    if (g_ThemeCache.version == currVer && g_ThemeCache.initialized) {
        return;
    }
    RebuildTokenCacheLocked(currVer);
}

}

void InvalidateThemeCache() {
    g_GlobalThemeVersion.fetch_add(1, std::memory_order_release);
}

uint64_t GetThemeCacheVersion() {
    return g_GlobalThemeVersion.load(std::memory_order_relaxed);
}

IKindUITheme& ResolveDefaultTheme() {
    return ThemeManager::Get().Theme();
}

Color ResolveColor(ColorToken token) {
    EnsureCacheValid();
    const uint32_t idx = static_cast<uint32_t>(token);
    if (idx < kMaxColorTokens) {
        Color resolved = g_ThemeCache.colors[idx];
        if (UiColorDebug::IsEnabled()) {
            UiColorDebug::Get().TraceResolve(token, resolved);
        }
        return resolved;
    }
    return Color::Transparent();
}

float ResolveMetric(MetricToken token) {
    EnsureCacheValid();
    const uint32_t idx = static_cast<uint32_t>(token);
    if (idx < kMaxMetricTokens) {
        return g_ThemeCache.metrics[idx];
    }
    return 0.0f;
}

Margin ResolvePadding(PaddingToken token) {
    EnsureCacheValid();
    const uint32_t idx = static_cast<uint32_t>(token);
    if (idx < kMaxPaddingTokens) {
        return g_ThemeCache.paddings[idx];
    }
    return {};
}

float ResolveSpacing(SpacingToken token) {
    EnsureCacheValid();
    const uint32_t idx = static_cast<uint32_t>(token);
    if (idx < kMaxSpacingTokens) {
        return g_ThemeCache.spacings[idx];
    }
    return 0.0f;
}

float ResolveRadius(RadiusToken token) {
    EnsureCacheValid();
    const uint32_t idx = static_cast<uint32_t>(token);
    if (idx < kMaxRadiusTokens) {
        return g_ThemeCache.radii[idx];
    }
    return 0.0f;
}

float ResolveFontSize(TypographyToken token) {
    EnsureCacheValid();
    const uint32_t idx = static_cast<uint32_t>(token);
    if (idx < kMaxTypographyTokens) {
        return g_ThemeCache.fontSizes[idx];
    }
    return 12.0f;
}

TypographySpec ResolveTypography(TypographyToken token) {
    return ThemeManager::Get().Theme().ResolveTypography(token);
}

float ResolveControlHeight(ControlSize size) {
    switch (size) {
    case ControlSize::Compact:
        return ResolveMetric(MetricToken::ControlHeightCompact);
    case ControlSize::Large:
        return ResolveMetric(MetricToken::ControlHeightLarge);
    case ControlSize::Default:
    default:
        return ResolveMetric(MetricToken::ButtonHeight);
    }
}

Color ResolveInteractiveBackground(float hoverAnim, float pressAnim, bool selected) {
    return ResolveInteractiveBackground(hoverAnim, pressAnim, selected, ColorToken::PanelBackground);
}

Color ResolveInteractiveBackground(
    float hoverAnim,
    float pressAnim,
    bool selected,
    ColorToken surfaceToken)
{
    return ResolveInteractiveBackgroundImpl(hoverAnim, pressAnim, selected, surfaceToken);
}

Color MixInteractiveSurface(
    Color base,
    float hoverAnim,
    float pressAnim,
    bool selected,
    bool disabled,
    Color opaqueUnderlay)
{
    // ResolveColor inside the blender polls palette hot-reload via EnsureCacheValid.
    return MixInteractiveSurfaceImpl(base, hoverAnim, pressAnim, selected, disabled, opaqueUnderlay);
}

Color ResolveTextForState(bool hovered, bool active) {
    return ThemeManager::Get().Theme().TextForState(hovered, active);
}

Color ResolveIconForState(bool hovered, bool active) {
    return ThemeManager::Get().Theme().IconForState(hovered, active);
}

Color ResolveIconColor(
    IconColorRole role,
    float hoverAnim,
    float pressStrength,
    bool accent)
{
    if (accent || role == IconColorRole::Accent) {
        Color accentColor = ResolveColor(ColorToken::IconAccent);
        if (hoverAnim > 0.001f || pressStrength > 0.001f) {
            accentColor = ColorSpace::LerpColor(
                accentColor,
                ResolveColor(ColorToken::AccentHover),
                Clamp01(std::max(hoverAnim, pressStrength)) * 0.25f);
        }
        return accentColor;
    }

    if (role == IconColorRole::Disabled) {
        return ResolveColor(ColorToken::IconDisabled);
    }

    return ResolveColor(ColorToken::IconSecondary);
}

Color ResolveIconColorForState(bool hovered, bool accent, bool disabled, bool secondary) {
    if (disabled) {
        return ResolveIconColor(IconColorRole::Disabled);
    }
    if (accent) {
        return ResolveIconColor(IconColorRole::Accent, hovered ? 1.0f : 0.0f, 0.0f, true);
    }
    return ResolveIconColor(
        IconColorRole::Secondary,
        hovered ? 1.0f : 0.0f,
        0.0f,
        accent);
}

}

