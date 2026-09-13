// ==============================================================================
// WindEffects — KindUI — Breadcrumb
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"

#include <functional>
#include <string>
#include <vector>

namespace we::runtime::kindui {

class KINDUI_API Breadcrumb : public Widget {
public:
    Breadcrumb();
    virtual ~Breadcrumb() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;
    void OnHoverLost() override;

    void SetPath(const std::vector<std::string>& path);
    const std::vector<std::string>& GetPath() const { return m_PathSegments; }
    void AddCrumb(const std::string& crumb);
    void Clear();

    using OnCrumbClicked = std::function<void(size_t index)>;
    void SetOnCrumbClicked(OnCrumbClicked callback) { m_OnCrumbClicked = callback; }

private:
    struct CrumbInfo {
        std::string text;
        Rect geometry;
        float textWidth = 0.0f;
        bool hovered = false;
    };

    void UpdateCrumbMetrics();
    void CalculateLayout();
    CrumbInfo* GetCrumbAtPosition(const Point& pos);

    std::vector<CrumbInfo> m_Crumbs;
    float m_SeparatorSpacing = 8.0f;
    float m_CrumbSpacing = 4.0f;
    int m_HoveredCrumb = -1;
    float m_LastTextSize = -1.0f;
    float m_LastUiScale = -1.0f;
    bool m_CrumbMetricsDirty = true;

    OnCrumbClicked m_OnCrumbClicked;
    std::vector<std::string> m_PathSegments;
};

} // namespace we::runtime::kindui
