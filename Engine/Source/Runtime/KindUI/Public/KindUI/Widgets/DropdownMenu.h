// ==============================================================================
// WindEffects — KindUI — DropdownMenu
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Widgets/MenuBar.h"
#include <vector>
#include <memory>
#include "KindUI/Core/Style.h"

namespace we::runtime::kindui {

class KINDUI_API DropdownMenu : public Widget {
public:
    DropdownMenu(const std::vector<std::shared_ptr<MenuItem>>& items);
    virtual ~DropdownMenu() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    void OnHoverLost() override { m_HoveredItem = -1; }

private:
    std::vector<std::shared_ptr<MenuItem>> m_Items;
    int m_HoveredItem = -1;
    float m_ScrollOffset = 0.0f;

    int HitItemAt(const Point& pos) const;

    float m_ItemHeight = 0.0f;
    float m_PaddingY = 0.0f;
    float m_PaddingX = 0.0f;
};

} // namespace we::runtime::kindui
