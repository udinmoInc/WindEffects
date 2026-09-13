// ==============================================================================
// WindEffects — KindUI — CollapsibleGroup
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Expansion.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/Flex.h"
#include <functional>
#include <memory>
#include <string>

namespace we::runtime::kindui {

/// Reusable collapsible group container with header, expand chevron, and content stack.
class KINDUI_API CollapsibleGroup : public Column, public IExpansionNode {
public:
    CollapsibleGroup(std::string title = "", bool expanded = true);
    ~CollapsibleGroup() override;

    void SetTitle(std::string title);
    void SetExpanded(bool expanded);
    [[nodiscard]] bool IsExpanded() const override { return m_Expanded; }
    void ApplyExpanded(bool expanded) override;

    void SetOnExpandedChanged(std::function<void(bool expanded)> cb);

    void AddContentChild(const std::shared_ptr<Widget>& child);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnHoverLost() override;
    void OnMouseDown(const MouseEvent& event) override;

private:
    std::string m_Title;
    bool m_Expanded = true;
    bool m_HeaderHovered = false;
    std::shared_ptr<Column> m_ContentColumn;
    std::function<void(bool expanded)> m_OnExpandedChanged;
};

} // namespace we::runtime::kindui
