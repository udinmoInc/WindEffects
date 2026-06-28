#include "Widgets/ContentBrowser.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include <algorithm>
#include <cctype>
#include <algorithm>

namespace we::UI {

ContentBrowser::ContentBrowser()
    : m_Style(WidgetStyle::Panel())
{
    m_Model = std::make_shared<ContentBrowserModel>();
    m_Controller = std::make_shared<ContentBrowserController>(m_Model);
}

Size ContentBrowser::Measure(const Size& availableSize) {
    BuildRenderList();
    
    float oldWidth = m_Geometry.width;
    // Temporarily set width so CalculateGridLayout computes correct rows
    m_Geometry.width = std::max(1.0f, availableSize.width);
    
    if (m_Model->viewMode == ContentViewMode::Grid) {
        CalculateGridLayout();
    } else {
        CalculateListLayout();
    }
    
    m_Geometry.width = oldWidth;
    
    float totalHeight = 0.0f;
    for (const auto& item : m_RenderList) {
        totalHeight = std::max(totalHeight, item.geometry.y + item.geometry.height - m_Geometry.y);
    }
    
    m_DesiredSize = Size{ availableSize.width, totalHeight };
    return m_DesiredSize;
}

void ContentBrowser::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    
    if (m_Model->viewMode == ContentViewMode::Grid) {
        CalculateGridLayout();
    } else {
        CalculateListLayout();
    }
}

