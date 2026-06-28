#include "TreeView.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Icon.hpp"
#include <algorithm>
#include <SDL3/SDL.h>

namespace we::UI {

TreeView::TreeView()
    : m_Style(WidgetStyle::TreeItem())
{
    // Create a default root node
    m_Root = std::make_shared<TreeNode>();
    m_Root->id = "root";
    m_Root->label = "Root";
}

Size TreeView::Measure(const Size& availableSize) {
    BuildRenderList();
    
    float totalHeight = static_cast<float>(m_RenderList.size()) * m_ItemHeight;
    return Size{ availableSize.width, totalHeight };
}

void TreeView::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    BuildRenderList();
    
    // Update render item geometries
    float y = m_Geometry.y - m_ScrollOffset;
    for (auto& item : m_RenderList) {
        item.geometry = Rect{
            m_Geometry.x + item.depth * m_IndentWidth,
            y,
            m_Geometry.width - item.depth * m_IndentWidth,
            m_ItemHeight
        };
        y += m_ItemHeight;
    }
}

void TreeView::Paint(PaintContext& context) {
    // Draw background
    context.DrawRect(m_Geometry, Theme::Get().PanelBackground);
    
    // Draw items
    for (const auto& item : m_RenderList) {
        const auto& node = item.node;
        
        // Skip if outside visible area
        if (item.geometry.y + item.geometry.height < m_Geometry.y ||
            item.geometry.y > m_Geometry.y + m_Geometry.height) {
            continue;
        }
        
        // Determine background color
        Color bgColor = Color{0, 0, 0, 0};
        
        // Alternating row colors
        bool isEven = (&item - &m_RenderList[0]) % 2 == 0;
        if (isEven) {
            bgColor = Color{1.0f, 1.0f, 1.0f, 0.02f};
        }
        
        if (node->id == m_SelectedId) {
            bgColor = Theme::Get().SelectedAccent * 0.4f; // More visible blue accent
        } else if (node->id == m_HoveredId) {
            bgColor = Theme::Get().HoverOverlay;
        }
        
        if (bgColor.a > 0.001f) {
            // Draw full width flat rectangle for row highlights
            Rect rowRect = item.geometry;
            rowRect.x = m_Geometry.x; // Span full width
            rowRect.width = m_Geometry.width;
            context.DrawRect(rowRect, bgColor);
        }
        
        // Draw expand/collapse chevron
        if (!node->children.empty()) {
            float chevronSize = 14.0f;
            float chevronX = item.geometry.x + 4.0f + item.depth * m_IndentWidth;
            float chevronY = item.geometry.y + (m_ItemHeight - chevronSize) / 2.0f;
            
            int chevronIcon = node->expanded ? Icons::ChevronDown : Icons::ChevronRight;
            context.DrawIcon(chevronIcon, Point{ chevronX, chevronY }, Theme::Get().TextSecondary, chevronSize);
        }
        
        // Draw item icon
        if (!node->iconName.empty()) {
            float iconSize = 16.0f;
            float iconX = item.geometry.x + 20.0f + item.depth * m_IndentWidth;
            float iconY = item.geometry.y + (m_ItemHeight - iconSize) / 2.0f;
            
            int iconCode = Icons::GetCodepoint(node->iconName);
            if (iconCode != 0) {
                context.DrawIcon(iconCode, Point{ iconX, iconY }, Theme::Get().TextPrimary, iconSize);
            }
        }
        
        // Draw label
        float textX = item.geometry.x + 44.0f + item.depth * m_IndentWidth;
        float textY = item.geometry.y + (m_ItemHeight - m_Style.text.size) / 2.0f;
        
        Color textColor = node->locked ? Theme::Get().TextSecondary * 0.6f : Theme::Get().TextPrimary;
        if (!node->visible) {
            textColor = Theme::Get().TextSecondary * 0.4f;
        }
        
        context.DrawText(node->label, Point{ textX, textY }, textColor, m_Style.text.size);
        
        // Draw visibility eye icon
        float eyeSize = 14.0f;
        float eyeX = item.geometry.x + item.geometry.width - eyeSize - 8.0f;
        float eyeY = item.geometry.y + (m_ItemHeight - eyeSize) / 2.0f;
        
        int eyeIcon = node->visible ? Icons::Eye : Icons::EyeOff;
        Color eyeColor = node->visible ? Theme::Get().TextSecondary : Theme::Get().TextSecondary * 0.5f;
        context.DrawIcon(eyeIcon, Point{ eyeX, eyeY }, eyeColor, eyeSize);
        
        // Draw lock icon if locked
        if (node->locked) {
            float lockSize = 14.0f;
            float lockX = eyeX - lockSize - 4.0f;
            float lockY = item.geometry.y + (m_ItemHeight - lockSize) / 2.0f;
            
            context.DrawIcon(Icons::Lock, Point{ lockX, lockY }, Theme::Get().Warning, lockSize);
        }
    }
}

