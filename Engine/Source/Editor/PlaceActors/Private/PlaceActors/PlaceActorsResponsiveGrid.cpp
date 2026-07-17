#include "PlaceActors/PlaceActorsResponsiveGrid.h"

#include <algorithm>
#include <cmath>

namespace we::programs::editor {

using we::runtime::kindui::Rect;

PlaceActorsGridLayout PlaceActorsResponsiveGrid::Compute(
    float viewportWidth,
    const PlaceActorsGridMetrics& metrics)
{
    PlaceActorsGridLayout layout;
    layout.cardGap = metrics.cardGap;
    layout.contentPad = metrics.contentPad;
    layout.scrollbarReserve = metrics.scrollbarReserve;

    const float rightReserve = metrics.contentPad + metrics.scrollbarReserve;
    const float available = std::max(
        metrics.minCardWidth,
        viewportWidth - metrics.contentPad - rightReserve);
    const float stride = metrics.minCardWidth + metrics.cardGap;
    int columns = static_cast<int>(std::floor((available + metrics.cardGap) / stride));
    columns = std::max(1, columns);

    float cardWidth = (available - metrics.cardGap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    cardWidth = std::clamp(cardWidth, metrics.minCardWidth, metrics.maxCardWidth);

    while (columns > 1) {
        const float fitted = (available - metrics.cardGap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
        if (fitted >= metrics.minCardWidth * 0.95f) {
            cardWidth = std::min(fitted, metrics.maxCardWidth);
            break;
        }
        --columns;
    }
    if (columns == 1) {
        cardWidth = std::clamp(available, metrics.minCardWidth, metrics.maxCardWidth);
    }

    layout.columns = columns;
    layout.cardWidth = cardWidth;
    layout.previewSize = cardWidth;
    layout.cardHeight = layout.previewSize + metrics.labelGap + metrics.labelHeight;
    return layout;
}

Rect PlaceActorsResponsiveGrid::CardRect(
    const PlaceActorsGridLayout& layout,
    float originX,
    float originY,
    int index)
{
    const int col = index % layout.columns;
    const int row = index / layout.columns;
    const float x = originX + layout.contentPad + static_cast<float>(col) * (layout.cardWidth + layout.cardGap);
    const float y = originY + static_cast<float>(row) * (layout.cardHeight + layout.cardGap);
    return Rect{ x, y, layout.cardWidth, layout.cardHeight };
}

Rect PlaceActorsResponsiveGrid::PreviewRect(
    const PlaceActorsGridLayout& layout,
    const Rect& cardRect)
{
    return Rect{ cardRect.x, cardRect.y, layout.previewSize, layout.previewSize };
}

Rect PlaceActorsResponsiveGrid::LabelRect(
    const PlaceActorsGridLayout& layout,
    const Rect& cardRect)
{
    const float labelY = cardRect.y + layout.previewSize + (layout.cardHeight - layout.previewSize) * 0.15f;
    const float labelH = std::max(12.0f, cardRect.y + cardRect.height - labelY);
    return Rect{ cardRect.x, labelY, cardRect.width, labelH };
}

float PlaceActorsResponsiveGrid::ContentHeight(const PlaceActorsGridLayout& layout, int itemCount) {
    if (itemCount <= 0 || layout.columns <= 0) {
        return 0.0f;
    }
    const int rows = (itemCount + layout.columns - 1) / layout.columns;
    return static_cast<float>(rows) * layout.cardHeight
        + static_cast<float>(std::max(0, rows - 1)) * layout.cardGap;
}

} // namespace we::programs::editor
