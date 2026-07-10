#include "PlaceActors/PlaceActorsCategory.h"

#include "Core/Theme.h"
#include "Core/Icon.h"

namespace we::programs::editor {

using we::UI::Color;
using we::UI::PaintContext;
using we::UI::Point;
using we::UI::Rect;
using we::UI::Theme;

float PlaceActorsCategory::MeasureHeaderHeight(float configuredHeight) {
    return configuredHeight;
}

void PlaceActorsCategory::PaintHeader(PaintContext& context,
                                      const Rect& bounds,
                                      const std::string& label,
                                      const std::string& iconName,
                                      bool expanded,
                                      float hoverAnim) {
    const auto& theme = Theme::Get();
    Color bg = theme.HeaderBackground;
    if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.HoverBg, hoverAnim);
    }
    context.DrawRect(bounds, bg);

    const char* chevron = expanded ? we::UI::Icons::ChevronDownName : we::UI::Icons::ChevronRightName;
    we::UI::IconPainter::DrawIcon(context, chevron,
        Rect{ bounds.x + theme.Space2 - 2.0f, bounds.y + theme.Space2 - 1.0f, theme.IconSizeTree, theme.IconSizeTree }, theme.TextSecondary);
    if (!iconName.empty()) {
        we::UI::IconPainter::DrawIcon(context, iconName,
            Rect{ bounds.x + theme.Space6, bounds.y + theme.Space2 - 2.0f, theme.IconSizeToolbar, theme.IconSizeToolbar }, theme.TextPrimary);
    }
    context.DrawText(label, Point{ bounds.x + theme.Space6 - 4.0f, bounds.y + theme.Space2 - 1.0f }, theme.TextPrimary, theme.TextSizeSmall, true);
}

} // namespace we::programs::editor
