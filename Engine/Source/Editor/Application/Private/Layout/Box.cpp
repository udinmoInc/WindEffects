#include "Layout/Box.hpp"
#include <algorithm>

namespace we::UI {

Size Box::Measure(const Size& availableSize) {
    float paddingWidth = m_Padding.left + m_Padding.right;
    float paddingHeight = m_Padding.top + m_Padding.bottom;

    Size contentAvailable{
        std::max(0.0f, availableSize.width - paddingWidth),
        std::max(0.0f, availableSize.height - paddingHeight)
    };

    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
    bool first = true;

    for (const auto& child : m_Children) {
        if (!child->IsVisible()) continue;

        Size childDesired = child->Measure(contentAvailable);

        if (m_Orientation == Orientation::Horizontal) {
            totalWidth += childDesired.width;
            if (!first) totalWidth += m_Spacing;
            totalHeight = std::max(totalHeight, childDesired.height);
        } else {
            totalHeight += childDesired.height;
            if (!first) totalHeight += m_Spacing;
            totalWidth = std::max(totalWidth, childDesired.width);
        }
        first = false;
    }

    m_DesiredSize = Size{
        totalWidth + paddingWidth,
        totalHeight + paddingHeight
    };

    return m_DesiredSize;
}

void Box::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    float paddingWidth = m_Padding.left + m_Padding.right;
    float paddingHeight = m_Padding.top + m_Padding.bottom;
    float contentWidth = std::max(0.0f, allottedRect.width - paddingWidth);
    float contentHeight = std::max(0.0f, allottedRect.height - paddingHeight);

    float currentX = allottedRect.x + m_Padding.left;
    float currentY = allottedRect.y + m_Padding.top;
    bool first = true;

    // Calculate overflow to shrink expanding widgets (like Splitters)
    float totalDesiredWidth = 0.0f;
    float totalDesiredHeight = 0.0f;
    size_t largestWidthIdx = 0;
    size_t largestHeightIdx = 0;
    float maxChildWidth = -1.0f;
    float maxChildHeight = -1.0f;

    for (size_t i = 0; i < m_Children.size(); ++i) {
        if (!m_Children[i]->IsVisible()) continue;
        Size childDesired = m_Children[i]->GetDesiredSize();
        
        if (m_Orientation == Orientation::Horizontal) {
            if (totalDesiredWidth > 0) totalDesiredWidth += m_Spacing;
            totalDesiredWidth += childDesired.width;
            if (childDesired.width > maxChildWidth) {
                maxChildWidth = childDesired.width;
                largestWidthIdx = i;
            }
        } else {
            if (totalDesiredHeight > 0) totalDesiredHeight += m_Spacing;
            totalDesiredHeight += childDesired.height;
            if (childDesired.height > maxChildHeight) {
                maxChildHeight = childDesired.height;
                largestHeightIdx = i;
            }
        }
    }

    // Count how many children share the max width/height to distribute overflow
    int numMaxWidths = 0;
    int numMaxHeights = 0;
    for (size_t i = 0; i < m_Children.size(); ++i) {
        if (!m_Children[i]->IsVisible()) continue;
        Size childDesired = m_Children[i]->GetDesiredSize();
        if (m_Orientation == Orientation::Horizontal) {
            if (std::abs(childDesired.width - maxChildWidth) < 0.1f) numMaxWidths++;
        } else {
            if (std::abs(childDesired.height - maxChildHeight) < 0.1f) numMaxHeights++;
        }
    }

    float widthOverflow = std::max(0.0f, totalDesiredWidth - contentWidth);
    float heightOverflow = std::max(0.0f, totalDesiredHeight - contentHeight);
    
    float widthShrinkPerElement = numMaxWidths > 0 ? (widthOverflow / numMaxWidths) : 0.0f;
    float heightShrinkPerElement = numMaxHeights > 0 ? (heightOverflow / numMaxHeights) : 0.0f;

    for (size_t i = 0; i < m_Children.size(); ++i) {
        const auto& child = m_Children[i];
        if (!child->IsVisible()) continue;

        Size childDesired = child->GetDesiredSize();
        bool isLast = (i == m_Children.size() - 1);

        if (m_Orientation == Orientation::Horizontal) {
            if (!first) currentX += m_Spacing;
            
            float childWidth = childDesired.width;
            if (std::abs(childDesired.width - maxChildWidth) < 0.1f) childWidth -= widthShrinkPerElement;
            if (isLast && currentX + childWidth < allottedRect.x + allottedRect.width) {
                childWidth = allottedRect.x + allottedRect.width - currentX; // Fill remaining if any
            }
            
            float childHeight = childDesired.height;
            if (child->GetVerticalAlignment() == VerticalAlignment::Fill) {
                childHeight = contentHeight;
            } else {
                childHeight = std::min(contentHeight, childDesired.height);
            }
            
            float childY = currentY;
            if (child->GetVerticalAlignment() == VerticalAlignment::Center) {
                childY = currentY + (contentHeight - childHeight) * 0.5f;
            } else if (child->GetVerticalAlignment() == VerticalAlignment::Bottom) {
                childY = currentY + contentHeight - childHeight;
            }

            child->Arrange(Rect{ currentX, childY, childWidth, childHeight });
            currentX += childWidth;
        } else {
            if (!first) currentY += m_Spacing;

            float childWidth = childDesired.width;
            if (child->GetHorizontalAlignment() == HorizontalAlignment::Fill) {
                childWidth = contentWidth;
            } else {
                childWidth = std::min(contentWidth, childDesired.width);
            }
            
            float childHeight = childDesired.height;
            if (std::abs(childDesired.height - maxChildHeight) < 0.1f) childHeight -= heightShrinkPerElement;
            if (isLast && currentY + childHeight < allottedRect.y + allottedRect.height) {
                childHeight = allottedRect.y + allottedRect.height - currentY; // Fill remaining if any
            }
            
            float childX = currentX;
            if (child->GetHorizontalAlignment() == HorizontalAlignment::Center) {
                childX = currentX + (contentWidth - childWidth) * 0.5f;
            } else if (child->GetHorizontalAlignment() == HorizontalAlignment::Right) {
                childX = currentX + contentWidth - childWidth;
            }

            child->Arrange(Rect{ childX, currentY, childWidth, childHeight });
            currentY += childHeight;
        }
        first = false;
    }
}

void Box::Paint(PaintContext& context) {
    if (!m_Visible) return;
    for (const auto& child : m_Children) {
        if (child->IsVisible()) {
            child->Paint(context);
        }
    }
}

} // namespace we::editor::application::UI
