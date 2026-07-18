#pragma once

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Geometry.h"
#include <string>

namespace we::programs::editor::ActorsPanelChrome {

void PaintSearchField(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    const std::string& placeholder,
    const std::string& text,
    bool focused,
    bool showCaret = false);

void PaintActorRowBackground(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& rowRect,
    float hoverAnim,
    float pressAnim,
    bool selected);

void PaintCategoryHeaderBackground(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    float hoverAnim);

void PaintSectionBackground(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds);

void PaintSoftSeparator(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds);

void PaintChevron(
    we::runtime::kindui::PaintContext& context,
    const we::runtime::kindui::Rect& bounds,
    bool expanded,
    float hoverAnim);

} // namespace we::programs::editor::ActorsPanelChrome
