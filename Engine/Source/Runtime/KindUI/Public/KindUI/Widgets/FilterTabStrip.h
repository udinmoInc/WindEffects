// ==============================================================================
// WindEffects — KindUI — FilterTabStrip
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
#include <functional>
#include <string>
#include <vector>

namespace we::runtime::kindui {

/// Reusable category and mode tab filter bar component.
class KINDUI_API FilterTabStrip : public Widget {
public:
    FilterTabStrip();
    ~FilterTabStrip() override;

    void SetTabs(std::vector<std::string> tabs);
    void SetActiveTab(std::string activeTab);
    [[nodiscard]] const std::string& GetActiveTab() const { return m_ActiveTab; }

    void SetOnTabSelected(std::function<void(const std::string& tab)> cb);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseDown(const MouseEvent& event) override;

private:
    struct TabSlot {
        std::string label;
        Rect rect;
    };

    float LayoutTabsForWidth(float originX, float originY, float availableW);
    [[nodiscard]] std::string TabAt(const Point& pos) const;

    std::vector<std::string> m_TabLabels;
    std::vector<TabSlot> m_Tabs;
    std::string m_ActiveTab;
    std::string m_HoveredTab;
    std::function<void(const std::string& tab)> m_OnTabSelected;
};

} // namespace we::runtime::kindui
