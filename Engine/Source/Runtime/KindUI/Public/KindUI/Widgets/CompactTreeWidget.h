// ==============================================================================
// WindEffects — KindUI — CompactTreeWidget
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Layout/ScrollViewport.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::kindui {

struct KINDUI_API CompactTreeNode {
    std::string id;
    std::string label;
    std::string category;
    std::string parentId;
    WindIconRef icon = kWindIconNone;
    int depth = 0;
    bool hasChildren = false;
    bool expanded = true;
    Rect rect{};
    Rect expanderRect{};
};

/// Reusable compact tree view for component sub-outliners and hierarchy trees.
class KINDUI_API CompactTreeWidget : public Widget {
public:
    CompactTreeWidget();
    ~CompactTreeWidget() override;

    void SetItems(std::vector<CompactTreeNode> items);
    void SetActiveCategory(std::string category);

    void SetOnItemClicked(std::function<void(const CompactTreeNode& item)> cb);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseWheel(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseDown(const MouseEvent& event) override;

private:
    bool IsItemVisible(const CompactTreeNode& item) const;
    size_t GetVisibleItemCount() const;
    float CalculateContentHeight(float scale) const;

    std::vector<CompactTreeNode> m_Items;
    std::string m_ActiveCategory;
    std::string m_HoveredId;
    std::unordered_map<std::string, bool> m_ExpandedState;
    ScrollViewport m_ScrollViewport;
    std::function<void(const CompactTreeNode& item)> m_OnItemClicked;
};

} // namespace we::runtime::kindui
