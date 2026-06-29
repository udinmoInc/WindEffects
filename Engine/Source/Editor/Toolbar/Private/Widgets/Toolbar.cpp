#include "Widgets/Toolbar.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include "Widgets/ToolButton.hpp"
#include <algorithm>

namespace we::UI {
namespace {
    constexpr float kToolbarItemSpacing = 2.0f;
    constexpr float kToolbarSeparatorSpacing = 6.0f;
}

Toolbar::Toolbar()
    : m_Style(WidgetStyle::Panel())
{
    // No fixed-width containers or spacers. Layout is computed dynamically in Arrange.
}

Size Toolbar::Measure(const Size& availableSize) {
    for (auto& tool : m_Tools) {
        if (tool.button && tool.button->IsVisible()) {
            tool.button->Measure(availableSize);
        }
    }
    m_DesiredSize = Size{ availableSize.width, m_Height };
    return m_DesiredSize;
}

void Toolbar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    
    // Group tools by alignment
    std::vector<ToolInfo*> leftTools, centerTools, rightTools;
    for (auto& tool : m_Tools) {
        if (tool.button && !tool.button->IsVisible()) continue;
        
        if (tool.align == ToolbarAlignment::Left) leftTools.push_back(&tool);
        else if (tool.align == ToolbarAlignment::Center) centerTools.push_back(&tool);
        else if (tool.align == ToolbarAlignment::Right) rightTools.push_back(&tool);
    }
    
    // Layout algorithm for a given group of tools
    auto layoutGroup = [&](const std::vector<ToolInfo*>& tools, float startX, bool isRightAligned) -> float {
        struct ItemToPlace {
            std::shared_ptr<Widget> button;
            float width;
            float marginBefore;
        };
        
        std::vector<ItemToPlace> items;
        float pendingSpacing = 0.0f;
        bool isFirstButton = true;
        
        for (auto* tool : tools) {
            if (tool->isSeparator && tool->button) {
                if (!isFirstButton) {
                    float w = tool->button->GetDesiredSize().width;
                    float margin = kToolbarSeparatorSpacing / 2.0f; // Half before
                    items.push_back({tool->button, w, margin});
                    pendingSpacing = kToolbarSeparatorSpacing / 2.0f; // Half after
                }
            } else if (tool->button) {
                float w = tool->button->GetDesiredSize().width;
                float margin = isFirstButton ? 0.0f : pendingSpacing;
                if (!isFirstButton && pendingSpacing == 0.0f) margin = kToolbarItemSpacing; // fallback
                items.push_back({tool->button, w, margin});
                pendingSpacing = kToolbarItemSpacing;
                isFirstButton = false;
            }
        }
        
        float totalWidth = 0.0f;
        for (const auto& item : items) {
            totalWidth += item.marginBefore + item.width;
        }
        
        float currentX = startX;
        if (isRightAligned) {
            currentX = startX - totalWidth;
        }
        
        for (const auto& item : items) {
            currentX += item.marginBefore;
            float itemHeight = item.button->GetDesiredSize().height;
            float y = allottedRect.y + (m_Height - itemHeight) / 2.0f;
            item.button->Arrange(Rect{currentX, y, item.width, itemHeight});
            currentX += item.width;
        }
        
        return totalWidth;
    };
    
    layoutGroup(leftTools, allottedRect.x + m_LeftInset, false);
    layoutGroup(rightTools, allottedRect.x + allottedRect.width - m_EdgePadding, true);
    
    // Center group remains perfectly centered within the viewport region
    auto computeWidth = [&](const std::vector<ToolInfo*>& tools) -> float {
        float pendingSpacing = 0.0f;
        bool isFirstButton = true;
        float totalWidth = 0.0f;
        for (auto* tool : tools) {
            if (tool->isSeparator && tool->button) {
                if (!isFirstButton) {
                    totalWidth += kToolbarSeparatorSpacing / 2.0f + tool->button->GetDesiredSize().width;
                    pendingSpacing = kToolbarSeparatorSpacing / 2.0f;
                }
            } else if (tool->button) {
                float margin = isFirstButton ? 0.0f : pendingSpacing;
                if (!isFirstButton && pendingSpacing == 0.0f) margin = kToolbarItemSpacing;
                totalWidth += margin + tool->button->GetDesiredSize().width;
                pendingSpacing = kToolbarItemSpacing;
                isFirstButton = false;
            }
        }
        return totalWidth;
    };
    
