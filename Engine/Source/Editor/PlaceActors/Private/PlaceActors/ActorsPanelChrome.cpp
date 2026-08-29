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

void PaintActorRowBackground(PaintContext& context, const Rect& rowRect, float hoverAnim, float pressAnim, bool selected) {
    const float radius = ActorsPanelLayout::RowRadius();
    if (selected) {
        context.DrawRoundedRect(rowRect, we::runtime::kindui::ResolveColor(ColorToken::SelectedBackground), radius);
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
    Color bg = we::runtime::kindui::ResolveColor(ColorToken::HoverBackground);
    bg.a = 0.55f;
    context.DrawRoundedRect(bounds, bg, ActorsPanelLayout::SectionRadius());
    if (hoverAnim > 0.01f) {
        Color hover = we::runtime::kindui::ResolveColor(ColorToken::HoverBackground);
        hover.a *= std::clamp(hoverAnim, 0.0f, 1.0f) * 0.6f;
        context.DrawRoundedRect(bounds, hover, ActorsPanelLayout::SectionRadius());
    }
}

void PaintSectionBackground(PaintContext& context, const Rect& bounds) {
    Color fill = we::runtime::kindui::ResolveColor(ColorToken::HoverBackground);
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
