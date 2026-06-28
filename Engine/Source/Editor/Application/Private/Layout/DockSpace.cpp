#include "DockSpace.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include <algorithm>
#include <iostream>

namespace we::UI {

DockSpace::DockSpace() {
    m_RootNode = std::make_shared<DockNode>();
    m_RootNode->Id = "Root";
}

Size DockSpace::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    // We just take all available space
    return availableSize;
}

void DockSpace::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_RootNode) {
        ArrangeNode(m_RootNode, m_Geometry);
    }
}

void DockSpace::ArrangeNode(std::shared_ptr<DockNode> node, const Rect& rect) {
    if (!node) return;

    if (node->IsSplit) {
        float ratio = node->SplitRatio;
        Rect rectA = rect;
        Rect rectB = rect;

        if (node->SplitDirection == DockDirection::Left || node->SplitDirection == DockDirection::Right) {
            float widthA = rect.width * ratio;
            float widthB = rect.width - widthA;

            if (node->SplitDirection == DockDirection::Left) {
                rectA.width = widthA;
                rectB.x = rect.x + widthA;
                rectB.width = widthB;
            } else {
                rectB.width = widthB;
                rectA.x = rect.x + widthB;
                rectA.width = widthA;
            }
        } else if (node->SplitDirection == DockDirection::Top || node->SplitDirection == DockDirection::Bottom) {
            float heightA = rect.height * ratio;
            float heightB = rect.height - heightA;

            if (node->SplitDirection == DockDirection::Top) {
                rectA.height = heightA;
                rectB.y = rect.y + heightA;
                rectB.height = heightB;
            } else {
                rectB.height = heightB;
                rectA.y = rect.y + heightB;
                rectA.height = heightA;
            }
        }

        ArrangeNode(node->ChildA, rectA);
        ArrangeNode(node->ChildB, rectB);
    } else {
        // Render tabs
        if (!node->Tabs.empty()) {
            float tabHeight = 32.0f;
            Rect contentRect = rect;
            contentRect.y += tabHeight;
            contentRect.height -= tabHeight;

            if (node->ActiveTab >= 0 && node->ActiveTab < (int)node->Tabs.size()) {
                auto activeWidget = node->Tabs[node->ActiveTab];
                activeWidget->Measure({contentRect.width, contentRect.height});
                activeWidget->Arrange(contentRect);
            }
        }
    }
}

void DockSpace::Paint(PaintContext& context) {
    // Background
    context.DrawRect(m_Geometry, Theme::Get().WorkspaceBackground);

    if (m_RootNode) {
        PaintNode(m_RootNode, context, m_Geometry);
    }

    // Drag Preview
    if (m_IsDragging && m_DraggedWidget) {
        // Outline only with Accent Blue
        context.DrawRoundedRectOutline(m_PreviewRect, Theme::Get().SelectedAccent, 2.0f, Theme::Get().CornerRadiusSmall);
        
        // Very subtle background tint (mostly transparent)
        Color tint = Theme::Get().SelectedAccent;
        tint.a = 0.1f;
        context.DrawRect(m_PreviewRect, tint);
    }
}

