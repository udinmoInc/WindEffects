#include "Toolbar.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Icon.hpp"
#include "ToolButton.hpp"
#include <algorithm>

namespace HouseEngine::UI {

Toolbar::Toolbar()
    : m_Style(WidgetStyle::Panel())
{}

Size Toolbar::Measure(const Size& availableSize) {
    float totalWidth = 8.0f; // left padding
    
    for (auto& tool : m_Tools) {
        if (tool.isSeparator) {
            totalWidth += ToolSeparator::SEPARATOR_WIDTH + m_Spacing;
        } else if (tool.button) {
            Size btnSize = tool.button->Measure(availableSize);
            totalWidth += btnSize.width + m_Spacing;
        }
    }
    
    totalWidth += 8.0f; // right padding
    
    return Size{ totalWidth, m_Height };
}

void Toolbar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    
    float x = allottedRect.x + 8.0f;
    float y = allottedRect.y + (m_Height - m_IconSize - Theme::Get().PaddingIconBtn.top - Theme::Get().PaddingIconBtn.bottom) / 2.0f;
    
    for (auto& tool : m_Tools) {
        if (tool.isSeparator) {
            float sepHeight = ToolSeparator::SEPARATOR_HEIGHT;
            float sepY = allottedRect.y + (m_Height - sepHeight) / 2.0f;
            
            if (!tool.button) {
                tool.button = std::make_shared<ToolSeparator>();
            }
            
            Rect sepRect{ x, sepY, ToolSeparator::SEPARATOR_WIDTH, sepHeight };
            tool.button->Arrange(sepRect);
            
            x += ToolSeparator::SEPARATOR_WIDTH + m_Spacing;
        } else {
            if (tool.button) {
                Size btnSize = tool.button->GetDesiredSize();
                float y = allottedRect.y + (m_Height - btnSize.height) / 2.0f;
                Rect btnRect{ x, y, btnSize.width, btnSize.height };
                tool.button->Arrange(btnRect);
                x += btnSize.width + m_Spacing;
            }
        }
    }
}

void Toolbar::Paint(PaintContext& context) {
    // Draw background
    context.DrawRect(m_Geometry, Theme::Get().ToolbarBackground);
    
    // Draw separator line at bottom
    Rect separatorRect{
        m_Geometry.x,
        m_Geometry.y + m_Geometry.height - 1.0f,
        m_Geometry.width,
        1.0f
    };
    context.DrawRect(separatorRect, Theme::Get().Separator);
    

    // Draw tools
    for (const auto& tool : m_Tools) {
        if (tool.button) {
            tool.button->Paint(context);
        }
    }
}

void Toolbar::AddTool(const std::string& iconName, const std::string& label, std::function<void()> onClick, const std::string& tooltip, bool isPlayButton) {
    ToolInfo tool;
    tool.iconName = iconName;
    tool.label = label;
    tool.onClick = onClick;
    tool.tooltip = tooltip;
    tool.isPlayButton = isPlayButton;
    tool.isSeparator = false;
    
    auto btn = std::make_shared<ToolButton>(iconName, label, onClick, tooltip);
    if (isPlayButton) {
        btn->SetButtonStyle(ToolButtonStyle::PlayButton);
    }
    tool.button = btn;
    
    m_Tools.push_back(tool);
}

void Toolbar::AddSeparator() {
    ToolInfo tool;
    tool.isSeparator = true;
    m_Tools.push_back(tool);
}

void Toolbar::AddGroup(const std::vector<std::pair<std::string, std::function<void()>>>& tools) {
    if (!m_Tools.empty()) {
        AddSeparator();
    }
    
    for (const auto& [iconName, onClick] : tools) {
        AddTool(iconName, "", onClick);
    }
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

// ToolbarGroup implementation
ToolbarGroup::ToolbarGroup() {}

Size ToolbarGroup::Measure(const Size& availableSize) {
    float totalWidth = 0.0f;
    
    for (size_t i = 0; i < m_Tools.size(); ++i) {
        if (i > 0) {
            totalWidth += m_Spacing;
        }
        totalWidth += 20.0f + 8.0f; // icon size + padding
    }
    
    return Size{ totalWidth, 32.0f };
}

void ToolbarGroup::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    
    float x = allottedRect.x;
    float y = allottedRect.y + (allottedRect.height - 24.0f) / 2.0f;
    
    for (auto& tool : m_Tools) {
        Rect toolRect{ x, y, 24.0f, 24.0f };
        tool->Arrange(toolRect);
        x += 24.0f + m_Spacing;
    }
}

void ToolbarGroup::Paint(PaintContext& context) {
    // Draw group background
    context.DrawRoundedRect(m_Geometry, Theme::Get().PanelBackground, 4.0f);
    
    // Draw tools
    for (const auto& tool : m_Tools) {
        tool->Paint(context);
    }
}

void ToolbarGroup::AddTool(const std::string& iconName, const std::string& label, std::function<void()> onClick, const std::string& tooltip) {
    auto tool = std::make_shared<ToolButton>(iconName, label, onClick, tooltip);
    m_Tools.push_back(tool);
}

void ToolbarGroup::Clear() {
    m_Tools.clear();
}

} // namespace HouseEngine::UI
