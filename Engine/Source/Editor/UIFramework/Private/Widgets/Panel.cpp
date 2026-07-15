#include "Widgets/Panel.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
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
    m_HeaderHeight = ThemeMetric(ThemeToken::PanelTabHeight);
}

void Panel::SetTabBrand(we::rhi::RHIDescriptorSetHandle descriptor, float logicalSize) {
    m_TabBrandDescriptor = descriptor;
    m_TabBrandLogicalSize = logicalSize;
}

void Panel::SetToolbar(const std::shared_ptr<Widget>& toolbar) {
    if (m_Toolbar) RemoveChild(m_Toolbar);
    m_Toolbar = toolbar;
    if (m_Toolbar) AddChild(m_Toolbar);
}

Size Panel::Measure(const Size& availableSize) {
    float reservedHeight = m_HeaderHeight;

    if (m_Toolbar && !m_FloatingToolbar) {
        Size tbSize = m_Toolbar->Measure(availableSize);
        reservedHeight += tbSize.height;
    }

    if (m_Content) {
        const float contentHeight = std::max(0.0f, availableSize.height - reservedHeight);
        m_Content->Measure(Size{ availableSize.width, contentHeight });
    }

    return Size{ availableSize.width, availableSize.height };
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

    // Docked toolbar sits below the header; content starts underneath it.
    if (m_Toolbar && !m_FloatingToolbar) {
        m_Toolbar->Measure(Size{ allottedRect.width, allottedRect.height });
        const Size tbSize = m_Toolbar->GetDesiredSize();
        m_ToolbarRect = Rect{
            allottedRect.x,
            currentY,
            allottedRect.width,
            tbSize.height
        };
        m_Toolbar->Arrange(m_ToolbarRect);
        currentY += tbSize.height;
    }

    if (m_Content) {
        const float contentHeight = std::max(0.0f, allottedRect.y + allottedRect.height - currentY);
        m_ContentRect = Rect{
            allottedRect.x,
            currentY,
            allottedRect.width,
            contentHeight
        };
        m_Content->Arrange(m_ContentRect);
    }

    // Floating toolbar overlays the top of the content region.
    if (m_Toolbar && m_FloatingToolbar) {
        const float uiScale = std::max(1.0f, DPIContext::GetScale());
        const float inset = ThemeMetric(ThemeToken::Space2) * uiScale;
        m_Toolbar->Measure(m_ContentRect.width > 0.0f
            ? Size{ m_ContentRect.width, m_ContentRect.height }
            : Size{ allottedRect.width, allottedRect.height });
        const Size tbSize = m_Toolbar->GetDesiredSize();
        m_ToolbarRect = Rect{
            m_ContentRect.x + inset,
            m_ContentRect.y + inset,
            std::max(0.0f, m_ContentRect.width - inset * 2.0f),
            tbSize.height
        };
        m_Toolbar->Arrange(m_ToolbarRect);
    }
    
    CalculateHeaderGeometries();
}

void Panel::Paint(PaintContext& context) {
    if (!m_TransparentBackground) {
        PanelChrome::PaintContentRegion(context, m_Geometry);
    } else if (m_Toolbar && !m_FloatingToolbar) {
        PanelChrome::PaintToolbarRegion(context, m_ToolbarRect);
    }

    if (m_HeaderHeight > 0.0f) {
        std::vector<PanelChrome::FloatingHeaderAction> chromeActions;
        chromeActions.reserve(m_HeaderActions.size());
        for (size_t i = 0; i < m_HeaderActions.size(); ++i) {
            PanelChrome::FloatingHeaderAction action{};
            action.iconName = m_HeaderActions[i].iconName;
            action.hovered = static_cast<int>(i) == m_HoveredActionIndex;
            action.pressed = static_cast<int>(i) == m_PressedActionIndex;
            chromeActions.push_back(action);
        }

        PanelChrome::PaintFloatingPanelHeader(
            context,
            m_HeaderRect,
            m_Title,
            m_TabIconName,
            HasTabBrand(),
            m_TabBrandDescriptor,
            m_TabBrandLogicalSize,
            chromeActions,
            HasOptionsMenuHandler(),
            m_OptionsMenuHovered,
            m_OptionsMenuRect);

        const float buttonSize = PanelChrome::HeaderButtonSize();
        const float padH = PanelChrome::TabPadH();
        const float gap = ThemeMetric(ThemeToken::Space1) * PanelChrome::UiScale();
        const float centerY = m_HeaderRect.y + m_HeaderRect.height * 0.5f;
        float actionX = m_HeaderRect.x + m_HeaderRect.width - padH - buttonSize;

        if (HasOptionsMenuHandler()) {
            actionX -= buttonSize + gap;
        }

        for (int i = static_cast<int>(m_HeaderActions.size()) - 1; i >= 0; --i) {
            m_HeaderActions[static_cast<size_t>(i)].geometry = Rect{
                actionX,
                centerY - buttonSize * 0.5f,
                buttonSize,
                buttonSize
            };
            actionX -= buttonSize + gap;
        }
    }

    if (m_Content) {
        context.PushClipRect(m_ContentRect);
        m_Content->Paint(context);
        context.PopClipRect();
    }

    if (m_Toolbar) {
        if (!m_FloatingToolbar) {
            PanelChrome::PaintToolbarRegion(context, m_ToolbarRect);
        }
        m_Toolbar->Paint(context);
    }
}

void Panel::OnMouseDown(const MouseEvent& event) {
    if (m_HeaderHeight > 0.0f && m_HeaderRect.Contains(event.position)) {
        if (m_OptionsMenuRect.Contains(event.position)) {
            m_PressedActionIndex = -1;
            InvokeOptionsMenu();
            return;
        }

        HeaderAction* action = GetActionAtPosition(event.position);
        if (action) {
            for (size_t i = 0; i < m_HeaderActions.size(); ++i) {
                if (&m_HeaderActions[i] == action) {
                    m_PressedActionIndex = static_cast<int>(i);
                    break;
                }
            }
            if (action->onClick) {
                action->onClick();
            }
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

        m_HoveredActionIndex = -1;
        for (size_t i = 0; i < m_HeaderActions.size(); ++i) {
            if (m_HeaderActions[i].geometry.Contains(event.position)) {
                m_HoveredActionIndex = static_cast<int>(i);
                break;
            }
        }
    } else {
        m_HeaderHovered = false;
        m_OptionsMenuHovered = false;
        m_HoveredActionIndex = -1;
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
    (void)event;
    m_PressedActionIndex = -1;

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
