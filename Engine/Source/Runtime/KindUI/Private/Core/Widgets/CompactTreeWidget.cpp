// ==============================================================================
// WindEffects — KindUI — CompactTreeWidget
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/CompactTreeWidget.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Layout/AutoAlign.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>

namespace we::runtime::kindui {

CompactTreeWidget::CompactTreeWidget() = default;
CompactTreeWidget::~CompactTreeWidget() = default;

void CompactTreeWidget::SetItems(std::vector<CompactTreeNode> items) {
    m_Items = std::move(items);
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
    if (item.parentId.empty()) return true;
    auto it = m_ExpandedState.find(item.parentId);
    if (it != m_ExpandedState.end() && !it->second) return false;
    return true;
}

size_t CompactTreeWidget::GetVisibleItemCount() const {
    size_t count = 0;
    for (const auto& item : m_Items) {
        if (IsItemVisible(item)) ++count;
    }
    return count;
}

float CompactTreeWidget::CalculateContentHeight(float scale) const {
    const float itemH = 22.0f * scale;
    const float paddingV = 4.0f * scale;
    return static_cast<float>(GetVisibleItemCount()) * itemH + paddingV * 2.0f;
}

Size CompactTreeWidget::Measure(const Size& availableSize) {
    if (!IsVisible()) {
        m_DesiredSize = Size{ availableSize.width, 0.0f };
        return m_DesiredSize;
    }

    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float itemH = 22.0f * scale;
    const float paddingV = 4.0f * scale;
    const float contentH = static_cast<float>(GetVisibleItemCount()) * itemH + paddingV * 2.0f;
    const float maxHeight = 160.0f * scale;
    m_DesiredSize = Size{ availableSize.width, (std::min)(contentH, maxHeight) };
    return m_DesiredSize;
}

void CompactTreeWidget::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float itemH = 22.0f * scale;
    const float paddingV = 4.0f * scale;
    const float padH = 8.0f * scale;

    float currentY = allottedRect.y + paddingV;
    for (auto& item : m_Items) {
        if (!IsItemVisible(item)) {
            item.rect = {};
            item.expanderRect = {};
            continue;
        }
        const float indent = static_cast<float>(item.depth) * 14.0f * scale;
        item.rect = Rect{ allottedRect.x + padH + 4.0f * scale + indent, currentY, allottedRect.width - (padH * 2.0f +
            8.0f * scale + indent), itemH };

        if (item.hasChildren) {
            item.expanderRect = Rect{ item.rect.x, item.rect.y, 16.0f * scale, itemH };
        } else {
            item.expanderRect = {};
        }

        currentY += itemH;
    }
}

void CompactTreeWidget::Paint(PaintContext& context) {
    if (!IsVisible()) return;

    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float padH = 8.0f * scale;
    const Rect cardRect{ m_Geometry.x + padH, m_Geometry.y, m_Geometry.width - padH * 2.0f, m_Geometry.height };

    context.DrawRoundedRect(cardRect, ResolveColor(ColorToken::SecondarySurface), 4.0f * scale);
    context.DrawControlOutline(cardRect, ResolveColor(ColorToken::Separator), 1.0f * scale, 4.0f * scale);

    const float contentH = CalculateContentHeight(scale);
    const float viewportH = m_Geometry.height;
    m_ScrollViewport.Sync(viewportH, contentH);

    context.PushClipRect(cardRect);

    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * scale;

    for (const auto& item : m_Items) {
        if (!IsItemVisible(item)) continue;

        Rect drawRect = item.rect;
        drawRect.y -= m_ScrollViewport.offset;

        if (drawRect.y + drawRect.height < m_Geometry.y || drawRect.y > m_Geometry.y + m_Geometry.height) {
            continue;
        }

        const bool isSelected = item.category.empty() ? m_ActiveCategory.empty() : (m_ActiveCategory == item.category);
        const bool isHovered = (m_HoveredId == item.id);

        if (isSelected) {
            context.DrawRoundedRect(drawRect, ResolveColor(ColorToken::SelectInactiveBackground), 3.0f * scale);
        } else if (isHovered) {
            context.DrawRoundedRect(drawRect, ResolveColor(ColorToken::ControlBackgroundHover), 3.0f * scale);
        }

        const float itemPadH = ResolveMetric(MetricToken::Space2) * scale;
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

    // Scrollbar track & thumb
    const float sbWidth = 4.0f * scale;
    const float sbMargin = 3.0f * scale;
    const Rect trackRect{
        cardRect.x + cardRect.width - sbWidth - sbMargin,
        cardRect.y + sbMargin,
        sbWidth,
        cardRect.height - sbMargin * 2.0f
    };

    Rect thumbRect = trackRect;
    if (contentH > viewportH && viewportH > 0.0f) {
        const float maxScroll = contentH - viewportH;
        const float scrollRatio = maxScroll > 0.0f ? (m_ScrollViewport.offset / maxScroll) : 0.0f;
        const float minThumbH = 12.0f * scale;
        const float thumbH = (std::max)(minThumbH, trackRect.height * (viewportH / contentH));
        const float thumbY = trackRect.y + (trackRect.height - thumbH) * scrollRatio;
        thumbRect = Rect{ trackRect.x, thumbY, trackRect.width, thumbH };
    }

    context.DrawRoundedRect(trackRect, ResolveColor(ColorToken::ControlBackground), sbWidth * 0.5f);
    context.DrawRoundedRect(thumbRect, ResolveColor(ColorToken::ControlBackgroundHover), sbWidth * 0.5f);
}

void CompactTreeWidget::OnMouseWheel(const MouseEvent& event) {
    if (!IsVisible()) return;
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float contentH = CalculateContentHeight(scale);
    const float viewportH = m_Geometry.height;
    if (ScrollViewport::NeedsScrollbar(viewportH, contentH)) {
        m_ScrollViewport.ApplyWheel(event.deltaY, 36.0f * scale, viewportH, contentH);
        InvalidatePaint();
    }
}

void CompactTreeWidget::OnMouseMove(const MouseEvent& event) {
    if (!IsVisible()) return;
    std::string newHover;
    for (const auto& item : m_Items) {
        if (!IsItemVisible(item)) continue;
        Rect itemScrolledRect = item.rect;
        itemScrolledRect.y -= m_ScrollViewport.offset;
        if (itemScrolledRect.Contains(event.position)) {
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

    for (auto& item : m_Items) {
        if (!IsItemVisible(item)) continue;
        Rect itemScrolledRect = item.rect;
        itemScrolledRect.y -= m_ScrollViewport.offset;

        if (itemScrolledRect.Contains(event.position)) {
            if (item.hasChildren) {
                Rect expanderScrolledRect = item.expanderRect;
                expanderScrolledRect.y -= m_ScrollViewport.offset;

                if (expanderScrolledRect.Contains(event.position)) {
                    item.expanded = !item.expanded;
                    m_ExpandedState[item.id] = item.expanded;
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
}

} // namespace we::runtime::kindui