void ContentBrowser::Paint(PaintContext& context) {
    // Draw items
    for (const auto& renderItem : m_RenderList) {
        const auto& item = renderItem.item;
        
        // Skip if outside visible area
        if (renderItem.geometry.y + renderItem.geometry.height < m_Geometry.y ||
            renderItem.geometry.y > m_Geometry.y + m_Geometry.height) {
            continue;
        }
        
        // Draw selection background
        if (IsSelected(item.id)) {
            // Selected: Subtle dark fill with thin blue outline (#3B82F6)
            context.DrawRoundedRect(renderItem.geometry, Color{0.1f, 0.1f, 0.1f, 0.3f}, 4.0f);
            context.DrawRoundedRectOutline(renderItem.geometry, Color{0.231f, 0.51f, 0.965f, 1.0f}, 1.0f, 4.0f);
        } else if (item.id == m_HoveredId) {
            // Hovered: #303030
            context.DrawRoundedRect(renderItem.geometry, Color{0.188f, 0.188f, 0.188f, 1.0f}, 4.0f);
        }
        
        if (m_Model->viewMode == ContentViewMode::Grid) {
            // Draw thumbnail/icon (48x48)
            float iconSize = 48.0f; // m_ThumbnailSize
            float iconX = renderItem.geometry.x + (m_GridItemSize - iconSize) / 2.0f;
            float iconY = renderItem.geometry.y + 12.0f; // Center somewhat in top portion
            
            Rect iconRect{ iconX, iconY, iconSize, iconSize };
            if (item.iconTexture != VK_NULL_HANDLE) {
                context.DrawTexture(iconRect, item.iconTexture);
            } else {
                // Fallback
                IconPainter::DrawIcon(context, item.iconName, iconRect, Theme::Get().TextPrimary);
            }
            
            // Draw label
            float labelY = renderItem.geometry.y + 68.0f;
            
            // Truncate text if too long
            std::string displayName = item.name;
            float textWidth = context.GetTextWidth(displayName, 12.0f);
            if (textWidth > m_GridItemSize - 8.0f) {
                while (displayName.length() > 3 && context.GetTextWidth(displayName + "...", 12.0f) > m_GridItemSize - 8.0f) {
                    displayName.pop_back();
                }
                displayName += "...";
                textWidth = context.GetTextWidth(displayName, 12.0f);
            }
            
            float labelX = renderItem.geometry.x + (m_GridItemSize - textWidth) / 2.0f;
            context.DrawText(displayName, Point{ labelX, labelY }, Theme::Get().TextPrimary, 12.0f);
            
            // Draw favorite star
            if (item.isFavorite) {
                float starSize = 14.0f;
                float starX = renderItem.geometry.x + renderItem.geometry.width - starSize - 4.0f;
                float starY = renderItem.geometry.y + 4.0f;
                Rect starRect{ starX, starY, starSize, starSize };
                IconPainter::DrawIcon(context, Icons::StarFilled, starRect, Theme::Get().SelectedAccent);
            }
        } else {
            // List view
            float iconSize = 20.0f;
            float iconX = renderItem.geometry.x + 8.0f;
            float iconY = renderItem.geometry.y + (m_ListRowHeight - iconSize) / 2.0f;
            
            Rect iconRect{ iconX, iconY, iconSize, iconSize };
            if (item.iconTexture != VK_NULL_HANDLE) {
                context.DrawTexture(iconRect, item.iconTexture);
            } else {
                IconPainter::DrawIcon(context, item.iconName, iconRect, Theme::Get().TextPrimary);
            }
            
            // Draw name
            float nameX = iconX + iconSize + 8.0f;
            float nameY = renderItem.geometry.y + (m_ListRowHeight - 13.0f) / 2.0f;
            context.DrawText(item.name, Point{ nameX, nameY }, Theme::Get().TextPrimary, 13.0f);
            
            // Draw type
            float typeX = renderItem.geometry.x + renderItem.geometry.width - item.type.length() * 12.0f * 0.6f - 8.0f;
            float typeY = renderItem.geometry.y + (m_ListRowHeight - 12.0f) / 2.0f;
            context.DrawText(item.type, Point{ typeX, typeY }, Theme::Get().TextSecondary, 12.0f);
        }
    }
    
    // Draw marquee selection box
    if (m_IsSelecting) {
        float minX = std::min(m_SelectStart.x, m_SelectEnd.x);
        float maxX = std::max(m_SelectStart.x, m_SelectEnd.x);
        float minY = std::min(m_SelectStart.y, m_SelectEnd.y);
        float maxY = std::max(m_SelectStart.y, m_SelectEnd.y);
        Rect selectBox{minX, minY, maxX - minX, maxY - minY};
        
        context.DrawRect(selectBox, Color{0.231f, 0.51f, 0.965f, 0.15f}); // Semi-transparent blue fill
        context.DrawRoundedRectOutline(selectBox, Color{0.231f, 0.51f, 0.965f, 0.8f}, 1.0f, 0.0f); // Blue border
    }
    
    // Draw drag ghost
    if (m_IsDragging && !m_Model->selectedIds.empty()) {
        // Find the first selected item to use as a ghost
        for (const auto& renderItem : m_RenderList) {
            if (IsSelected(renderItem.item.id)) {
                Rect ghostRect{ m_MousePos.x - m_GridItemSize/2.0f, m_MousePos.y - m_GridItemSize/2.0f, m_GridItemSize, m_GridItemSize };
                context.DrawRoundedRect(ghostRect, Color{0.145f, 0.145f, 0.145f, 0.8f}, 4.0f);
                context.DrawRoundedRectOutline(ghostRect, Color{0.231f, 0.51f, 0.965f, 1.0f}, 1.0f, 4.0f);
                
                // Draw icon inside ghost
                Rect iconRect{ ghostRect.x + (m_GridItemSize - m_ThumbnailSize)/2.0f, ghostRect.y + 12.0f, m_ThumbnailSize, m_ThumbnailSize };
                if (renderItem.item.iconTexture != VK_NULL_HANDLE) {
                    context.DrawTexture(iconRect, renderItem.item.iconTexture);
                }
                
                // Draw badge with count if multiple
                if (m_Model->selectedIds.size() > 1) {
                    std::string countStr = std::to_string(m_Model->selectedIds.size());
                    Rect badgeRect{ ghostRect.x + ghostRect.width - 24.0f, ghostRect.y - 8.0f, 32.0f, 20.0f };
                    context.DrawRoundedRect(badgeRect, Color{0.9f, 0.2f, 0.2f, 1.0f}, 10.0f);
                    context.DrawText(countStr, Point{badgeRect.x + 8.0f, badgeRect.y + 2.0f}, Color::White(), 12.0f);
                }
                break;
            }
        }
    }
}

