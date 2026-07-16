#pragma once

#include "Core/Icon.h"
#include "Core/PaintContext.h"
#include "UI/LauncherHelpers.h"
#include "UI/Widgets/SkeletonViews.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

namespace we::programs::welauncher {

enum class ThumbnailVisualState {
    Skeleton,     // shimmer while resolving
    Placeholder,  // intentional missing-image art
    Ready         // real texture (fade in)
};

inline uint32_t HashString(const std::string& text) {
    uint32_t h = 2166136261u;
    for (unsigned char c : text) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

inline WindEffects::Editor::UI::Color AccentFromKey(const std::string& key) {
    static const WindEffects::Editor::UI::Color kPalette[] = {
        { 0.28f, 0.42f, 0.62f, 1.0f },
        { 0.32f, 0.48f, 0.44f, 1.0f },
        { 0.46f, 0.34f, 0.52f, 1.0f },
        { 0.48f, 0.38f, 0.28f, 1.0f },
        { 0.30f, 0.36f, 0.48f, 1.0f },
        { 0.36f, 0.46f, 0.34f, 1.0f },
    };
    const uint32_t h = HashString(key);
    return kPalette[h % (sizeof(kPalette) / sizeof(kPalette[0]))];
}

inline char ProjectInitial(const std::string& name) {
    for (unsigned char c : name) {
        if (std::isalnum(c)) {
            return static_cast<char>(std::toupper(c));
        }
    }
    return 'W';
}

inline void PaintGeometricAccent(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& rect,
    const WindEffects::Editor::UI::Color& accent,
    uint32_t seed) {
    using namespace WindEffects::Editor::UI;
    const float s = LScale();

    Color a = accent;
    a.a = 0.18f;
    context.DrawRoundedRect(
        Rect{
            rect.x + rect.width * 0.55f,
            rect.y - 12.0f * s,
            rect.width * 0.6f,
            rect.height * 0.7f
        },
        a,
        40.0f * s);

    Color b = accent;
    b.a = 0.12f;
    context.DrawRoundedRect(
        Rect{
            rect.x - 20.0f * s,
            rect.y + rect.height * 0.45f,
            rect.width * 0.45f,
            rect.height * 0.7f
        },
        b,
        28.0f * s);

    Color c = accent;
    c.a = 0.10f + 0.04f * static_cast<float>((seed >> 3) % 5);
    const float orb = 48.0f * s + static_cast<float>(seed % 20) * s;
    context.DrawRoundedRect(
        Rect{
            rect.x + rect.width * 0.35f,
            rect.y + rect.height * 0.2f,
            orb,
            orb
        },
        c,
        orb * 0.5f);
}

inline void PaintProjectThumbnailPlaceholder(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& rect,
    float radius,
    const std::string& displayName,
    const std::string& templateId,
    bool compatible,
    float fade = 1.0f) {
    using namespace WindEffects::Editor::UI;
    const float s = LScale();
    const float a = std::clamp(fade, 0.0f, 1.0f);

    Color top = LColor(ThemeToken::PanelContentBackground);
    Color accent = AccentFromKey(templateId.empty() ? displayName : templateId);
    Color bottom = Color::Lerp(LColor(ThemeToken::HeaderBackground), accent, 0.35f);
    top.a *= a;
    bottom.a *= a;
    context.DrawGradient(rect, top, bottom, radius);

    PaintGeometricAccent(context, rect, accent, HashString(displayName));

    Color rim = LColor(ThemeToken::BorderDefault);
    rim.a *= 0.65f * a;
    context.DrawRoundedRectOutline(rect, rim, 1.0f, radius);

    const char initial = ProjectInitial(displayName);
    const std::string letter(1, initial);
    const float letterSize = 48.0f * s;
    const float letterW = letterSize * 0.62f;
    Color letterColor = LColor(ThemeToken::TextPrimary);
    letterColor.a *= 0.92f * a;
    context.DrawText(
        letter,
        Point{
            rect.x + (rect.width - letterW) * 0.5f,
            rect.y + (rect.height - letterSize) * 0.42f
        },
        letterColor,
        letterSize,
        true);

    const float iconSize = 18.0f * s;
    Color iconColor = compatible ? LColor(ThemeToken::IconAccent) : LColor(ThemeToken::Warning);
    iconColor.a *= a;
    IconPainter::DrawIcon(
        context,
        Icons::Cube3DName,
        Rect{
            rect.x + (rect.width - iconSize) * 0.5f,
            rect.y + rect.height * 0.68f,
            iconSize,
            iconSize
        },
        iconColor);
}

inline void PaintTemplateThumbnailPlaceholder(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& rect,
    float radius,
    const std::string& displayName,
    const std::string& category,
    const std::string& templateId,
    float fade = 1.0f) {
    using namespace WindEffects::Editor::UI;
    (void)displayName;
    const float s = LScale();
    const float a = std::clamp(fade, 0.0f, 1.0f);

    Color accent = AccentFromKey(templateId.empty() ? category : templateId);
    Color top = Color::Lerp(LColor(ThemeToken::PanelContentBackground), accent, 0.28f);
    Color bottom = Color::Lerp(LColor(ThemeToken::HeaderBackground), accent, 0.45f);
    top.a *= a;
    bottom.a *= a;
    context.DrawGradient(rect, top, bottom, radius);
    PaintGeometricAccent(context, rect, accent, HashString(templateId + category));

    Color rim = LColor(ThemeToken::BorderDefault);
    rim.a *= 0.7f * a;
    context.DrawRoundedRectOutline(rect, rim, 1.0f, radius);

    const float iconSize = 40.0f * s;
    Color iconColor = LColor(ThemeToken::IconAccent);
    iconColor.a *= a;
    IconPainter::DrawIcon(
        context,
        Icons::LayersName,
        Rect{
            rect.x + (rect.width - iconSize) * 0.5f,
            rect.y + (rect.height - iconSize) * 0.38f,
            iconSize,
            iconSize
        },
        iconColor);

    const std::string badge = category.empty() ? "Template" : category;
    const float badgeH = 18.0f * s;
    const float badgePad = 8.0f * s;
    const float badgeW = std::min(
        rect.width * 0.6f,
        static_cast<float>(badge.size()) * 6.5f * s + badgePad * 2.0f);
    Rect badgeRect{
        rect.x + 10.0f * s,
        rect.y + rect.height - badgeH - 10.0f * s,
        badgeW,
        badgeH
    };
    Color badgeBg = LColor(ThemeToken::HoverBackground);
    badgeBg.a *= a;
    context.DrawRoundedRect(badgeRect, badgeBg, badgeH * 0.5f);
    Color badgeText = LColor(ThemeToken::TextSecondary);
    badgeText.a *= a;
    context.DrawText(
        badge,
        Point{
            badgeRect.x + badgePad,
            badgeRect.y + (badgeH - LMetric(ThemeToken::TextSizeCaption) * s) * 0.5f
        },
        badgeText,
        LMetric(ThemeToken::TextSizeCaption) * s);
}

} // namespace we::programs::welauncher
