#include "PlaceActors/ActorsPanelChrome.h"

#include "PlaceActors/ActorsPanelLayout.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>

namespace we::programs::editor::ActorsPanelChrome {

using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

void PaintSearchField(PaintContext& context, const Rect& bounds, const std::string& placeholder, const std::string& text, bool focused, bool showCaret) {
    const float uiScale = std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
    const float radius = we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall) * uiScale;
    const float pad = ActorsPanelLayout::ContentPadH();

    Color bg = we::runtime::kindui::ResolveColor(ColorToken::SearchBoxBackground);
    if (focused) {
        bg = Color::Lerp(bg, we::runtime::kindui::ResolveColor(ColorToken::HoverBackground), 0.35f);
    }
    context.DrawRoundedRect(bounds, bg, radius);
    context.DrawRoundedRectOutline(bounds, we::runtime::kindui::ResolveColor(ColorToken::BorderDefault), 1.0f, radius);

    const float iconSize = static_cast<float>(we::runtime::kindui::IconMetrics::GlyphTierPx(MetricToken::IconSizeSearch));
    Rect iconBand{ bounds.x + pad, bounds.y, iconSize, bounds.height };
    we::runtime::kindui::IconPainter::DrawIcon(
        context,
        we::runtime::kindui::Icons::SearchName,
        we::runtime::kindui::IconMetrics::PlaceGlyphCentered(iconBand, iconSize),
        we::runtime::kindui::ResolveColor(ColorToken::IconPrimary));

    const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeBody) * uiScale;
    const float textX = bounds.x + pad + iconSize + we::runtime::kindui::ResolveMetric(MetricToken::Space2);
    const float textY = bounds.y + (bounds.height - fontSize) * 0.5f;
    const std::string& display = text.empty() ? placeholder : text;
    const Color textColor = text.empty()
        ? we::runtime::kindui::ResolveColor(ColorToken::SearchPlaceholder)
        : we::runtime::kindui::ResolveColor(ColorToken::TextPrimary);
    context.DrawText(display, Point{ textX, textY }, textColor, fontSize);

    if (!text.empty() && focused && showCaret) {
        const float caretX = textX + context.GetTextWidth(text, fontSize);
        context.DrawRect(
            Rect{ caretX, textY, we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth), fontSize },
            we::runtime::kindui::ResolveColor(ColorToken::TextPrimary));
    }
}

void PaintActorRowBackground(PaintContext& context, const Rect& rowRect, float hoverAnim, float pressAnim, bool selected) {
    const float radius = ActorsPanelLayout::RowRadius();
    if (selected) {
        context.DrawRoundedRect(rowRect, we::runtime::kindui::ResolveColor(ColorToken::SelectedBackground), radius);
        context.DrawRoundedRectOutline(rowRect, we::runtime::kindui::ResolveColor(ColorToken::BorderDefault), 1.0f, radius);
        return;
    }

    if (hoverAnim > 0.01f || pressAnim > 0.01f) {
        Color bg = we::runtime::kindui::ResolveColor(ColorToken::HoverBackground);
        const float t = std::clamp(std::max(hoverAnim, pressAnim * 0.85f), 0.0f, 1.0f);
        bg.a *= t;
        context.DrawRoundedRect(rowRect, bg, radius);
    }
}

void PaintCategoryHeaderBackground(PaintContext& context, const Rect& bounds, float hoverAnim) {
    Color bg = we::runtime::kindui::ResolveColor(ColorToken::ActiveBackground);
    bg.a = 0.55f;
    context.DrawRoundedRect(bounds, bg, ActorsPanelLayout::SectionRadius());
    if (hoverAnim > 0.01f) {
        Color hover = we::runtime::kindui::ResolveColor(ColorToken::HoverBackground);
        hover.a *= std::clamp(hoverAnim, 0.0f, 1.0f) * 0.6f;
        context.DrawRoundedRect(bounds, hover, ActorsPanelLayout::SectionRadius());
    }
}

void PaintSectionBackground(PaintContext& context, const Rect& bounds) {
    Color fill = we::runtime::kindui::ResolveColor(ColorToken::ActiveBackground);
    fill.a = 0.28f;
    context.DrawRoundedRect(bounds, fill, ActorsPanelLayout::SectionRadius());
}

void PaintSoftSeparator(PaintContext& context, const Rect& bounds) {
    const float y = bounds.y + bounds.height * 0.5f;
    context.DrawLine(
        Point{ bounds.x + ActorsPanelLayout::ContentPadH(), y },
        Point{ bounds.x + bounds.width - ActorsPanelLayout::ContentPadH(), y },
        we::runtime::kindui::ResolveColor(ColorToken::Separator),
        1.0f);
}

void PaintChevron(PaintContext& context, const Rect& bounds, bool expanded, float hoverAnim) {
    const float tier = static_cast<float>(we::runtime::kindui::IconMetrics::StandardGlyphTierPx());
    Color color = we::runtime::kindui::ResolveColor(ColorToken::TextSecondary);
    color = Color::Lerp(color, we::runtime::kindui::ResolveColor(ColorToken::TextPrimary), std::clamp(hoverAnim, 0.0f, 1.0f));

    const char* chevronIcon = expanded
        ? we::runtime::kindui::Icons::ChevronDownName
        : we::runtime::kindui::Icons::ChevronRightName;
    we::runtime::kindui::IconPainter::DrawIcon(
        context,
        chevronIcon,
        we::runtime::kindui::IconMetrics::PlaceGlyphCentered(bounds, tier),
        color);
}

} // namespace we::programs::editor::ActorsPanelChrome
