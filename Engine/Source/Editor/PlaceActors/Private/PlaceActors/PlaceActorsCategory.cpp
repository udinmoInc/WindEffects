#include "PlaceActors/PlaceActorsCategory.h"

#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"

namespace we::programs::editor {

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::ThemeToken;

float PlaceActorsCategory::MeasureHeaderHeight(float configuredHeight) {
    return configuredHeight;
}

void PlaceActorsCategory::PaintHeader(PaintContext& context,
                                      const Rect& bounds,
                                      const std::string& label,
                                      const std::string& iconName,
                                      bool expanded,
                                      float hoverAnim) {
    const float uiScale = (std::max)(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float fontSize = ResolveThemeMetric(ThemeToken::TextSizeSmall) * uiScale;
    Color bg = ResolveThemeColor(ThemeToken::HeaderBackground);
    if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveThemeColor(ThemeToken::HoverBackground), hoverAnim);
    }
    context.DrawRect(bounds, bg);

    const char* chevron = expanded ? WindEffects::Editor::UI::Icons::ChevronDownName : WindEffects::Editor::UI::Icons::ChevronRightName;
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, chevron,
        Rect{ bounds.x + ResolveThemeMetric(ThemeToken::Space2) - 2.0f, bounds.y + ResolveThemeMetric(ThemeToken::Space2) - 1.0f, ResolveThemeMetric(ThemeToken::IconSizeTree), ResolveThemeMetric(ThemeToken::IconSizeTree) }, ResolveThemeColor(ThemeToken::TextSecondary));
    if (!iconName.empty()) {
        WindEffects::Editor::UI::IconPainter::DrawIcon(context, iconName,
            Rect{ bounds.x + ResolveThemeMetric(ThemeToken::Space6), bounds.y + ResolveThemeMetric(ThemeToken::Space2) - 2.0f, ResolveThemeMetric(ThemeToken::IconSizeToolbar), ResolveThemeMetric(ThemeToken::IconSizeToolbar) }, ResolveThemeColor(ThemeToken::TextPrimary));
    }
    const float textX = iconName.empty()
        ? bounds.x + ResolveThemeMetric(ThemeToken::Space6)
        : bounds.x + ResolveThemeMetric(ThemeToken::Space6) + ResolveThemeMetric(ThemeToken::IconSizeToolbar) + ResolveThemeMetric(ThemeToken::Space2);
    context.DrawText(label, Point{ textX, bounds.y + ResolveThemeMetric(ThemeToken::Space2) - 1.0f }, ResolveThemeColor(ThemeToken::TextPrimary), fontSize, true);
}

} // namespace we::programs::editor
