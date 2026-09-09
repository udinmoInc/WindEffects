// ==============================================================================
// WindEffects — KindUI — VerticalDivider
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

/// Thin vertical separator for toolbars and panel chrome rows.
class KINDUI_API VerticalDivider : public Widget {
public:
    VerticalDivider();

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetHeightRatio(float ratio) { m_HeightRatio = ratio; InvalidateLayout(); }
    float GetHeightRatio() const { return m_HeightRatio; }

    void SetExplicitHeight(float h) { m_ExplicitHeight = h; InvalidateLayout(); }
    float GetExplicitHeight() const { return m_ExplicitHeight; }

private:
    float m_HeightRatio = 1.0f;
    float m_ExplicitHeight = 0.0f;
};

} // namespace we::runtime::kindui
