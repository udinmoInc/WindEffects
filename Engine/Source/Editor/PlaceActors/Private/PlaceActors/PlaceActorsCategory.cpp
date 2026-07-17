#include "PlaceActors/PlaceActorsCategory.h"

#include "PlaceActors/ActorsPanelLayout.h"
#include "PlaceActors/ActorsPanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeToken.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"

#include <algorithm>

namespace we::programs::editor {

using we::runtime::kindui::Color;
using we::runtime::kindui::PaintContext;
using we::runtime::kindui::Point;
using we::runtime::kindui::Rect;
using we::runtime::kindui::ThemeToken;
namespace WEIcons = we::runtime::kindui::Icons;

float PlaceActorsCategory::MeasureHeaderHeight(float configuredHeight) {
    (void)configuredHeight;
    return ActorsPanelLayout::CategoryHeight();
}

void PlaceActorsCategory::PaintSectionBackground(PaintContext& context, const Rect& bounds) {
    ActorsPanelChrome::PaintSectionBackground(context, bounds);
}

void PlaceActorsCategory::PaintEmptyState(PaintContext& context, const Rect& bounds, const std::string& message) {
    const float fontSize = ResolveThemeMetric(ThemeToken::TextSizeSmall);
    const float x = bounds.x + ActorsPanelLayout::ContentPadH();
    const float y = bounds.y + (bounds.height - fontSize) * 0.5f;
    context.DrawText(message, Point{ x, y }, ResolveThemeColor(ThemeToken::TextSecondary), fontSize);
}

void PlaceActorsCategory::PaintHeader(PaintContext& context,
                                      const Rect& bounds,
                                      const std::string& label,
                                      const std::string& iconName,
                                      bool expanded,
                                      float hoverAnim,
                                      float expandAnim,
                                      bool isFavoritesSection,
                                      bool showChevron)
{
    (void)expandAnim;
    const float uiScale = std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
    const float padH = ActorsPanelLayout::ContentPadH();
    const float chevronSize = ActorsPanelLayout::ChevronSize();
    const float fontSize = ResolveThemeMetric(ThemeToken::TextSizeBody) * uiScale;
    const float centerY = bounds.y + bounds.height * 0.5f;

    ActorsPanelChrome::PaintCategoryHeaderBackground(context, bounds, hoverAnim);

    float cursorX = bounds.x + padH;
    if (showChevron) {
        const Rect chevronRect{ cursorX, centerY - chevronSize * 0.5f, chevronSize, chevronSize };
        ActorsPanelChrome::PaintChevron(context, chevronRect, expanded, hoverAnim);
        cursorX += chevronSize + ResolveThemeMetric(ThemeToken::Space1) * uiScale;
    }

    const float iconDraw = ActorsPanelLayout::IconSize();
    if (isFavoritesSection) {
        we::runtime::kindui::IconPainter::DrawIcon(
            context,
            WEIcons::StarName,
            Rect{ cursorX, centerY - iconDraw * 0.5f, iconDraw, iconDraw },
            ResolveThemeColor(ThemeToken::Warning));
        cursorX += iconDraw + ResolveThemeMetric(ThemeToken::Space1) * uiScale;
    } else if (!iconName.empty()) {
        we::runtime::kindui::IconPainter::DrawIcon(
            context,
            iconName,
            Rect{ cursorX, centerY - iconDraw * 0.5f, iconDraw, iconDraw },
            ResolveThemeColor(ThemeToken::IconPrimary));
        cursorX += iconDraw + ResolveThemeMetric(ThemeToken::Space1) * uiScale;
    }

    // Emphasize pinned/non-collapsible sections (Quick Access) and Favorites.
    const bool emphasize = isFavoritesSection || !showChevron;
    const Color titleColor = emphasize
        ? ResolveThemeColor(ThemeToken::TextPrimary)
        : ResolveThemeColor(ThemeToken::TextSecondary);
    context.DrawText(
        label,
        Point{ cursorX, centerY - fontSize * 0.5f },
        titleColor,
        fontSize,
        emphasize);
}

} // namespace we::programs::editor
