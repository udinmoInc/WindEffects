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
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>

namespace we::programs::editor {
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyEventType;
using ::we::runtime::kindui::IconPainter;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;


using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace WEIcons = we::runtime::kindui::Icons;

namespace {

void PaintItemIcon(PaintContext& context, const std::string& iconName, const Rect& iconRect) {
    we::runtime::kindui::IconPainter::DrawIcon(
        context, iconName, iconRect, we::runtime::kindui::ResolveColor(ColorToken::IconPrimary));
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

    const float uiScale = std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
    const float labelFontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeBody) * uiScale;
    const float iconSize = static_cast<float>(we::runtime::kindui::IconMetrics::GlyphTierPx(MetricToken::IconSizeTree));

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
        we::runtime::kindui::IconMetrics::PlaceGlyphCentered(iconBand, iconSize));

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

    const float starSize = static_cast<float>(we::runtime::kindui::IconMetrics::StandardGlyphTierPx());
    const float starX = ActorsPanelLayout::StarIconX(bounds.x, bounds.width);
    Rect starBand{ starX, bounds.y, starSize, bounds.height };
    if (favorite || hoverAnim > 0.01f) {
        const Color starColor = favorite
            ? we::runtime::kindui::ResolveColor(ColorToken::Warning)
            : we::runtime::kindui::ResolveColor(ColorToken::IconPrimary);
        we::runtime::kindui::IconPainter::DrawIcon(
            context,
            WEIcons::StarName,
            we::runtime::kindui::IconMetrics::PlaceGlyphCentered(starBand, starSize),
            starColor);
    }
}

} // namespace we::programs::editor