void ContentBrowser::OnMouseDown(const MouseEvent& event) {
    RenderItem* renderItem = GetItemAtPosition(event.position);
    
    if (event.button == MouseButton::Left) {
        if (renderItem) {
            // Check modifiers
            bool ctrlDown = event.ctrlDown;
            bool shiftDown = event.shiftDown;
            
            if (ctrlDown) {
                if (IsSelected(renderItem->item.id)) {
                    if (m_Controller) m_Controller->RemoveFromSelection(renderItem->item.id);
                } else {
                    if (m_Controller) m_Controller->AddToSelection(renderItem->item.id);
                }
            } else if (shiftDown && !m_LastSelectedId.empty()) {
                // Find indices
                int startIdx = -1;
                int endIdx = -1;
                for (size_t i = 0; i < m_RenderList.size(); ++i) {
                    if (m_RenderList[i].item.id == m_LastSelectedId) startIdx = (int)i;
                    if (m_RenderList[i].item.id == renderItem->item.id) endIdx = (int)i;
                }
                
                if (startIdx != -1 && endIdx != -1) {
                    ClearSelection();
                    int minIdx = std::min(startIdx, endIdx);
                    int maxIdx = std::max(startIdx, endIdx);
                    for (int i = minIdx; i <= maxIdx; ++i) {
                        m_Model->selectedIds.push_back(m_RenderList[i].item.id);
                    }
                    m_LastSelectedId = renderItem->item.id;
                } else {
                    if (m_Controller) m_Controller->SetSelectedId(renderItem->item.id);
                }
            } else {
                if (IsSelected(renderItem->item.id)) {
                    // Might be dragging. Don't deselect others yet.
                    m_DragStart = event.position;
                    m_IsDragging = false;
                } else {
                    if (m_Controller) m_Controller->SetSelectedId(renderItem->item.id);
                    m_DragStart = event.position;
                    m_IsDragging = false;
                }
            }
            
            if (m_OnItemSelected) {
                m_OnItemSelected(renderItem->item);
            }
        } else {
            ClearSelection();
            
            // Start marquee selection
            m_IsSelecting = true;
            m_SelectStart = event.position;
            m_SelectEnd = event.position;
            m_IsDragging = false;
            m_DragStart = Point{0,0};
        }
    } else if (event.button == MouseButton::Right) {
        if (renderItem) {
            // If right-clicking an unselected item, select it first
            if (!IsSelected(renderItem->item.id)) {
                if (m_Controller) m_Controller->SetSelectedId(renderItem->item.id);
                if (m_OnItemSelected) {
                    m_OnItemSelected(renderItem->item);
                }
            }
            
            if (m_OnItemRightClicked) {
                m_OnItemRightClicked(renderItem->item, event.position);
            }
        } else {
            ClearSelection();
            if (m_OnBackgroundRightClicked) {
                m_OnBackgroundRightClicked(event.position);
            }
        }
    }
}

void ContentBrowser::OnMouseMove(const MouseEvent& event) {
    m_MousePos = event.position;
    
    if (m_IsSelecting) {
        m_SelectEnd = event.position;
        
        // Compute selection box
        float minX = std::min(m_SelectStart.x, m_SelectEnd.x);
        float maxX = std::max(m_SelectStart.x, m_SelectEnd.x);
        float minY = std::min(m_SelectStart.y, m_SelectEnd.y);
        float maxY = std::max(m_SelectStart.y, m_SelectEnd.y);
        Rect selectBox{minX, minY, maxX - minX, maxY - minY};
        
        // Update selection dynamically
        ClearSelection();
        for (const auto& renderItem : m_RenderList) {
            Rect intersection = renderItem.geometry.Intersect(selectBox);
            if (intersection.width > 0.0f && intersection.height > 0.0f) {
                if (m_Controller) m_Controller->AddToSelection(renderItem.item.id);
            }
        }
    } else if (m_DragStart.x != 0.0f || m_DragStart.y != 0.0f) {
        // Check drag threshold
        float dx = event.position.x - m_DragStart.x;
        float dy = event.position.y - m_DragStart.y;
        if (dx*dx + dy*dy > 25.0f) { // 5px threshold
            m_IsDragging = true;
        }
    }
    
    RenderItem* renderItem = GetItemAtPosition(event.position);
    m_HoveredId = renderItem ? renderItem->item.id : "";
}

