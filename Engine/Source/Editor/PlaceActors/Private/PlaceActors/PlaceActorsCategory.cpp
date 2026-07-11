#include "PlaceActors/PlaceActorsCategory.h"

#include "Core/Theme.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"

namespace we::programs::editor {

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::Theme;

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
    const float uiScale = (std::max)(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float fontSize = theme.TextSizeSmall * uiScale;
    Color bg = theme.HeaderBackground;
    if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.HoverBg, hoverAnim);
    }
    context.DrawRect(bounds, bg);

    const char* chevron = expanded ? WindEffects::Editor::UI::Icons::ChevronDownName : WindEffects::Editor::UI::Icons::ChevronRightName;
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, chevron,
        Rect{ bounds.x + theme.Space2 - 2.0f, bounds.y + theme.Space2 - 1.0f, theme.IconSizeTree, theme.IconSizeTree }, theme.TextSecondary);
    if (!iconName.empty()) {
        WindEffects::Editor::UI::IconPainter::DrawIcon(context, iconName,
            Rect{ bounds.x + theme.Space6, bounds.y + theme.Space2 - 2.0f, theme.IconSizeToolbar, theme.IconSizeToolbar }, theme.TextPrimary);
    }
    const float textX = iconName.empty()
        ? bounds.x + theme.Space6
        : bounds.x + theme.Space6 + theme.IconSizeToolbar + theme.Space2;
    context.DrawText(label, Point{ textX, bounds.y + theme.Space2 - 1.0f }, theme.TextPrimary, fontSize, true);
}

} // namespace we::programs::editor
