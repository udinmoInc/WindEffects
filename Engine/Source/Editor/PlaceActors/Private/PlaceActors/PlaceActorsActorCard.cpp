#include "PlaceActors/PlaceActorsActorCard.h"

#include "PlaceActors/ActorsPanelChrome.h"
#include "PlaceActors/ActorsPanelLayout.h"
#include "PlaceActors/PlaceActorsThumbnailProvider.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"
#include "Rendering/IconMetrics.h"

#include <algorithm>

namespace we::programs::editor {

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::ThemeToken;
namespace WEIcons = WindEffects::Editor::UI::Icons;

namespace {

Rect FavoriteStarRect(const Rect& cardBounds) {
    const float starSize = static_cast<float>(WindEffects::Editor::UI::IconMetrics::StandardGlyphTierPx());
    return Rect{
        cardBounds.x + cardBounds.width - starSize - 4.0f,
        cardBounds.y + 4.0f,
        starSize,
        starSize
    };
}

} // namespace

void PlaceActorsActorCard::Paint(PaintContext& context,
                                 const Rect& cardBounds,
                                 const Rect& previewBounds,
                                 const PlaceActorsItemData& item,
                                 float hoverAnim,
                                 float pressAnim,
                                 bool selected,
                                 bool favorite) {
    const float uiScale = std::max(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float labelFontSize = ResolveThemeMetric(ThemeToken::TextSizeSmall) * uiScale;
    const float radius = ActorsPanelLayout::RowRadius();

    if (selected) {
        context.DrawRoundedRect(cardBounds, ResolveThemeColor(ThemeToken::SelectedBackground), radius);
        context.DrawRoundedRectOutline(cardBounds, ResolveThemeColor(ThemeToken::BorderDefault), 1.0f, radius);
    } else if (hoverAnim > 0.01f || pressAnim > 0.01f) {
        Color bg = ResolveThemeColor(ThemeToken::HoverBackground);
        bg.a *= std::clamp(std::max(hoverAnim, pressAnim * 0.85f), 0.0f, 1.0f);
        context.DrawRoundedRect(cardBounds, bg, radius);
    }

    PlaceActorsThumbnailProvider::Get().Paint(context, previewBounds, item, hoverAnim);

    const float maxLabelWidth = std::max(8.0f, cardBounds.width - 4.0f);
    std::string label = item.label;
    if (context.GetTextWidth(label, labelFontSize) > maxLabelWidth) {
        while (label.size() > 1 &&
               context.GetTextWidth(label + "...", labelFontSize) > maxLabelWidth) {
            label.pop_back();
        }
        label += "...";
    }

    const float textWidth = context.GetTextWidth(label, labelFontSize);
    const float textX = cardBounds.x + std::max(2.0f, (cardBounds.width - textWidth) * 0.5f);
    const float textY = previewBounds.y + previewBounds.height
        + std::max(2.0f, (cardBounds.y + cardBounds.height - (previewBounds.y + previewBounds.height) - labelFontSize) * 0.5f);
    context.DrawText(label, Point{ textX, textY }, ResolveThemeColor(ThemeToken::TextPrimary), labelFontSize);

    if (favorite || hoverAnim > 0.01f) {
        const Rect starRect = FavoriteStarRect(cardBounds);
        const Color starColor = favorite
            ? ResolveThemeColor(ThemeToken::Warning)
            : ResolveThemeColor(ThemeToken::IconPrimary);
        WindEffects::Editor::UI::IconPainter::DrawIcon(context, WEIcons::StarName, starRect, starColor);
    }
}

bool PlaceActorsActorCard::HitFavoriteStar(const Rect& cardBounds, const Point& position) {
    const Rect star = FavoriteStarRect(cardBounds);
    const Rect hit{
        star.x - 4.0f,
        star.y - 4.0f,
        star.width + 8.0f,
        star.height + 8.0f
    };
    return hit.Contains(position);
}

} // namespace we::programs::editor
