#include "PlaceActors/PlaceActorsCategory.h"

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"

namespace we::programs::editor {

using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::ThemeToken;

float PlaceActorsCategory::MeasureHeaderHeight(float configuredHeight) {
    (void)configuredHeight;
    return WindEffects::Editor::UI::PanelChrome::CategoryHeaderHeight();
}

void PlaceActorsCategory::PaintHeader(PaintContext& context,
                                      const Rect& bounds,
                                      const std::string& label,
                                      const std::string& iconName,
                                      bool expanded,
                                      float hoverAnim) {
    (void)iconName;
    const bool hovered = hoverAnim > 0.01f;
    WindEffects::Editor::UI::PanelChrome::PaintCategoryHeader(context, bounds, label, expanded, hovered);
}

} // namespace we::programs::editor
