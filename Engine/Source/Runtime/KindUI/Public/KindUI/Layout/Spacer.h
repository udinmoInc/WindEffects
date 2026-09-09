// ==============================================================================
// WindEffects — KindUI — Spacer
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Widget.h"

namespace we::runtime::kindui {

class KINDUI_API Spacer : public Widget {
public:
    Spacer();
    ~Spacer() override;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
};

} // namespace we::runtime::kindui
