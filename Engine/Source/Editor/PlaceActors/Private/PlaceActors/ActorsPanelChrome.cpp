#include "PlaceActors/ActorsPanelChrome.h"

#include "PlaceActors/ActorsPanelLayout.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"
#include "Rendering/IconMetrics.h"

#include <algorithm>

namespace we::programs::editor::ActorsPanelChrome {

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::ThemeToken;

void PaintSearchField(PaintContext& context, const Rect& bounds, const std::string& placeholder, const std::string& text, bool focused, bool showCaret) {
    const float uiScale = std::max(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float radius = ResolveThemeMetric(ThemeToken::CornerRadiusSmall) * uiScale;
    const float pad = ActorsPanelLayout::ContentPadH();

    Color bg = ResolveThemeColor(ThemeToken::SearchBoxBackground);
    if (focused) {
        bg = Color::Lerp(bg, ResolveThemeColor(ThemeToken::HoverBackground), 0.35f);
    }
    context.DrawRoundedRect(bounds, bg, radius);
    context.DrawRoundedRectOutline(bounds, ResolveThemeColor(ThemeToken::BorderDefault), 1.0f, radius);

    const float iconSize = static_cast<float>(WindEffects::Editor::UI::IconMetrics::GlyphTierPx(ThemeToken::IconSizeSearch));
    Rect iconBand{ bounds.x + pad, bounds.y, iconSize, bounds.height };
    WindEffects::Editor::UI::IconPainter::DrawIcon(
        context,
        WindEffects::Editor::UI::Icons::SearchName,
        WindEffects::Editor::UI::IconMetrics::PlaceGlyphCentered(iconBand, iconSize),
        ResolveThemeColor(ThemeToken::IconDefault));

    const float fontSize = ResolveThemeMetric(ThemeToken::TextSizeBody) * uiScale;
    const float textX = bounds.x + pad + iconSize + ResolveThemeMetric(ThemeToken::Space2);
    const float textY = bounds.y + (bounds.height - fontSize) * 0.5f;
    const std::string& display = text.empty() ? placeholder : text;
    const Color textColor = text.empty()
        ? ResolveThemeColor(ThemeToken::SearchPlaceholder)
        : ResolveThemeColor(ThemeToken::TextPrimary);
    context.DrawText(display, Point{ textX, textY }, textColor, fontSize);

    if (!text.empty() && focused && showCaret) {
        const float caretX = textX + context.GetTextWidth(text, fontSize);
        context.DrawRect(
            Rect{ caretX, textY, ResolveThemeMetric(ThemeToken::BorderWidth), fontSize },
            ResolveThemeColor(ThemeToken::TextPrimary));
    }
}

void PaintActorRowBackground(PaintContext& context, const Rect& rowRect, float hoverAnim, float pressAnim, bool selected) {
    const float radius = ActorsPanelLayout::RowRadius();
    if (selected) {
        context.DrawRoundedRect(rowRect, ResolveThemeColor(ThemeToken::SelectedBackground), radius);
        context.DrawRoundedRectOutline(rowRect, ResolveThemeColor(ThemeToken::BorderDefault), 1.0f, radius);
        return;
    }

    if (hoverAnim > 0.01f || pressAnim > 0.01f) {
        Color bg = ResolveThemeColor(ThemeToken::HoverBackground);
        const float t = std::clamp(std::max(hoverAnim, pressAnim * 0.85f), 0.0f, 1.0f);
        bg.a *= t;
        context.DrawRoundedRect(rowRect, bg, radius);
    }
}

void PaintCategoryHeaderBackground(PaintContext& context, const Rect& bounds, float hoverAnim) {
    Color bg = ResolveThemeColor(ThemeToken::ActiveBackground);
    bg.a = 0.55f;
    context.DrawRoundedRect(bounds, bg, ActorsPanelLayout::SectionRadius());
    if (hoverAnim > 0.01f) {
        Color hover = ResolveThemeColor(ThemeToken::HoverBackground);
        hover.a *= std::clamp(hoverAnim, 0.0f, 1.0f) * 0.6f;
        context.DrawRoundedRect(bounds, hover, ActorsPanelLayout::SectionRadius());
    }
}

void PaintSectionBackground(PaintContext& context, const Rect& bounds) {
    Color fill = ResolveThemeColor(ThemeToken::ActiveBackground);
    fill.a = 0.28f;
    context.DrawRoundedRect(bounds, fill, ActorsPanelLayout::SectionRadius());
}

void PaintSoftSeparator(PaintContext& context, const Rect& bounds) {
    const float y = bounds.y + bounds.height * 0.5f;
    context.DrawLine(
        Point{ bounds.x + ActorsPanelLayout::ContentPadH(), y },
        Point{ bounds.x + bounds.width - ActorsPanelLayout::ContentPadH(), y },
        ResolveThemeColor(ThemeToken::Separator),
        1.0f);
}

void PaintChevron(PaintContext& context, const Rect& bounds, bool expanded, float hoverAnim) {
    const float tier = static_cast<float>(WindEffects::Editor::UI::IconMetrics::GlyphTierPx(bounds.width));
    Color color = ResolveThemeColor(ThemeToken::TextSecondary);
    color = Color::Lerp(color, ResolveThemeColor(ThemeToken::TextPrimary), std::clamp(hoverAnim, 0.0f, 1.0f));

    const char* chevronIcon = expanded
        ? WindEffects::Editor::UI::Icons::ChevronDownName
        : WindEffects::Editor::UI::Icons::ChevronRightName;
    WindEffects::Editor::UI::IconPainter::DrawIcon(
        context,
        chevronIcon,
        WindEffects::Editor::UI::IconMetrics::PlaceGlyphCentered(bounds, tier),
        color);
}

} // namespace we::programs::editor::ActorsPanelChrome