    float centerTotalWidth = computeWidth(centerTools);
    float centerStartX = allottedRect.x + (allottedRect.width - centerTotalWidth) / 2.0f;
    layoutGroup(centerTools, centerStartX, false);
}

void Toolbar::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, Theme::Get().ToolbarBackground);

    Rect bottomBorder{
        m_Geometry.x,
        m_Geometry.y + m_Geometry.height - 1.0f,
        m_Geometry.width,
        1.0f
    };
    context.DrawRect(bottomBorder, Theme::Get().BorderSecondary);
    
    for (auto& tool : m_Tools) {
        if (tool.button && tool.button->IsVisible()) {
            tool.button->Paint(context);
        }
    }
}

std::shared_ptr<ToolButton> Toolbar::AddTool(const std::string& iconName, const std::string& label, std::function<void()> onClick, const std::string& tooltip, bool isPlayButton, ToolbarAlignment align) {
    ToolInfo tool;
    tool.iconName = iconName;
    tool.isSeparator = false;
    tool.align = align;
    
    auto btn = std::make_shared<ToolButton>(iconName, label, onClick, tooltip);
    if (isPlayButton) {
        btn->SetButtonStyle(ToolButtonStyle::PlayButton);
    } else {
        if (label.empty()) {
            btn->SetButtonStyle(ToolButtonStyle::ToolbarIconOnly);
        }
    }
    tool.button = btn;
    m_Tools.push_back(tool);
    
    return btn;
}

void Toolbar::AddSeparator(ToolbarAlignment align) {
    ToolInfo tool;
    tool.isSeparator = true;
    tool.align = align;
    tool.button = std::make_shared<ToolbarSeparator>();
    m_Tools.push_back(tool);
}

void Toolbar::AddWidget(std::shared_ptr<Widget> widget, ToolbarAlignment align) {
    ToolInfo tool;
    tool.iconName = "";
    tool.isSeparator = false;
    tool.align = align;
    tool.button = widget;
    m_Tools.push_back(tool);
}

void Toolbar::Clear() {
    m_Tools.clear();
    m_ActiveTool.clear();
}

void Toolbar::SetActiveTool(const std::string& iconName) {
    m_ActiveTool = iconName;
    
    for (auto& tool : m_Tools) {
        if (tool.button && !tool.isSeparator) {
            auto toolBtn = std::dynamic_pointer_cast<ToolButton>(tool.button);
            if (toolBtn) {
                toolBtn->SetActive(tool.iconName == iconName);
            }
        }
    }
}

std::shared_ptr<Widget> Toolbar::HitToolAt(const Point& position) const {
    for (auto it = m_Tools.rbegin(); it != m_Tools.rend(); ++it) {
        if (it->button && it->button->IsVisible() && it->button->GetGeometry().Contains(position)) {
            return it->button;
        }
    }
    return nullptr;
}

void Toolbar::OnMouseDown(const MouseEvent& event) {
    if (auto hit = HitToolAt(event.position)) {
        hit->OnMouseDown(event);
    }
}

void Toolbar::OnMouseMove(const MouseEvent& event) {
    if (auto hit = HitToolAt(event.position)) {
        hit->OnMouseMove(event);
    }
}

void Toolbar::OnMouseUp(const MouseEvent& event) {
    if (auto hit = HitToolAt(event.position)) {
        hit->OnMouseUp(event);
    }
}

bool Toolbar::ShowsPointerCursor(const Point& position) const {
    if (auto hit = HitToolAt(position)) {
        return hit->ShowsPointerCursor(position);
    }
    return false;
}

// ToolbarSeparator is rendered as a subtle 1px vertical divider
ToolbarSeparator::ToolbarSeparator() {}
Size ToolbarSeparator::Measure(const Size& availableSize) { return Size{1.0f, 16.0f}; }
void ToolbarSeparator::Arrange(const Rect& allottedRect) { m_Geometry = allottedRect; }
void ToolbarSeparator::Paint(PaintContext& context) {
    float centerY = m_Geometry.y + m_Geometry.height / 2.0f;
    Rect lineRect{ m_Geometry.x, centerY - 8.0f, 1.0f, 16.0f };
    context.DrawRect(lineRect, Theme::Get().Separator);
}

} // namespace we::editor::toolbar::UI
