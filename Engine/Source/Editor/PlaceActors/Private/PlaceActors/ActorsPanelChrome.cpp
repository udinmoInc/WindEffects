#include "PlaceActors/ActorsPanelChrome.h"

#include "PlaceActors/ActorsPanelLayout.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Rendering/IconMetrics.h"

namespace we::programs::editor::ActorsPanelChrome {

using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::WindIconRef;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::ColorToken;
namespace PanelChrome = ::we::editor::panels::PanelChrome;

void PaintActorRowBackground(
    PaintContext& context,
    const Rect& rowRect,
    float hoverAnim,
    float pressAnim,
    bool selected) {
    const bool hovered = hoverAnim > 0.01f || pressAnim > 0.01f;
    if (selected) {
        PanelChrome::PaintListRowBackground(context, rowRect, hovered, true, false);
        return;
    }
    if (hovered) {
        we::runtime::kindui::ControlChrome::PaintInteractiveFill(
            context,
            rowRect,
            ActorsPanelLayout::RowRadius(),
            hoverAnim,
            pressAnim,
            false,
            ColorToken::SecondarySurface);
    }
}

void PaintCategoryHeaderBackground(PaintContext& context, const Rect& bounds, float hoverAnim) {
    const Color bg = hoverAnim > 0.01f
        ? we::runtime::kindui::ResolveColor(ColorToken::HoverBackground)
        : we::runtime::kindui::ds::Panel::ListLabelBandBackground();
    context.DrawRect(bounds, bg);
}

void PaintSectionBackground(PaintContext& context, const Rect& bounds) {
    PanelChrome::PaintContentWell(context, bounds);
}

void PaintSoftSeparator(PaintContext& context, const Rect& bounds) {
    const float y = bounds.y + bounds.height * 0.5f;
    context.DrawLine(
        Point{ bounds.x + ActorsPanelLayout::ContentPadH(), y },
        Point{ bounds.x + bounds.width - ActorsPanelLayout::ContentPadH(), y },
        we::runtime::kindui::ResolveColor(ColorToken::Separator),
        1.0f);
}

void PaintChevron(PaintContext& context, const Rect& bounds, bool expanded, float hoverAnim) {
    const Color color = we::runtime::kindui::ResolveTextForState(hoverAnim > 0.01f, false);
    const WindIconRef chevronIcon = expanded
        ? we::runtime::kindui::WindIcons::ChevronDown16
        : we::runtime::kindui::WindIcons::ChevronRight16;
    we::runtime::kindui::IconPainter::Draw(context, chevronIcon, bounds);
}

} // namespace we::programs::editor::ActorsPanelChrome