void ContentBrowser::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        if (m_IsSelecting) {
            m_IsSelecting = false;
        } else if (m_IsDragging) {
            m_IsDragging = false;
            m_DragStart = Point{0,0};
            // Trigger drop logic here if hovering over a folder
            RenderItem* target = GetItemAtPosition(event.position);
            if (target && target->item.isFolder && !IsSelected(target->item.id)) {
                // Drop into folder logic
            }
        } else if (m_DragStart.x != 0.0f || m_DragStart.y != 0.0f) {
            // It was just a click on a selected item without dragging
            RenderItem* target = GetItemAtPosition(event.position);
            if (target && IsSelected(target->item.id)) {
                bool ctrlDown = event.ctrlDown;
                bool shiftDown = event.shiftDown;
                if (!ctrlDown && !shiftDown) {
                    if (m_Controller) m_Controller->SetSelectedId(target->item.id);
                }
            }
            m_DragStart = Point{0,0};
        }
    }
}

void ContentBrowser::OnMouseWheel(const MouseEvent& event) {
    float scrollAmount = event.wheelDeltaY * (GetViewMode() == ContentViewMode::Grid ? m_GridItemSize : m_ListRowHeight) * 0.5f;
    m_ScrollOffset -= scrollAmount;
    
    // Clamp scroll
    float totalHeight = Measure(Size{m_Geometry.width, m_Geometry.height}).height;
    float maxScroll = std::max(0.0f, totalHeight - m_Geometry.height);
    m_ScrollOffset = std::max(0.0f, std::min(m_ScrollOffset, maxScroll));
    
    Arrange(m_Geometry);
}

void ContentBrowser::OnKeyDown(const KeyEvent& event) {
    // TODO: Implement keyboard navigation
}







bool ContentBrowser::IsSelected(const std::string& id) const {
    return std::find(m_Model->selectedIds.begin(), m_Model->selectedIds.end(), id) != m_Model->selectedIds.end();
}



void ContentBrowser::BuildRenderList() {
    m_RenderList.clear();
    
    for (const auto& item : m_Model->items) {
        if (!m_Model->filterText.empty()) {
            std::string itemNameLower = item.name;
            std::transform(itemNameLower.begin(), itemNameLower.end(), itemNameLower.begin(), ::tolower);
            if (itemNameLower.find(m_Model->filterText) == std::string::npos) {
                continue;
            }
        }
        
        if (item.iconTexture == VK_NULL_HANDLE && !item.thumbnailRequested) {
            if (m_OnItemNeedsThumbnail) {
                m_OnItemNeedsThumbnail(item.id);
            }
            // We need to mark the original item in m_Model->items as requested so we don't spam
            for (auto& origItem : m_Model->items) {
                if (origItem.id == item.id) {
                    origItem.thumbnailRequested = true;
                    break;
                }
            }
        }
        
        RenderItem renderItem;
        renderItem.item = item;
        renderItem.geometry = Rect{};
        m_RenderList.push_back(renderItem);
    }
}

void ContentBrowser::CalculateGridLayout() {
    float x = m_Geometry.x + 8.0f;
    float y = m_Geometry.y + 8.0f - m_ScrollOffset;
    int itemsPerRow = std::max(1, static_cast<int>((m_Geometry.width - 16.0f) / (m_GridItemSize + m_GridSpacing)));
    
    for (size_t i = 0; i < m_RenderList.size(); ++i) {
        int col = static_cast<int>(i) % itemsPerRow;
        int row = static_cast<int>(i) / itemsPerRow;
        
        m_RenderList[i].geometry = Rect{
            x + col * (m_GridItemSize + m_GridSpacing),
            y + row * (m_GridItemSize + m_GridSpacing + 20.0f), // +20 for label
            m_GridItemSize,
            m_GridItemSize + 20.0f
        };
    }
}

void ContentBrowser::CalculateListLayout() {
    float y = m_Geometry.y - m_ScrollOffset;
    
    for (auto& renderItem : m_RenderList) {
        renderItem.geometry = Rect{
            m_Geometry.x,
            y,
            m_Geometry.width,
            m_ListRowHeight
        };
        y += m_ListRowHeight;
    }
}

ContentBrowser::RenderItem* ContentBrowser::GetItemAtPosition(const Point& pos) {
    for (auto& renderItem : m_RenderList) {
        if (pos.x >= renderItem.geometry.x && pos.x <= renderItem.geometry.x + renderItem.geometry.width &&
            pos.y >= renderItem.geometry.y && pos.y <= renderItem.geometry.y + renderItem.geometry.height) {
            return &renderItem;
        }
    }
    return nullptr;
}