void TreeView::OnMouseDown(const MouseEvent& event) {
    RenderItem* item = GetItemAtPosition(event.position);
    if (!item) return;
    
    const auto& node = item->node;
    
    // Check if clicked on chevron
    if (!node->children.empty()) {
        float chevronX = item->geometry.x + 4.0f + item->depth * m_IndentWidth;
        float chevronSize = 12.0f;
        Rect chevronRect{ chevronX, item->geometry.y + (m_ItemHeight - chevronSize) / 2.0f, chevronSize, chevronSize };
        
        if (event.position.x >= chevronRect.x && event.position.x <= chevronRect.x + chevronRect.width &&
            event.position.y >= chevronRect.y && event.position.y <= chevronRect.y + chevronRect.height) {
            ToggleExpand(node->id);
            return;
        }
    }
    
    // Check if clicked on visibility eye
    float eyeSize = 14.0f;
    float eyeX = item->geometry.x + item->geometry.width - eyeSize - 8.0f;
    Rect eyeRect{ eyeX, item->geometry.y + (m_ItemHeight - eyeSize) / 2.0f, eyeSize, eyeSize };
    
    if (event.position.x >= eyeRect.x && event.position.x <= eyeRect.x + eyeRect.width &&
        event.position.y >= eyeRect.y && event.position.y <= eyeRect.y + eyeRect.height) {
        node->visible = !node->visible;
        if (m_OnVisibilityToggled) {
            m_OnVisibilityToggled(node->id, node->visible);
        }
        return;
    }
    
    // Select item
    SetSelectedId(node->id);
    if (m_OnSelectionChanged) {
        m_OnSelectionChanged(node->id);
    }
}

void TreeView::OnMouseUp(const MouseEvent& event) {
    // Handle double-click detection
    static std::string lastClickedId;
    static uint64_t lastClickTime = 0;
    
    RenderItem* item = GetItemAtPosition(event.position);
    if (item) {
        uint64_t now = SDL_GetPerformanceCounter();
        uint64_t freq = SDL_GetPerformanceFrequency();
        double elapsed = static_cast<double>(now - lastClickTime) / freq;
        
        if (item->node->id == lastClickedId && elapsed < 0.3) {
            if (m_OnItemDoubleClicked) {
                m_OnItemDoubleClicked(item->node->id);
            }
        }
        
        lastClickedId = item->node->id;
        lastClickTime = now;
    }
}

void TreeView::OnMouseMove(const MouseEvent& event) {
    RenderItem* item = GetItemAtPosition(event.position);
    m_HoveredId = item ? item->node->id : "";
}

void TreeView::OnMouseWheel(const MouseEvent& event) {
    float scrollAmount = event.wheelDeltaY * m_ItemHeight * 0.5f;
    m_ScrollOffset -= scrollAmount;
    
    // Clamp scroll
    float maxScroll = std::max(0.0f, static_cast<float>(m_RenderList.size()) * m_ItemHeight - m_Geometry.height);
    m_ScrollOffset = std::max(0.0f, std::min(m_ScrollOffset, maxScroll));
    
    Arrange(m_Geometry);
}

void TreeView::OnKeyDown(const KeyEvent& event) {
    // TODO: Implement keyboard navigation
}

void TreeView::SetRoot(const std::shared_ptr<TreeNode>& root) {
    m_Root = root;
    m_SelectedId.clear();
    BuildRenderList();
}

void TreeView::AddItem(const std::shared_ptr<TreeNode>& item, const std::string& parentId) {
    if (parentId.empty()) {
        m_Root->children.push_back(item);
    } else {
        // Find parent and add
        // TODO: Implement recursive search
    }
    BuildRenderList();
}

void TreeView::RemoveItem(const std::string& id) {
    // TODO: Implement recursive removal
    BuildRenderList();
}

void TreeView::Clear() {
    m_Root->children.clear();
    m_SelectedId.clear();
    BuildRenderList();
}

void TreeView::SetSelectedId(const std::string& id) {
    m_SelectedId = id;
}

void TreeView::BuildRenderList() {
    m_RenderList.clear();
    
    std::function<void(const std::shared_ptr<TreeNode>&, int)> buildRecursive = 
        [&](const std::shared_ptr<TreeNode>& node, int depth) {
            if (node->id != "root") { // Skip root
                m_RenderList.push_back({ node, depth, Rect{} });
            }
            
            if (node->expanded) {
                for (const auto& child : node->children) {
                    buildRecursive(child, depth + 1);
                }
            }
        };
    
    for (const auto& child : m_Root->children) {
        buildRecursive(child, 0);
    }
}

TreeView::RenderItem* TreeView::GetItemAtPosition(const Point& pos) {
    for (auto& item : m_RenderList) {
        if (pos.x >= item.geometry.x && pos.x <= item.geometry.x + item.geometry.width &&
            pos.y >= item.geometry.y && pos.y <= item.geometry.y + item.geometry.height) {
            return &item;
        }
    }
    return nullptr;
}

void TreeView::ToggleExpand(const std::string& id) {
    std::function<void(std::shared_ptr<TreeNode>&)> toggleRecursive = 
        [&](std::shared_ptr<TreeNode>& node) {
            if (node->id == id) {
                node->expanded = !node->expanded;
                return;
            }
            for (auto& child : node->children) {
                toggleRecursive(child);
            }
        };
    
    for (auto& child : m_Root->children) {
        toggleRecursive(child);
    }
    
    BuildRenderList();
    Arrange(m_Geometry);
}

} // namespace we::editor::contentbrowser::UI
