#include "PlaceActors/PlaceActorsItem.h"

#include "PlaceActors/ActorsPanelLayout.h"
#include "PlaceActors/ActorsPanelChrome.h"
#include "PlaceActors/PlaceActorsActorCard.h"
#include "PlaceActors/PlaceActorsIconProvider.h"
#include "PlaceActors/PlaceActorsSearch.h"
#include "PlaceActors/PlaceActorsResponsiveGrid.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>

namespace we::programs::editor {
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::WindIconRef;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace PanelChrome = ::we::editor::panels::PanelChrome;


using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;


void PaintItemIcon(PaintContext& context, we::runtime::kindui::WindIconRef icon, const Rect& iconRect) {
    if (icon.IsValid()) {
        IconPainter::Draw(context, icon, iconRect);
    }
}

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

    const float uiScale = std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
    const float labelFontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeBody) * uiScale;
    const float iconSize = static_cast<float>(16u);

    const Rect rowRect{
        bounds.x + 2.0f,
        bounds.y + 1.0f,
        std::max(0.0f, bounds.width - 4.0f),
        std::max(0.0f, bounds.height - 2.0f)
    };
    ActorsPanelChrome::PaintActorRowBackground(context, rowRect, hoverAnim, pressAnim, selected);

    const float iconX = ActorsPanelLayout::ItemIconX(bounds.x);
    Rect iconBand{ iconX, bounds.y, iconSize, bounds.height };
    const WindIconRef chromeIcon = PlaceActorsIconProvider::Get().ResolveChromeIcon(item);
    PaintItemIcon(
        context,
        chromeIcon.IsValid() ? chromeIcon : item.icon,
        IconMetrics::PlaceGlyphCentered(iconBand, static_cast<uint32_t>(iconSize)));

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
        we::runtime::kindui::ResolveColor(ColorToken::TextPrimary),
        we::runtime::kindui::ResolveColor(ColorToken::TextPrimary));

    const float starSize = static_cast<float>(16u);
    const float starX = ActorsPanelLayout::StarIconX(bounds.x, bounds.width);
    Rect starBand{ starX, bounds.y, starSize, bounds.height };
    if (favorite || hoverAnim > 0.01f) {
        const Color starColor = favorite
            ? we::runtime::kindui::ResolveColor(ColorToken::Warning)
            : we::runtime::kindui::ResolveColor(ColorToken::IconPrimary);
        we::runtime::kindui::IconPainter::Draw(
            context, we::runtime::kindui::kWindIconNone, we::runtime::kindui::IconMetrics::PlaceGlyphCentered(starBand, 16u));
    }
}

} // namespace we::programs::editor