void DockSpace::PaintNode(std::shared_ptr<DockNode> node, PaintContext& context, const Rect& rect) {
    if (!node) return;

    if (node->IsSplit) {
        Rect rectA = rect;
        Rect rectB = rect;
        Rect splitterRect;

        float ratio = node->SplitRatio;
        float splitThickness = 4.0f; // Visual splitter hit area

        if (node->SplitDirection == DockDirection::Left || node->SplitDirection == DockDirection::Right) {
            float widthA = rect.width * ratio;
            float widthB = rect.width - widthA;

            if (node->SplitDirection == DockDirection::Left) {
                rectA.width = widthA;
                rectB.x = rect.x + widthA;
                rectB.width = widthB;
                splitterRect = { rectA.x + rectA.width - splitThickness/2, rectA.y, splitThickness, rectA.height };
            } else {
                rectB.width = widthB;
                rectA.x = rect.x + widthB;
                rectA.width = widthA;
                splitterRect = { rectB.x + rectB.width - splitThickness/2, rectB.y, splitThickness, rectB.height };
            }
        } else {
            float heightA = rect.height * ratio;
            float heightB = rect.height - heightA;

            if (node->SplitDirection == DockDirection::Top) {
                rectA.height = heightA;
                rectB.y = rect.y + heightA;
                rectB.height = heightB;
                splitterRect = { rectA.x, rectA.y + rectA.height - splitThickness/2, rectA.width, splitThickness };
            } else {
                rectB.height = heightB;
                rectA.y = rect.y + heightB;
                rectA.height = heightA;
                splitterRect = { rectB.x, rectB.y + rectB.height - splitThickness/2, rectB.width, splitThickness };
            }
        }

        PaintNode(node->ChildA, context, rectA);
        PaintNode(node->ChildB, context, rectB);

        // Draw splitter (1px line visually, but logic has 4px hit area)
        context.DrawRect({ splitterRect.x + splitterRect.width/2.0f, splitterRect.y, 1.0f, splitterRect.height }, Theme::Get().Separator);

    } else {
        // Panel Background
        context.DrawRect(rect, Theme::Get().PanelBackground);

        // Draw Tabs
        if (!node->Tabs.empty()) {
            float tabHeight = 32.0f;
            Rect tabHeaderRect = { rect.x, rect.y, rect.width, tabHeight };
            context.DrawRect(tabHeaderRect, Theme::Get().HeaderBackground);

            float currentX = rect.x;
            for (size_t i = 0; i < node->Tabs.size(); ++i) {
                std::string title = "Tab " + std::to_string(i); // Replace with widget title later
                float tabWidth = 100.0f; 
                Rect tabRect = { currentX, rect.y, tabWidth, tabHeight };

                if ((int)i == node->ActiveTab) {
                    context.DrawRect(tabRect, Theme::Get().TabBackground);
                    // Active Tab Line
                    context.DrawRect({ tabRect.x, tabRect.y + tabRect.height - 2.0f, tabRect.width, 2.0f }, Theme::Get().ActiveTabLine);
                    context.DrawText(title, { tabRect.x + 8.0f, tabRect.y + 8.0f }, Theme::Get().TextPrimary, Theme::Get().TextSizeNormal);
                } else {
                    context.DrawText(title, { tabRect.x + 8.0f, tabRect.y + 8.0f }, Theme::Get().TextSecondary, Theme::Get().TextSizeNormal);
                }

                currentX += tabWidth;
            }

            // Draw Separator below tabs
            context.DrawRect({ rect.x, rect.y + tabHeight - 1.0f, rect.width, 1.0f }, Theme::Get().Separator);

            // Paint Active Tab
            if (node->ActiveTab >= 0 && node->ActiveTab < (int)node->Tabs.size()) {
                node->Tabs[node->ActiveTab]->Paint(context);
            }
        }
    }
}

void DockSpace::DockWidget(std::shared_ptr<Widget> widget, DockDirection direction, const std::string& targetNodeId) {
    if (!widget) return;
    
    // Simplistic docking to root for now
    if (m_RootNode->Tabs.empty() && !m_RootNode->IsSplit) {
        m_RootNode->Tabs.push_back(widget);
        m_RootNode->ActiveTab = 0;
    } else {
        // Create split
        auto newSplit = std::make_shared<DockNode>();
        newSplit->IsSplit = true;
        newSplit->SplitDirection = direction;
        newSplit->SplitRatio = 0.5f;

        auto childA = std::make_shared<DockNode>();
        childA->Tabs.push_back(widget);

        newSplit->ChildA = childA;
        newSplit->ChildB = m_RootNode; // move existing root to child B

        m_RootNode = newSplit;
    }
    
    AddChild(widget); // Register in widget tree
}

void DockSpace::OnMouseDown(const MouseEvent& event) {
    // Implement tab dragging / splitter dragging
}

void DockSpace::OnMouseMove(const MouseEvent& event) {
    if (m_IsDragging) {
        // Calculate docking preview bounds
    }
}

void DockSpace::OnMouseUp(const MouseEvent& event) {
    if (m_IsDragging) {
        m_IsDragging = false;
        // Perform dock
    }
}

} // namespace we::editor::application::UI
