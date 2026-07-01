#include "PlaceActors/PlaceActorsCategory.h"

#include "Core/Theme.hpp"
#include "Core/Icon.hpp"

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
    Color bg = Color{ 0.12f, 0.12f, 0.12f, 1.0f };
    if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.HoverOverlay, hoverAnim);
    }
    context.DrawRect(bounds, bg);

    const char* chevron = expanded ? we::UI::Icons::ChevronDownName : we::UI::Icons::ChevronRightName;
    we::UI::IconPainter::DrawIcon(context, chevron, Rect{ bounds.x + 6.0f, bounds.y + 7.0f, 14.0f, 14.0f }, theme.TextSecondary);
    if (!iconName.empty()) {
        we::UI::IconPainter::DrawIcon(context, iconName, Rect{ bounds.x + 24.0f, bounds.y + 6.0f, 16.0f, 16.0f }, theme.TextPrimary);
    }
    context.DrawText(label, Point{ bounds.x + 44.0f, bounds.y + 7.0f }, theme.TextPrimary, 12.0f, true);
}

} // namespace we::programs::editor
