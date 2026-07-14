#include "PlaceActors/PlaceActorsItem.h"

#include "PlaceActors/ActorsPanelLayout.h"
#include "PlaceActors/ActorsPanelChrome.h"
#include "PlaceActors/PlaceActorsActorCard.h"
#include "PlaceActors/PlaceActorsIconProvider.h"
#include "PlaceActors/PlaceActorsSearch.h"
#include "PlaceActors/PlaceActorsResponsiveGrid.h"
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
using WindEffects::Editor::UI::Size;
using WindEffects::Editor::UI::ThemeToken;

namespace WEIcons = WindEffects::Editor::UI::Icons;

namespace {

void PaintItemIcon(PaintContext& context, const std::string& iconName, const Rect& iconRect) {
    WindEffects::Editor::UI::IconPainter::DrawIcon(
        context, iconName, iconRect, ResolveThemeColor(ThemeToken::IconPrimary));
}

} // namespace

Size PlaceActorsItem::MeasureGrid(const PlaceActorsItemMetrics& metrics) {
    return Size{ metrics.cardSize, metrics.cardHeight > 0.0f ? metrics.cardHeight : metrics.cardSize };
}

Size PlaceActorsItem::MeasureList(const PlaceActorsItemMetrics& metrics) {
    return Size{ 0.0f, metrics.listRowHeight };
}

void PlaceActorsItem::PaintGrid(PaintContext& context,
                                const Rect& bounds,
                                const PlaceActorsItemData& item,
                                const PlaceActorsItemMetrics& metrics,
                                float hoverAnim,
                                float pressAnim,
                                bool selected,
                                bool favorite) {
    PlaceActorsGridLayout gridLayout;
    gridLayout.previewSize = metrics.previewSize > 0.0f ? metrics.previewSize : metrics.cardSize;
    gridLayout.cardWidth = bounds.width;
    gridLayout.cardHeight = bounds.height;
    const Rect preview = PlaceActorsResponsiveGrid::PreviewRect(gridLayout, bounds);
    PlaceActorsActorCard::Paint(context, bounds, preview, item, hoverAnim, pressAnim, selected, favorite);
}

void PlaceActorsItem::PaintList(PaintContext& context,
                                const Rect& bounds,
                                const PlaceActorsItemData& item,
                                const PlaceActorsItemMetrics& metrics,
                                float hoverAnim,
                                float pressAnim,
                                bool selected,
                                bool favorite,
                                const std::string& searchQuery,
                                float revealAnim)
{
    (void)metrics;
    if (revealAnim <= 0.01f) {
        return;
    }

    const float uiScale = std::max(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float labelFontSize = ResolveThemeMetric(ThemeToken::TextSizeBody) * uiScale;
    const float iconSize = static_cast<float>(WindEffects::Editor::UI::IconMetrics::GlyphTierPx(ThemeToken::IconSizeTree));

    const Rect rowRect{
        bounds.x + 2.0f,
        bounds.y + 1.0f,
        std::max(0.0f, bounds.width - 4.0f),
        std::max(0.0f, bounds.height - 2.0f)
    };
    ActorsPanelChrome::PaintActorRowBackground(context, rowRect, hoverAnim, pressAnim, selected);

    const float iconX = ActorsPanelLayout::ItemIconX(bounds.x);
    Rect iconBand{ iconX, bounds.y, iconSize, bounds.height };
    const std::string chromeIcon = PlaceActorsIconProvider::Get().ResolveChromeIcon(item);
    PaintItemIcon(
        context,
        !chromeIcon.empty() ? chromeIcon : item.iconName,
        WindEffects::Editor::UI::IconMetrics::PlaceGlyphCentered(iconBand, iconSize));

    const Point labelPos{
        ActorsPanelLayout::LabelX(bounds.x),
        bounds.y + (bounds.height - labelFontSize) * 0.5f
    };
    PlaceActorsSearch::PaintHighlightedLabel(
        context,
        item.label,
        labelPos,
        labelFontSize,
        searchQuery,
        ResolveThemeColor(ThemeToken::TextPrimary),
        ResolveThemeColor(ThemeToken::TextPrimary));

    const float starSize = static_cast<float>(WindEffects::Editor::UI::IconMetrics::StandardGlyphTierPx());
    const float starX = ActorsPanelLayout::StarIconX(bounds.x, bounds.width);
    Rect starBand{ starX, bounds.y, starSize, bounds.height };
    if (favorite || hoverAnim > 0.01f) {
        const Color starColor = favorite
            ? ResolveThemeColor(ThemeToken::Warning)
            : ResolveThemeColor(ThemeToken::IconPrimary);
        WindEffects::Editor::UI::IconPainter::DrawIcon(
            context,
            WEIcons::StarName,
            WindEffects::Editor::UI::IconMetrics::PlaceGlyphCentered(starBand, starSize),
            starColor);
    }
}

} // namespace we::programs::editor
