#include "PlaceActors/PlaceActorsItem.h"

#include "PlaceActors/ActorsPanelLayout.h"
#include "PlaceActors/ActorsPanelChrome.h"
#include "PlaceActors/PlaceActorsSearch.h"
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
        context, iconName, iconRect, ResolveThemeColor(ThemeToken::IconDefault));
}

} // namespace

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
    const float uiScale = std::max(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float labelFontSize = ResolveThemeMetric(ThemeToken::TextSizeSmall) * uiScale;
    const bool hovered = hoverAnim > 0.01f || pressAnim > 0.01f;

    ActorsPanelChrome::PaintActorRowBackground(context, bounds, hoverAnim, pressAnim, selected);

    const float iconDraw = std::min(metrics.iconSize, bounds.width - ActorsPanelLayout::ContentPadH() * 2.0f);
    const float iconX = bounds.x + (bounds.width - iconDraw) * 0.5f;
    const float iconY = bounds.y + ActorsPanelLayout::ContentPadH();
    PaintItemIcon(context, item.iconName, Rect{ iconX, iconY, iconDraw, iconDraw });

    const float textY = bounds.y + bounds.height - labelFontSize - ActorsPanelLayout::ContentPadH();
    const float textWidth = context.GetTextWidth(item.label, labelFontSize);
    const float textX = bounds.x + std::max(ActorsPanelLayout::ContentPadH(), (bounds.width - textWidth) * 0.5f);
    context.DrawText(item.label, Point{ textX, textY }, ResolveThemeColor(ThemeToken::TextPrimary), labelFontSize);

    if (favorite) {
        const float starSize = static_cast<float>(WindEffects::Editor::UI::IconMetrics::NativeIconTierPx(
            ActorsPanelLayout::IconSize() * 0.75f));
        WindEffects::Editor::UI::IconPainter::DrawIcon(
            context,
            WEIcons::StarName,
            Rect{ bounds.x + bounds.width - starSize - 4.0f, bounds.y + 4.0f, starSize, starSize },
            ResolveThemeColor(ThemeToken::Warning));
    }
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
    const float iconSize = ActorsPanelLayout::IconSize();

    const Rect rowRect{
        bounds.x + 2.0f,
        bounds.y + 1.0f,
        std::max(0.0f, bounds.width - 4.0f),
        std::max(0.0f, bounds.height - 2.0f)
    };
    ActorsPanelChrome::PaintActorRowBackground(context, rowRect, hoverAnim, pressAnim, selected);

    const float iconX = ActorsPanelLayout::ItemIconX(bounds.x);
    const float iconY = bounds.y + (bounds.height - iconSize) * 0.5f;
    PaintItemIcon(context, item.iconName, Rect{ iconX, iconY, iconSize, iconSize });

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

    const float starSize = static_cast<float>(WindEffects::Editor::UI::IconMetrics::NativeIconTierPx(
        ActorsPanelLayout::IconSize() * 0.75f));
    const float starX = ActorsPanelLayout::StarIconX(bounds.x, bounds.width);
    const float starY = bounds.y + (bounds.height - starSize) * 0.5f;
    if (favorite || hoverAnim > 0.01f) {
        const Color starColor = favorite
            ? ResolveThemeColor(ThemeToken::Warning)
            : ResolveThemeColor(ThemeToken::IconDefault);
        WindEffects::Editor::UI::IconPainter::DrawIcon(
            context,
            WEIcons::StarName,
            Rect{ starX, starY, starSize, starSize },
            starColor);
    }
}

} // namespace we::programs::editor
