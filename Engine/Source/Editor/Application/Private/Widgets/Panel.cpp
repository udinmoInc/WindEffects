#include "Widgets/Panel.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include "Core/DockTabIconRegistry.hpp"
#include <functional>
#include <algorithm>

namespace we::UI {

Panel::Panel(const std::string& title)
    : m_Title(title)
    , m_Style(WidgetStyle::Panel())
    , m_HeaderStyle(WidgetStyle::Panel())
{
    m_HeaderHeight = 28.0f; // Tab size height: 28 px
}

void Panel::SetToolbar(const std::shared_ptr<Widget>& toolbar) {
    if (m_Toolbar) RemoveChild(m_Toolbar);
    m_Toolbar = toolbar;
    if (m_Toolbar) AddChild(m_Toolbar);
}

Size Panel::Measure(const Size& availableSize) {
    float totalHeight = m_HeaderHeight;
    
    if (m_Toolbar) {
        Size tbSize = m_Toolbar->Measure(availableSize);
        totalHeight += tbSize.height;
    }
    
    if (m_Content) {
        Size contentSize = m_Content->Measure(availableSize);
        totalHeight += contentSize.height;
    }
    
    return Size{ availableSize.width, std::max(totalHeight, availableSize.height) };
}

void Panel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    
    // Calculate header rect
    m_HeaderRect = Rect{
        allottedRect.x,
        allottedRect.y,
        allottedRect.width,
        m_HeaderHeight
    };
    
    float currentY = allottedRect.y + m_HeaderHeight;
    
    // Calculate toolbar rect
    if (m_Toolbar) {
        Size tbSize = m_Toolbar->GetDesiredSize();
        m_ToolbarRect = Rect{
            allottedRect.x,
            currentY,
            allottedRect.width,
            tbSize.height
        };
        m_Toolbar->Arrange(m_ToolbarRect);
        currentY += tbSize.height;
    }
    
    // Calculate content rect
    if (m_Content) {
        m_ContentRect = Rect{
            allottedRect.x,
            currentY,
            allottedRect.width,
            allottedRect.y + allottedRect.height - currentY
        };
        m_Content->Arrange(m_ContentRect);
    }
    
    CalculateHeaderGeometries();
}