// Breadcrumb implementation
Breadcrumb::Breadcrumb() {}

Size Breadcrumb::Measure(const Size& availableSize) {
    CalculateLayout();
    
    float totalWidth = 0.0f;
    for (const auto& crumb : m_Crumbs) {
        totalWidth += crumb.geometry.width;
    }
    
    return Size{ totalWidth, 24.0f };
}

void Breadcrumb::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateLayout();
}

void Breadcrumb::Paint(PaintContext& context) {
    for (size_t i = 0; i < m_Crumbs.size(); ++i) {
        const auto& crumb = m_Crumbs[i];
        
        // Draw hover background
        if (crumb.hovered) {
            context.DrawRoundedRect(crumb.geometry, Theme::Get().HoverOverlay, 4.0f);
        }
        
        // Draw crumb text
        float textX = crumb.geometry.x + 8.0f;
        float textY = crumb.geometry.y + (24.0f - 13.0f) / 2.0f;
        
        Color textColor = (static_cast<int>(i) == m_HoveredCrumb) ? Theme::Get().SelectedAccent : Theme::Get().TextPrimary;
        context.DrawText(crumb.text, Point{ textX, textY }, textColor, 13.0f);
        
        // Draw separator (chevron)
        if (i < m_Crumbs.size() - 1) {
            float sepX = crumb.geometry.x + crumb.geometry.width + m_CrumbSpacing;
            float sepY = crumb.geometry.y + (24.0f - 12.0f) / 2.0f;
            Rect sepRect{ sepX, sepY, 12.0f, 12.0f };
            IconPainter::DrawIcon(context, Icons::ChevronRight, sepRect, Theme::Get().TextSecondary);
        }
    }
}

void Breadcrumb::OnMouseDown(const MouseEvent& event) {
    CrumbInfo* crumb = GetCrumbAtPosition(event.position);
    if (crumb && m_OnCrumbClicked) {
        size_t index = crumb - &m_Crumbs[0];
        m_OnCrumbClicked(index);
    }
}

void Breadcrumb::OnMouseMove(const MouseEvent& event) {
    m_HoveredCrumb = -1;
    
    CrumbInfo* crumb = GetCrumbAtPosition(event.position);
    if (crumb) {
        m_HoveredCrumb = static_cast<int>(crumb - &m_Crumbs[0]);
    }
    
    for (auto& c : m_Crumbs) {
        c.hovered = false;
    }
    
    if (m_HoveredCrumb >= 0) {
        m_Crumbs[m_HoveredCrumb].hovered = true;
    }
}

void Breadcrumb::SetPath(const std::vector<std::string>& path) {
    m_Crumbs.clear();
    
    for (const auto& crumb : path) {
        CrumbInfo info;
        info.text = crumb;
        info.geometry = Rect{};
        m_Crumbs.push_back(info);
    }
    
    CalculateLayout();
}

void Breadcrumb::AddCrumb(const std::string& crumb) {
    CrumbInfo info;
    info.text = crumb;
    info.geometry = Rect{};
    m_Crumbs.push_back(info);
    CalculateLayout();
}

void Breadcrumb::Clear() {
    m_Crumbs.clear();
}

void Breadcrumb::CalculateLayout() {
    float x = m_Geometry.x;
    
    for (auto& crumb : m_Crumbs) {
        float textWidth = crumb.text.length() * 13.0f * 0.6f;
        float width = textWidth + 16.0f; // padding
        
        crumb.geometry = Rect{ x, m_Geometry.y, width, 24.0f };
        x += width + m_SeparatorSpacing + m_CrumbSpacing;
    }
}

Breadcrumb::CrumbInfo* Breadcrumb::GetCrumbAtPosition(const Point& pos) {
    for (auto& crumb : m_Crumbs) {
        if (pos.x >= crumb.geometry.x && pos.x <= crumb.geometry.x + crumb.geometry.width &&
            pos.y >= crumb.geometry.y && pos.y <= crumb.geometry.y + crumb.geometry.height) {
            return &crumb;
        }
    }
    return nullptr;
}

} // namespace we::editor::contentbrowser::UI
