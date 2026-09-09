// ==============================================================================
// WindEffects — PlaceActors — ActorsPanelChrome
// Internal implementation for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Geometry.h"
#include <string>

namespace we::programs::editor::ActorsPanelChrome {

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
