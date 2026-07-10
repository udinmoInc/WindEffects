#include "PlaceActors/PlaceActorsItem.h"

#include "Core/Theme.h"
#include "Core/Icon.h"
#include "EditorToolsRegistry.h"

namespace we::programs::editor {

using we::UI::Color;
using we::UI::PaintContext;
using we::UI::Point;
using we::UI::Rect;
using we::UI::Size;
using we::UI::Theme;

Size PlaceActorsItem::MeasureGrid(const PlaceActorsItemMetrics& metrics) {
    return Size{ metrics.cardSize, metrics.cardSize };
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
    const auto& theme = Theme::Get();
    Color bg = theme.PanelBackground;
    if (selected) {
        bg = theme.SelectedBg;
    } else if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.HoverBg, hoverAnim);
    }
    if (pressAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.PressedOverlay, pressAnim);
    }

    context.DrawRoundedRect(bounds, bg, metrics.cornerRadius);
    if (selected || hoverAnim > 0.2f) {
        context.DrawRoundedRectOutline(bounds, theme.SelectedAccent, 1.0f, metrics.cornerRadius);
    }

    const float iconDraw = std::min(metrics.iconSize, bounds.width - 16.0f);
    const float iconX = bounds.x + (bounds.width - iconDraw) * 0.5f;
    const float iconY = bounds.y + 10.0f;
    we::UI::IconPainter::DrawIcon(context, item.iconName, Rect{ iconX, iconY, iconDraw, iconDraw }, theme.TextPrimary);

    const float textY = bounds.y + bounds.height - 22.0f;
    const float textWidth = context.GetTextWidth(item.label, 11.0f);
    const float textX = bounds.x + std::max(4.0f, (bounds.width - textWidth) * 0.5f);
    context.DrawText(item.label, Point{ textX, textY }, theme.TextPrimary, 11.0f, true);

    if (favorite) {
        we::UI::IconPainter::DrawIcon(context, we::UI::Icons::StarFilledName,
            Rect{ bounds.x + bounds.width - 18.0f, bounds.y + 6.0f, 12.0f, 12.0f }, theme.Warning);
    }
}

void PlaceActorsItem::PaintList(PaintContext& context,
                                const Rect& bounds,
                                const PlaceActorsItemData& item,
                                const PlaceActorsItemMetrics& metrics,
                                float hoverAnim,
                                float pressAnim,
                                bool selected,
                                bool favorite) {
    const auto& theme = Theme::Get();
    Color bg = theme.PanelBackground;
    if (selected) {
        bg = theme.SelectedBg;
    } else if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.HoverBg, hoverAnim);
    }
    if (pressAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.PressedOverlay, pressAnim * 0.5f);
    }

    context.DrawRoundedRect(bounds, bg, theme.CornerRadiusMedium);

    const float iconSize = theme.IconSizeToolbar + theme.Space3;
    we::UI::IconPainter::DrawIcon(context, item.iconName,
        Rect{ bounds.x + theme.Space2, bounds.y + (bounds.height - iconSize) * 0.5f, iconSize, iconSize }, theme.TextPrimary);

    context.DrawText(item.label, Point{ bounds.x + theme.Space6 - 4.0f, bounds.y + theme.Space2 }, theme.TextPrimary, theme.TextSizeSmall, true);
    if (!item.description.empty()) {
        context.DrawText(item.description, Point{ bounds.x + theme.Space6 - 4.0f, bounds.y + theme.Space4 }, theme.TextSecondary, theme.TextSizeCaption);
    }

    if (favorite) {
        we::UI::IconPainter::DrawIcon(context, we::UI::Icons::StarFilledName,
            Rect{ bounds.x + bounds.width - theme.Space5 - 2.0f, bounds.y + (bounds.height - theme.IconSizeTree) * 0.5f, theme.IconSizeTree, theme.IconSizeTree }, theme.Warning);
    }
}

} // namespace we::programs::editor
