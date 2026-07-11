#include "Widgets/Panel.h"
#include "Core/PaintContext.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include <functional>
#include <algorithm>

namespace WindEffects::Editor::UI {

Panel::Panel(const std::string& title)
    : m_Title(title)
{
    m_HeaderHeight = 28.0f;
}

void Panel::SetTabBrand(VkDescriptorSet descriptor, float logicalSize) {
    m_TabBrandDescriptor = descriptor;
    m_TabBrandLogicalSize = logicalSize;
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
        float contentHeight = std::max(0.0f, allottedRect.y + allottedRect.height - currentY);
        m_ContentRect = Rect{
            allottedRect.x,
            currentY,
            allottedRect.width,
            contentHeight
        };
        m_Content->Arrange(m_ContentRect);
    }
    
    CalculateHeaderGeometries();
}

void Panel::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    Color panelBodyColor = ThemeColor(ThemeToken::PanelBackground);
    Color headerBg = ThemeColor(ThemeToken::HeaderBackground);
    Color tabBg = m_HeaderHovered ? ThemeColor(ThemeToken::HoverBackground) : ThemeColor(ThemeToken::HeaderBackground);
    Color tabBorder = ThemeColor(ThemeToken::BorderDefault);

    if (!m_TransparentBackground) {
        context.DrawRect(m_Geometry, panelBodyColor);
    } else if (m_Toolbar) {
        context.DrawRect(m_ToolbarRect, panelBodyColor);
    }

    if (m_HeaderHeight > 0.0f) {
        context.DrawRect(m_HeaderRect, headerBg);

        const float fontSize = ThemeMetric(ThemeToken::TextSizeTabs) * uiScale;
        const float iconSize = ThemeMetric(ThemeToken::IconSizeNavigation) * uiScale;
        const float tabPaddingH = ThemeMetric(ThemeToken::Space3) * uiScale;

        float textWidth = context.GetTextWidth(m_Title, fontSize, true);

        std::string panelIcon = m_TabIconName;
        float panelIconWidth = 0.0f;
        if (!panelIcon.empty()) {
            panelIconWidth = iconSize + ThemeMetric(ThemeToken::Space2) * uiScale;
        }

        float closeBtnWidth = iconSize + ThemeMetric(ThemeToken::Space2) * uiScale;
        float tabWidth = tabPaddingH + panelIconWidth + textWidth + closeBtnWidth + tabPaddingH;
        tabWidth = std::clamp(tabWidth, 96.0f * uiScale, 240.0f * uiScale);

        Rect tabRect{ m_HeaderRect.x, m_HeaderRect.y, tabWidth, m_HeaderHeight };

        Color activeTabBg = m_HeaderHovered ? ThemeColor(ThemeToken::HoverBackground) : ThemeColor(ThemeToken::PanelBackground);
        context.DrawRect(tabRect, activeTabBg);

        Rect accentLine{
            tabRect.x,
            tabRect.y + tabRect.height - 2.0f * uiScale,
            tabRect.width,
            2.0f * uiScale
        };
        context.DrawRect(accentLine, ThemeColor(ThemeToken::ActiveTabLine));

        context.DrawRect(
            Rect{ m_HeaderRect.x, m_HeaderRect.y + m_HeaderRect.height - 1.0f, m_HeaderRect.width, 1.0f },
            ThemeColor(ThemeToken::BorderDefault));

        float currentX = tabRect.x + tabPaddingH;
        if (!panelIcon.empty()) {
            float pIconY = m_HeaderRect.y + (m_HeaderHeight - iconSize) / 2.0f;
            Color pIconColor = m_HeaderHovered ? ThemeColor(ThemeToken::IconHover) : ThemeColor(ThemeToken::IconDefault);
            IconPainter::DrawIcon(context, panelIcon, Rect{ currentX, pIconY, iconSize, iconSize }, pIconColor);
            currentX += panelIconWidth;
        }

        float titleY = m_HeaderRect.y + (m_HeaderHeight - fontSize) / 2.0f;
        context.DrawText(m_Title, Point{ currentX, titleY }, ThemeColor(ThemeToken::TextPrimary), fontSize, true);

        float headerPad = ThemeMetric(ThemeToken::Space2) * uiScale;
        const float kOptionsWidth = 12.0f * uiScale;
        const float kOptionsHeight = 14.0f * uiScale;
        float optionsX = m_HeaderRect.x + m_HeaderRect.width - headerPad - kOptionsWidth;
        float optionsY = m_HeaderRect.y + (m_HeaderHeight - kOptionsHeight) * 0.5f;
        m_OptionsMenuRect = Rect{ optionsX, optionsY, kOptionsWidth, kOptionsHeight };

        Color optionsColor = m_OptionsMenuHovered ? ThemeColor(ThemeToken::IconHover) : ThemeColor(ThemeToken::IconDefault);
        IconPainter::DrawVerticalMoreMenu(context, m_OptionsMenuRect, optionsColor);

        float actionX = optionsX - m_ActionSpacing - iconSize;
        for (auto& action : m_HeaderActions) {
            if (action.iconName == Icons::XName) {
                float closeX = tabRect.x + tabRect.width - tabPaddingH - iconSize;
                float closeY = m_HeaderRect.y + (m_HeaderHeight - iconSize) / 2.0f;
                action.geometry = Rect{ closeX, closeY, iconSize, iconSize };

                Color closeColor = m_HeaderHovered ? ThemeColor(ThemeToken::IconHover) : ThemeColor(ThemeToken::IconDefault);
                IconPainter::DrawIcon(context, Icons::XName, Rect{ closeX, closeY, iconSize, iconSize }, closeColor);
                continue;
            }

            float actionY = m_HeaderRect.y + (m_HeaderHeight - iconSize) / 2.0f;
            action.geometry = Rect{ actionX, actionY, iconSize, iconSize };
            Color actionColor = m_HeaderHovered ? ThemeColor(ThemeToken::IconHover) : ThemeColor(ThemeToken::IconDefault);
            IconPainter::DrawIcon(context, action.iconName, action.geometry, actionColor);
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

} // namespace WindEffects::Editor::UI