void Panel::Paint(PaintContext& context) {
    Color panelBodyColor{0.145f, 0.145f, 0.145f, 1.0f}; // #252525
    Color headerBg = Theme::Get().HeaderBackground;
    Color tabBg = m_HeaderHovered ? Color{0.196f, 0.196f, 0.196f, 1.0f} : Color{0.173f, 0.173f, 0.173f, 1.0f};
    Color tabBorder{0.227f, 0.227f, 0.227f, 1.0f};
    Color textColor{0.878f, 0.878f, 0.878f, 1.0f};
    Color shadowColor{0.0f, 0.0f, 0.0f, 0.3f};

    context.DrawRect(m_Geometry, panelBodyColor);

    if (m_HeaderHeight > 0.0f) {
        context.DrawRect(m_HeaderRect, headerBg);

        float fontSize = 13.0f;
        float iconSize = 16.0f;
        float tabPaddingH = 12.0f;

        float textWidth = context.GetTextWidth(m_Title, fontSize);

        std::string panelIcon = DockTabIconRegistry::Get().GetIcon(m_Title);
        float panelIconWidth = 0.0f;
        if (!panelIcon.empty()) {
            panelIconWidth = iconSize + 6.0f;
        }

        float closeBtnWidth = iconSize + 6.0f;
        float tabWidth = tabPaddingH + panelIconWidth + textWidth + closeBtnWidth + tabPaddingH;
        tabWidth = std::clamp(tabWidth, 80.0f, 220.0f);

        Rect tabRect{ m_HeaderRect.x, m_HeaderRect.y, tabWidth, m_HeaderHeight };

        context.DrawRoundedRect(tabRect, tabBg, 4.0f);

        float flattenHeight = 4.0f;
        context.DrawRect(Rect{tabRect.x, tabRect.y + tabRect.height - flattenHeight, tabRect.width, flattenHeight}, tabBg);
        context.DrawRoundedRectOutline(tabRect, tabBorder, 1.0f, 4.0f);
        context.DrawRect(Rect{tabRect.x, tabRect.y + tabRect.height - flattenHeight, 1.0f, flattenHeight}, tabBorder);
        context.DrawRect(Rect{tabRect.x + tabRect.width - 1.0f, tabRect.y + tabRect.height - flattenHeight, 1.0f, flattenHeight}, tabBorder);
        context.DrawRect(Rect{tabRect.x + 1.0f, tabRect.y + tabRect.height - 1.0f, tabRect.width - 2.0f, 1.0f}, tabBg);

        context.DrawRect(Rect{m_HeaderRect.x, m_HeaderRect.y + m_HeaderRect.height - 1.0f, m_HeaderRect.width, 1.0f}, shadowColor);
        context.DrawRect(Rect{tabRect.x, m_HeaderRect.y + m_HeaderRect.height - 1.0f, tabRect.width, 1.0f}, tabBg);

        float currentX = tabRect.x + tabPaddingH;
        if (!panelIcon.empty()) {
            float pIconY = m_HeaderRect.y + (m_HeaderHeight - iconSize) / 2.0f;
            Color pIconColor = Color{0.878f, 0.878f, 0.878f, 1.0f};
            int codepoint = Icons::GetCodepoint(panelIcon);
            if (codepoint != 0) {
                context.DrawIcon(codepoint, Point{ currentX, pIconY }, pIconColor, iconSize);
            }
            currentX += panelIconWidth;
        }

        float titleY = m_HeaderRect.y + (m_HeaderHeight - fontSize) / 2.0f;
        context.DrawText(m_Title, Point{ currentX, titleY }, textColor, fontSize, false);

        float headerPad = 8.0f;
        constexpr float kOptionsWidth = 12.0f;
        constexpr float kOptionsHeight = 14.0f;
        float optionsX = m_HeaderRect.x + m_HeaderRect.width - headerPad - kOptionsWidth;
        float optionsY = m_HeaderRect.y + (m_HeaderHeight - kOptionsHeight) * 0.5f;
        m_OptionsMenuRect = Rect{ optionsX, optionsY, kOptionsWidth, kOptionsHeight };

        Color optionsColor = m_OptionsMenuHovered ? Theme::Get().TextPrimary : Theme::Get().TextSecondary;
        IconPainter::DrawVerticalMoreMenu(context, m_OptionsMenuRect, optionsColor);

        float actionX = optionsX - m_ActionSpacing - iconSize;
        for (auto& action : m_HeaderActions) {
            if (action.iconName == Icons::XName) {
                float closeX = tabRect.x + tabRect.width - tabPaddingH - iconSize;
                float closeY = m_HeaderRect.y + (m_HeaderHeight - iconSize) / 2.0f;
                action.geometry = Rect{ closeX, closeY, iconSize, iconSize };

                Color closeColor = Theme::Get().TextSecondary;
                int crossCp = Icons::GetCodepoint(Icons::XName);
                if (crossCp != 0) {
                    context.DrawIcon(crossCp, Point{ closeX, closeY }, closeColor, iconSize);
                }
                continue;
            }

            float actionY = m_HeaderRect.y + (m_HeaderHeight - iconSize) / 2.0f;
            action.geometry = Rect{ actionX, actionY, iconSize, iconSize };
            IconPainter::DrawIcon(context, action.iconName, action.geometry, textColor);
            actionX -= (iconSize + m_ActionSpacing);
        }
    }
    // Draw toolbar
    if (m_Toolbar) {
        m_Toolbar->Paint(context);
        
        // Draw subtle 1px divider below toolbar
        Rect toolbarDividerRect{
            m_ToolbarRect.x,
            m_ToolbarRect.y + m_ToolbarRect.height - 1.0f,
            m_ToolbarRect.width,
            1.0f
        };
        context.DrawRect(toolbarDividerRect, tabBorder);
    }
    
    // Draw content (always expanded now since it's a dock tab)
    if (m_Content) {
        m_Content->Paint(context);
    }
}

