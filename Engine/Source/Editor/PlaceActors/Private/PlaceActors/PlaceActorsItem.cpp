#include "PlaceActors/PlaceActorsItem.h"

#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"
#include "EditorToolsRegistry.h"

namespace we::programs::editor {

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::Size;
using WindEffects::Editor::UI::ThemeToken;

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
    const float labelFontSize = 11.0f * uiScale;
    Color bg = ResolveThemeColor(ThemeToken::PanelBackground);
    if (selected) {
        bg = ResolveThemeColor(ThemeToken::SelectedBackground);
    } else if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveThemeColor(ThemeToken::HoverBackground), hoverAnim);
    }
    if (pressAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveThemeColor(ThemeToken::PressedBackground), pressAnim);
    }

    context.DrawRoundedRect(bounds, bg, metrics.cornerRadius);
    if (selected || hoverAnim > 0.2f) {
        context.DrawRoundedRectOutline(bounds, ResolveThemeColor(ThemeToken::AccentPrimary), 1.0f, metrics.cornerRadius);
    }

    const float iconDraw = std::min(metrics.iconSize, bounds.width - 16.0f);
    const float iconX = bounds.x + (bounds.width - iconDraw) * 0.5f;
    const float iconY = bounds.y + 10.0f;
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, item.iconName, Rect{ iconX, iconY, iconDraw, iconDraw }, ResolveThemeColor(ThemeToken::TextPrimary));

    const float textY = bounds.y + bounds.height - 22.0f;
    const float textWidth = context.GetTextWidth(item.label, labelFontSize, true);
    const float textX = bounds.x + std::max(4.0f, (bounds.width - textWidth) * 0.5f);
    context.DrawText(item.label, Point{ textX, textY }, ResolveThemeColor(ThemeToken::TextPrimary), labelFontSize, true);

    if (favorite) {
        WindEffects::Editor::UI::IconPainter::DrawIcon(context, WindEffects::Editor::UI::Icons::StarFilledName,
            Rect{ bounds.x + bounds.width - 18.0f, bounds.y + 6.0f, 12.0f, 12.0f }, ResolveThemeColor(ThemeToken::Warning));
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
    const float uiScale = (std::max)(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float labelFontSize = 11.0f * uiScale;
    Color bg = ResolveThemeColor(ThemeToken::PanelBackground);
    if (selected) {
        bg = ResolveThemeColor(ThemeToken::SelectedBackground);
    } else if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveThemeColor(ThemeToken::HoverBackground), hoverAnim);
    }
    if (pressAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveThemeColor(ThemeToken::PressedBackground), pressAnim * 0.5f);
    }

    context.DrawRoundedRect(bounds, bg, ResolveThemeMetric(ThemeToken::CornerRadiusMedium));

    const float iconSize = ResolveThemeMetric(ThemeToken::IconSizeToolbar) + ResolveThemeMetric(ThemeToken::Space3);
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, item.iconName,
        Rect{ bounds.x + ResolveThemeMetric(ThemeToken::Space2), bounds.y + (bounds.height - iconSize) * 0.5f, iconSize, iconSize }, ResolveThemeColor(ThemeToken::TextPrimary));

    const float captionFontSize = ResolveThemeMetric(ThemeToken::TextSizeCaption) * uiScale;
    context.DrawText(item.label, Point{ bounds.x + ResolveThemeMetric(ThemeToken::Space6) - 4.0f, bounds.y + ResolveThemeMetric(ThemeToken::Space2) }, ResolveThemeColor(ThemeToken::TextPrimary), labelFontSize, true);
    if (!item.description.empty()) {
        context.DrawText(item.description, Point{ bounds.x + ResolveThemeMetric(ThemeToken::Space6) - 4.0f, bounds.y + ResolveThemeMetric(ThemeToken::Space4) }, ResolveThemeColor(ThemeToken::TextSecondary), captionFontSize);
    }

    if (favorite) {
        WindEffects::Editor::UI::IconPainter::DrawIcon(context, WindEffects::Editor::UI::Icons::StarFilledName,
            Rect{ bounds.x + bounds.width - ResolveThemeMetric(ThemeToken::Space5) - 2.0f, bounds.y + (bounds.height - ResolveThemeMetric(ThemeToken::IconSizeTree)) * 0.5f, ResolveThemeMetric(ThemeToken::IconSizeTree), ResolveThemeMetric(ThemeToken::IconSizeTree) }, ResolveThemeColor(ThemeToken::Warning));
    }
}

} // namespace we::programs::editor
