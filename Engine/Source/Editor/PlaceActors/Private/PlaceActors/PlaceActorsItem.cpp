#include "PlaceActors/PlaceActorsItem.h"

#include "PlaceActors/ActorsPanelLayout.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"

namespace we::programs::editor {

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::Size;
using WindEffects::Editor::UI::ThemeToken;
namespace PanelChrome = WindEffects::Editor::UI::PanelChrome;

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
    const float uiScale = (std::max)(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float labelFontSize = ResolveThemeMetric(ThemeToken::TextSizeSmall) * uiScale;
    const bool hovered = hoverAnim > 0.01f || pressAnim > 0.01f;

    if (selected || hovered) {
        PanelChrome::PaintListRowBackground(context, bounds, hovered, selected);
    }

    const float iconDraw = std::min(metrics.iconSize, bounds.width - ActorsPanelLayout::ContentPadH() * 2.0f);
    const float iconX = bounds.x + (bounds.width - iconDraw) * 0.5f;
    const float iconY = bounds.y + ActorsPanelLayout::ContentPadH();
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, item.iconName,
        Rect{ iconX, iconY, iconDraw, iconDraw }, ResolveThemeColor(ThemeToken::IconDefault));

    const float textY = bounds.y + bounds.height - labelFontSize - ActorsPanelLayout::ContentPadH();
    const float textWidth = context.GetTextWidth(item.label, labelFontSize);
    const float textX = bounds.x + std::max(ActorsPanelLayout::ContentPadH(), (bounds.width - textWidth) * 0.5f);
    context.DrawText(item.label, Point{ textX, textY }, ResolveThemeColor(ThemeToken::TextPrimary), labelFontSize);

    if (favorite) {
        const float starSize = ActorsPanelLayout::IconSize();
        WindEffects::Editor::UI::IconPainter::DrawIcon(context, WindEffects::Editor::UI::Icons::StarFilledName,
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
                                bool favorite) {
    (void)metrics;
    const float uiScale = (std::max)(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float labelFontSize = ResolveThemeMetric(ThemeToken::TextSizeBody) * uiScale;
    const float iconSize = ActorsPanelLayout::IconSize();
    const bool hovered = hoverAnim > 0.01f || pressAnim > 0.01f;

    if (selected || hovered) {
        PanelChrome::PaintListRowBackground(context, bounds, hovered, selected);
    }

    const float iconX = ActorsPanelLayout::ItemIconX(bounds.x);
    const float iconY = bounds.y + (bounds.height - iconSize) * 0.5f;
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, item.iconName,
        Rect{ iconX, iconY, iconSize, iconSize }, ResolveThemeColor(ThemeToken::IconDefault));

    context.DrawText(item.label,
        Point{ ActorsPanelLayout::LabelX(bounds.x), bounds.y + (bounds.height - labelFontSize) * 0.5f },
        ResolveThemeColor(ThemeToken::TextPrimary), labelFontSize);

    const float starX = ActorsPanelLayout::StarIconX(bounds.x, bounds.width);
    const char* starIcon = favorite ? WindEffects::Editor::UI::Icons::StarFilledName : WindEffects::Editor::UI::Icons::StarName;
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, starIcon,
        Rect{ starX, bounds.y + (bounds.height - iconSize) * 0.5f, iconSize, iconSize },
        favorite ? ResolveThemeColor(ThemeToken::Warning) : ResolveThemeColor(ThemeToken::IconDefault));
}

} // namespace we::programs::editor