void Panel::OnMouseDown(const MouseEvent& event) {
    if (m_HeaderHeight > 0.0f && m_HeaderRect.Contains(event.position)) {
        if (m_OptionsMenuRect.Contains(event.position)) {
            InvokeOptionsMenu();
            return;
        }

        HeaderAction* action = GetActionAtPosition(event.position);
        if (action && action->onClick) {
            action->onClick();
            return;
        }
        return;
    }

    if (m_Toolbar && m_ToolbarRect.Contains(event.position)) {
        m_Toolbar->OnMouseDown(event);
        return;
    }

    if (m_Content && m_ContentRect.Contains(event.position)) {
        m_Content->OnFocus();
        m_Content->OnMouseDown(event);
    }
}

void Panel::OnMouseWheel(const MouseEvent& event) {
    if (m_Toolbar && m_ToolbarRect.Contains(event.position)) {
        m_Toolbar->OnMouseWheel(event);
        return;
    }

    if (m_Content && m_ContentRect.Contains(event.position)) {
        m_Content->OnMouseWheel(event);
    }
}

void Panel::OnMouseMove(const MouseEvent& event) {
    if (m_HeaderHeight > 0.0f) {
        m_HeaderHovered = m_HeaderRect.Contains(event.position);
        m_OptionsMenuHovered = m_OptionsMenuRect.Contains(event.position);
    } else {
        m_HeaderHovered = false;
        m_OptionsMenuHovered = false;
    }

    if (m_Toolbar && m_ToolbarRect.Contains(event.position)) {
        m_Toolbar->OnMouseMove(event);
        return;
    }

    if (m_Content && m_ContentRect.Contains(event.position)) {
        m_Content->OnMouseMove(event);
    }
}

void Panel::OnMouseUp(const MouseEvent& event) {
    if (m_Toolbar && m_ToolbarRect.Contains(event.position)) {
        m_Toolbar->OnMouseUp(event);
        return;
    }

    if (m_Content && m_ContentRect.Contains(event.position)) {
        m_Content->OnMouseUp(event);
    }
}

void Panel::SetContent(const std::shared_ptr<Widget>& content) {
    if (m_Content) RemoveChild(m_Content);
    m_Content = content;
    if (m_Content) AddChild(m_Content);
}

void Panel::SetExpanded(bool expanded) {
    if (m_Expanded == expanded) return;
    
    m_Expanded = expanded;
    Arrange(m_Geometry);
}

void Panel::AddHeaderAction(const std::string& iconName, std::function<void()> onClick) {
    HeaderAction action;
    action.iconName = iconName;
    action.onClick = onClick;
    m_HeaderActions.push_back(action);
    CalculateHeaderGeometries();
}

void Panel::InvokeOptionsMenu() const {
    if (m_OnOptionsMenu) {
        m_OnOptionsMenu();
    }
}

void Panel::CalculateHeaderGeometries() {
    // We don't pre-calculate action geometries anymore since they depend on the tab width
    // which is calculated in Paint(). The geometries are updated during Paint().
}

Panel::HeaderAction* Panel::GetActionAtPosition(const Point& pos) {
    for (auto& action : m_HeaderActions) {
        if (pos.x >= action.geometry.x && pos.x <= action.geometry.x + action.geometry.width &&
            pos.y >= action.geometry.y && pos.y <= action.geometry.y + action.geometry.height) {
            return &action;
        }
    }
    return nullptr;
}

} // namespace we::editor::application::UI
