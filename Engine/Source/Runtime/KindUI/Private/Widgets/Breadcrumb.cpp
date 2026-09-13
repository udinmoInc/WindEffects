// ==============================================================================
// WindEffects — KindUI — Breadcrumb
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/Breadcrumb.h"

#include "KindUI/Layout/AutoAlign.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>

namespace we::runtime::kindui {

Breadcrumb::Breadcrumb() = default;

Size Breadcrumb::Measure(const Size& availableSize) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float chevronW = 12.0f * uiScale;
    const float space = 4.0f * uiScale;

    UpdateCrumbMetrics();
    float totalW = 0.0f;
    for (size_t i = 0; i < m_Crumbs.size(); ++i) {
        float textW = m_Crumbs[i].textWidth;
        totalW += textW + space + chevronW + space;
    }
    const float h = ThemeMetric(MetricToken::ToolbarLabeledHeight) * uiScale;
    m_DesiredSize = Size{ totalW, h };
    return m_DesiredSize;
}

void Breadcrumb::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    UpdateCrumbMetrics();
    CalculateLayout();
}

void Breadcrumb::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
    const float chevronSize = 12.0f * uiScale;
    const Color kHighlightColor = we::runtime::kindui::ResolveColor(ColorToken::IconPrimary);

    for (size_t i = 0; i < m_Crumbs.size(); ++i) {
        const auto& crumb = m_Crumbs[i];
        const Color textColor = (static_cast<int>(i) == m_HoveredCrumb)
            ? kHighlightColor
            : ((i == m_Crumbs.size() - 1)
                ? kHighlightColor
                : ThemeColor(ColorToken::TextSecondary));

        const Rect crumbArea{ crumb.geometry.x, m_Geometry.y, crumb.geometry.width, m_Geometry.height };
        auto crumbLayout = we::runtime::kindui::AutoAlign::ComputeIconTextLayout(
            crumbArea, 0.0f, false, crumb.text, textSize);

        context.DrawText(crumb.text, crumbLayout.textPos, textColor, textSize, false);

        // Draw chevron separator '>' after each crumb
        const float chevronX = crumb.geometry.x + crumb.geometry.width + 3.0f * uiScale;
        const Rect chevronBand{ chevronX, m_Geometry.y, chevronSize, m_Geometry.height };
        const Rect chevronRect = we::runtime::kindui::AutoAlign::NormalizeIconBounds(chevronBand, chevronSize);
        IconPainter::Draw(context, WindIcons::ChevronRight16, chevronRect, ThemeColor(ColorToken::IconSecondary));
    }
}

void Breadcrumb::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) return;
    CrumbInfo* crumb = GetCrumbAtPosition(event.position);
    if (crumb && m_OnCrumbClicked) {
        const size_t index = static_cast<size_t>(crumb - &m_Crumbs[0]);
        m_OnCrumbClicked(index);
    }
}

void Breadcrumb::OnHoverLost() {
    m_HoveredCrumb = -1;
    for (auto& crumb : m_Crumbs) crumb.hovered = false;
}

void Breadcrumb::OnMouseMove(const MouseEvent& event) {
    m_HoveredCrumb = -1;
    for (size_t i = 0; i < m_Crumbs.size(); ++i) m_Crumbs[i].hovered = false;
    if (CrumbInfo* crumb = GetCrumbAtPosition(event.position)) {
        m_HoveredCrumb = static_cast<int>(crumb - &m_Crumbs[0]);
        m_Crumbs[static_cast<size_t>(m_HoveredCrumb)].hovered = true;
    }
}

bool Breadcrumb::ShowsPointerCursor(const Point& position) const {
    for (const auto& crumb : m_Crumbs) {
        if (crumb.geometry.Contains(position)) return true;
    }
    return false;
}

void Breadcrumb::SetPath(const std::vector<std::string>& path) {
    m_PathSegments = path;
    m_Crumbs.clear();
    for (const auto& crumb : path) {
        CrumbInfo info;
        info.text = crumb;
        m_Crumbs.push_back(info);
    }
    m_CrumbMetricsDirty = true;
    CalculateLayout();
}

void Breadcrumb::AddCrumb(const std::string& crumb) {
    CrumbInfo info;
    info.text = crumb;
    m_Crumbs.push_back(info);
    m_CrumbMetricsDirty = true;
    CalculateLayout();
}

void Breadcrumb::Clear() {
    m_Crumbs.clear();
    m_CrumbMetricsDirty = true;
}

void Breadcrumb::UpdateCrumbMetrics() {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
    if (!m_CrumbMetricsDirty && m_LastTextSize == textSize && m_LastUiScale == uiScale) {
        return;
    }

    PaintContext ctx;
    for (auto& crumb : m_Crumbs) {
        crumb.textWidth = ctx.GetTextWidth(crumb.text, textSize);
    }
    m_LastTextSize = textSize;
    m_LastUiScale = uiScale;
    m_CrumbMetricsDirty = false;
}

void Breadcrumb::CalculateLayout() {
    UpdateCrumbMetrics();
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float chevronW = 12.0f * uiScale;
    const float space = 4.0f * uiScale;
    float x = m_Geometry.x;
    const float crumbH = m_Geometry.height;
    const float y = m_Geometry.y;

    for (size_t i = 0; i < m_Crumbs.size(); ++i) {
        float textW = m_Crumbs[i].textWidth;
        m_Crumbs[i].geometry = Rect{ x, y, textW, crumbH };
        x += textW + space + chevronW + space;
    }
}

Breadcrumb::CrumbInfo* Breadcrumb::GetCrumbAtPosition(const Point& pos) {
    for (auto& crumb : m_Crumbs) {
        if (crumb.geometry.Contains(pos)) return &crumb;
    }
    return nullptr;
}

} // namespace we::runtime::kindui
