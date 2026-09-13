// ==============================================================================
// WindEffects — KindUI — TreeColumnHeader
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"

namespace we::runtime::kindui {

/// Explorer tree column header row (Item Label / Type) as its own layout slot.
class KINDUI_API TreeColumnHeader : public Widget {
public:
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
};

} // namespace we::runtime::kindui
