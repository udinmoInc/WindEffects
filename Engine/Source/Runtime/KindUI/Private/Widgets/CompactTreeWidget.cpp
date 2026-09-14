// ==============================================================================
// WindEffects — KindUI — CompactTreeWidget
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/CompactTreeWidget.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/UI/AutoAlign.h"
#include "KindUI/UI/PanelChrome.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Theme/DesignToken.h"

#include <algorithm>

namespace we::runtime::kindui {

CompactTreeWidget::CompactTreeWidget() = default;
CompactTreeWidget::~CompactTreeWidget() = default;

void CompactTreeWidget::SetItems(std::vector<CompactTreeNode> items) {
    m_Items = std::move(items);
    RebuildFlatIndices();
    InvalidateLayout();
    InvalidatePaint();
}

void CompactTreeWidget::SetActiveCategory(std::string category) {
    if (m_ActiveCategory != category) {
        m_ActiveCategory = std::move(category);
        InvalidatePaint();
    }
}

void CompactTreeWidget::SetOnItemClicked(std::function<void(const CompactTreeNode& item)> cb) {
    m_OnItemClicked = std::move(cb);
}

bool CompactTreeWidget::IsItemVisible(const CompactTreeNode& item) const {
    if (item.parentId.empty()) {
        return true;
    }
    auto it = m_ExpandedState.find(item.parentId);
    if (it != m_ExpandedState.end() && !it->second) {
        return false;
    }
    return true;
}

void CompactTreeWidget::RebuildFlatIndices() {
    m_FlatIndices.clear();
    m_FlatIndices.reserve(m_Items.size());
    for (size_t i = 0; i < m_Items.size(); ++i) {
        if (IsItemVisible(m_Items[i])) {
            m_FlatIndices.push_back(i);
        } else {
            m_Items[i].rect = {};
            m_Items[i].expanderRect = {};
        }
    }
}

size_t CompactTreeWidget::GetVisibleItemCount() const {
    return m_FlatIndices.size();
}

float CompactTreeWidget::CalculateContentHeight(float scale) const {
    const float itemH = 22.0f * scale;
    const float paddingV = 1.0f * scale;
    return static_cast<float>(m_FlatIndices.size()) * itemH + paddingV * 2.0f;
}

Size CompactTreeWidget::Measure(const Size& availableSize) {
    if (!IsVisible()) {
        m_DesiredSize = Size{ availableSize.width, 0.0f };
        return m_DesiredSize;
    }
    RebuildFlatIndices();
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float contentH = CalculateContentHeight(scale);
    const float itemH = 22.0f * scale;
    const float paddingV = 1.0f * scale;
    const float defaultPanelHeight = 2.0f * itemH + paddingV * 2.0f;
    const float maxHeight = 160.0f * scale;
    const float targetH = (std::max)(defaultPanelHeight, (std::min)(contentH, maxHeight));
    m_DesiredSize = Size{ availableSize.width, targetH };
    return m_DesiredSize;
}

void CompactTreeWidget::ArrangeVisibleRows(float itemH, float paddingV, float padH, float scale) {
    const float contentH = static_cast<float>(m_FlatIndices.size()) * itemH + paddingV * 2.0f;
    m_ScrollViewport.Sync(m_Geometry.height, contentH);
    const ListVisibleRange prev = m_ActiveRange;
    m_ActiveRange = ComputeFixedVisibleRange(
        m_ScrollViewport.offset,
        m_Geometry.height,
        m_FlatIndices.size(),
        itemH,
        4);

    // Clear only rows that left the previous window (O(window), not O(N)).
    for (size_t i = 0; i < prev.count; ++i) {
        const size_t flat = prev.first + i;
        if (!m_ActiveRange.Contains(flat) && flat < m_FlatIndices.size()) {
            auto& item = m_Items[m_FlatIndices[flat]];
            item.rect = {};
            item.expanderRect = {};
        }
    }

    for (size_t i = 0; i < m_ActiveRange.count; ++i) {
        const size_t flat = m_ActiveRange.first + i;
        auto& item = m_Items[m_FlatIndices[flat]];
        const float indent = static_cast<float>(item.depth) * 14.0f * scale;
        const float contentY = paddingV + static_cast<float>(flat) * itemH;
        item.rect = Rect{
            m_Geometry.x + padH + indent,
            m_Geometry.y + contentY,
            m_Geometry.width - (padH * 2.0f + indent),
            itemH
        };
        if (item.hasChildren) {
            item.expanderRect = Rect{ item.rect.x, item.rect.y, 16.0f * scale, itemH };
        } else {
            item.expanderRect = {};
        }
    }
}

void CompactTreeWidget::Arrange(const Rect& allottedRect) {
    if (!IsVisible()) {
        m_Geometry = allottedRect;
        return;
    }
    m_Geometry = allottedRect;
    RebuildFlatIndices();
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float itemH = 22.0f * scale;
    const float paddingV = 1.0f * scale;
    const float padH = ResolveMetric(MetricToken::Space2) * scale;
    ArrangeVisibleRows(itemH, paddingV, padH, scale);
}

void CompactTreeWidget::Paint(PaintContext& context) {
    if (!IsVisible()) return;

    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float itemH = 22.0f * scale;
    const float padH = ResolveMetric(MetricToken::Space2) * scale;
    const float paddingV = 1.0f * scale;

    context.DrawSurface(m_Geometry, SurfaceRole::PanelInner, 0.0f, "CompactTreeWidget");

    if (m_FlatIndices.empty()) {
        RebuildFlatIndices();
    }
    ArrangeVisibleRows(itemH, paddingV, padH, scale);

    const float contentH = CalculateContentHeight(scale);
    const float viewportH = m_Geometry.height;

    context.PushClipRect(m_Geometry);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * scale;

    for (size_t i = 0; i < m_ActiveRange.count; ++i) {
        const auto& item = m_Items[m_FlatIndices[m_ActiveRange.first + i]];
        Rect drawRect = item.rect;
        drawRect.y -= m_ScrollViewport.offset;
        if (drawRect.height <= 0.0f) {
            continue;
        }

        const bool isSelected = item.category.empty() ? m_ActiveCategory.empty() : (m_ActiveCategory == item.category);
        const bool isHovered = (m_HoveredId == item.id);

        if (isSelected) {
            context.DrawRoundedRect(drawRect, ResolveColor(ColorToken::SelectInactiveBackground), 3.0f * scale);
        } else if (isHovered) {
            context.DrawRoundedRect(drawRect, ResolveColor(ColorToken::ControlBackgroundHover), 3.0f * scale);
        }

        const float itemPadH = ResolveMetric(MetricToken::Space1) * scale;
        const float gap = ResolveMetric(MetricToken::Space1) * scale;
        float currentX = drawRect.x + itemPadH;

        if (item.hasChildren) {
            const float chevronSize = 10.0f * scale;
            const float chevronY = drawRect.y + (drawRect.height - chevronSize) * 0.5f;
            const Rect chevronRect{ currentX, chevronY, chevronSize, chevronSize };
            const WindIconRef chevronIcon = item.expanded ? WindIcons::TriangleDown16 : WindIcons::TriangleRight16;
            IconPainter::Draw(context, chevronIcon, chevronRect, ResolveColor(ColorToken::IconSecondary));
            currentX += chevronSize + gap;
        }

        const float iconSize = 16.0f * scale;
        const float iconY = drawRect.y + (drawRect.height - iconSize) * 0.5f;
        const Rect iconRect{ currentX, iconY, iconSize, iconSize };
        if (item.icon.IsValid()) {
            IconPainter::Draw(context, item.icon, iconRect, isSelected ? ResolveColor(ColorToken::IconAccent) :
                ResolveColor(ColorToken::IconPrimary));
            currentX += iconSize + gap;
        }

        const float labelY = AutoAlign::AlignTextTopY(drawRect, fontSize);
        const Color labelColor = isSelected ? ResolveColor(ColorToken::TextPrimary) :
            ResolveColor(ColorToken::TextSecondary);
        context.DrawText(item.label, Point{ currentX, labelY }, labelColor, fontSize);
    }

    context.PopClipRect();

    if (contentH > viewportH && viewportH > 0.0f) {
        const float sbWidth = 6.0f * scale;
        const float sbMargin = 2.0f * scale;
        const Rect trackRect{
            m_Geometry.x + m_Geometry.width - sbWidth - sbMargin,
            m_Geometry.y + sbMargin,
            sbWidth,
            m_Geometry.height - sbMargin * 2.0f
        };
        const float maxScroll = contentH - viewportH;
        const float scrollRatio = maxScroll > 0.0f ? (m_ScrollViewport.offset / maxScroll) : 0.0f;
        const float minThumbH = 16.0f * scale;
        const float thumbH = (std::max)(minThumbH, trackRect.height * (viewportH / contentH));
        const float thumbY = trackRect.y + (trackRect.height - thumbH) * scrollRatio;
        const Rect thumbRect{ trackRect.x, thumbY, trackRect.width, thumbH };

        context.DrawRoundedRect(trackRect, ResolveColor(ColorToken::ScrollbarTrack), sbWidth * 0.5f);
        context.DrawRoundedRect(thumbRect, ResolveColor(ColorToken::ScrollbarThumb), sbWidth * 0.5f);
    }
}

void CompactTreeWidget::OnMouseWheel(const MouseEvent& event) {
    if (!IsVisible()) return;
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float contentH = CalculateContentHeight(scale);
    const float viewportH = m_Geometry.height;
    if (ScrollViewport::NeedsScrollbar(viewportH, contentH)) {
        m_ScrollViewport.ApplyWheel(event.wheelDeltaY != 0.0f ? event.wheelDeltaY : event.deltaY,
            36.0f * scale, viewportH, contentH);
        InvalidateLayout();
        InvalidatePaint();
    }
}

void CompactTreeWidget::OnMouseMove(const MouseEvent& event) {
    if (!IsVisible()) return;
    std::string newHover;
    for (size_t i = 0; i < m_ActiveRange.count; ++i) {
        const auto& item = m_Items[m_FlatIndices[m_ActiveRange.first + i]];
        Rect itemScrolledRect = item.rect;
        itemScrolledRect.y -= m_ScrollViewport.offset;
        if (itemScrolledRect.height > 0.0f && itemScrolledRect.Contains(event.position)) {
            newHover = item.id;
            break;
        }
    }
    if (newHover != m_HoveredId) {
        m_HoveredId = newHover;
        InvalidatePaint();
    }
}

void CompactTreeWidget::OnMouseDown(const MouseEvent& event) {
    if (!IsVisible() || event.button != MouseButton::Left) return;

    for (size_t i = 0; i < m_ActiveRange.count; ++i) {
        auto& item = m_Items[m_FlatIndices[m_ActiveRange.first + i]];
        Rect itemScrolledRect = item.rect;
        itemScrolledRect.y -= m_ScrollViewport.offset;
        if (itemScrolledRect.height <= 0.0f || !itemScrolledRect.Contains(event.position)) {
            continue;
        }
        if (item.hasChildren) {
            Rect expanderScrolledRect = item.expanderRect;
            expanderScrolledRect.y -= m_ScrollViewport.offset;
            if (expanderScrolledRect.Contains(event.position)) {
                item.expanded = !item.expanded;
                m_ExpandedState[item.id] = item.expanded;
                RebuildFlatIndices();
                InvalidateLayout();
                InvalidatePaint();
                return;
            }
        }
        m_ActiveCategory = item.category;
        if (m_OnItemClicked) {
            m_OnItemClicked(item);
        }
        InvalidatePaint();
        break;
    }
}

Size TreeColumnHeader::Measure(const Size& availableSize) {
    m_DesiredSize = Size{
        availableSize.width < 1.0e8f ? availableSize.width : 0.0f,
        panels::PanelChrome::ColumnHeaderRowHeight()
    };
    return m_DesiredSize;
}

void TreeColumnHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void TreeColumnHeader::Paint(PaintContext& context) {
    panels::PanelChrome::PaintExplorerColumnHeader(context, m_Geometry, "Item Label");
}

} // namespace we::runtime::kindui
