#pragma once

#include "Core/PaintContext.h"
#include "Core/Geometry.h"
#include <string>

namespace we::programs::editor::ActorsPanelChrome {

void PaintSearchField(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& bounds,
    const std::string& placeholder,
    const std::string& text,
    bool focused,
    bool showCaret = false);

void PaintActorRowBackground(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& rowRect,
    float hoverAnim,
    float pressAnim,
    bool selected);

void PaintCategoryHeaderBackground(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& bounds,
    float hoverAnim);

void PaintSectionBackground(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& bounds);

void PaintSoftSeparator(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& bounds);

void PaintChevron(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& bounds,
    bool expanded,
    float hoverAnim);

} // namespace we::programs::editor::ActorsPanelChrome
