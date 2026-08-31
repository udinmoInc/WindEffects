#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include <functional>
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::DPIContext;

namespace we::editor::panels {
namespace Chrome = ::we::editor::panels::PanelChrome;

Panel::Panel(const std::string& title)
    : m_Title(title)
    , m_BodyLayout(std::make_shared<PanelBodyLayout>())
{
    m_HeaderHeight = ThemeMetric(MetricToken::PanelTabHeight);
}

void Panel::AttachBodyLayout() {
    if (m_BodyLayout && !m_BodyLayout->GetParent()) {
        m_BodyLayout->SetSuppressContentSurfaces(m_TransparentBackground);
        m_BodyLayout->SetOverlayToolbar(m_FloatingToolbar);
        AddChild(m_BodyLayout);
    }
}

void Panel::SetTabBrand(we::rhi::RHIDescriptorSetHandle descriptor, float logicalSize) {
    m_TabBrandDescriptor = descriptor;
    m_TabBrandLogicalSize = logicalSize;
}

void Panel::SetToolbar(const std::shared_ptr<Widget>& toolbar) {
    AttachBodyLayout();
    m_BodyLayout->SetRegion(PanelBodyRegion::Toolbar, toolbar);
}

std::shared_ptr<Widget> Panel::GetToolbar() const {
    return m_BodyLayout->GetRegion(PanelBodyRegion::Toolbar);
}

void Panel::SetModeTabs(const std::shared_ptr<Widget>& modeTabs) {
    AttachBodyLayout();
    m_BodyLayout->SetRegion(PanelBodyRegion::ModeTabs, modeTabs);
}

void Panel::SetSearch(const std::shared_ptr<Widget>& search) {
    AttachBodyLayout();
    m_BodyLayout->SetRegion(PanelBodyRegion::Search, search);
}

void Panel::SetColumnHeader(const std::shared_ptr<Widget>& columnHeader) {
    AttachBodyLayout();
    m_BodyLayout->SetRegion(PanelBodyRegion::ColumnHeader, columnHeader);
}

void Panel::SetFooter(const std::shared_ptr<Widget>& footer) {
    AttachBodyLayout();
    m_BodyLayout->SetRegion(PanelBodyRegion::Footer, footer);
}

Rect Panel::GetRegionRect(const PanelBodyRegion region) const {
    return m_BodyLayout ? m_BodyLayout->GetRegionRect(region) : Rect{};
}

Size Panel::Measure(const Size& availableSize) {
    AttachBodyLayout();
    const float headerH = (m_HeaderHeight > 0.0f)
        ? (std::max)(m_HeaderHeight, PanelChrome::TabHeight())
        : 0.0f;

    Size bodyAvailable = availableSize;
    if (bodyAvailable.height < 1.0e8f) {
        bodyAvailable.height = (std::max)(0.0f, bodyAvailable.height - headerH);
    }

    Size bodySize{ 0.0f, 0.0f };
    if (m_BodyLayout) {
        bodySize = m_BodyLayout->Measure(bodyAvailable);
    }

    const float desiredW = (availableSize.width < 1.0e8f)
        ? availableSize.width
        : bodySize.width;

    const float desiredH = (availableSize.height < 1.0e8f)
        ? availableSize.height
        : (headerH + bodySize.height);

    m_DesiredSize = ClampDesiredSize(Size{ desiredW, desiredH });
    return m_DesiredSize;
}

void Panel::Arrange(const Rect& allottedRect) {
    AttachBodyLayout();
    m_Geometry = allottedRect;

    const float headerH = (m_HeaderHeight > 0.0f)
        ? (std::max)(m_HeaderHeight, PanelChrome::TabHeight())
        : 0.0f;

    m_HeaderRect = Rect{
        allottedRect.x,
        allottedRect.y,
        allottedRect.width,
        headerH
    };

    const float bodyY = allottedRect.y + headerH;
    const float bodyHeight = (std::max)(0.0f, allottedRect.y + allottedRect.height - bodyY);
    if (m_BodyLayout) {
        m_BodyLayout->Arrange(Rect{
            allottedRect.x,
            bodyY,
            allottedRect.width,
            bodyHeight
        });
    }

    CalculateHeaderGeometries();
}

void Panel::CalculateHeaderGeometries() {
    if (m_HeaderHeight <= 0.0f) {
        m_OptionsMenuRect = {};
        for (auto& action : m_HeaderActions) {
            action.geometry = {};
        }
        return;
    }

    Chrome::LayoutFloatingPanelHeaderGeometries(
        m_HeaderRect,
        HasOptionsMenuHandler(),
        m_HeaderActions.size(),
        m_OptionsMenuRect,
        [this](const size_t index, const Rect& actionRect) {
            m_HeaderActions[index].geometry = actionRect;
        });
}

void Panel::Paint(PaintContext& context) {
    AttachBodyLayout();
    if (!m_TransparentBackground && !m_BodyLayout) {
        if (m_HeaderHeight > 0.0f) {
            const float bodyY = m_Geometry.y + m_HeaderRect.height;
            const float bodyH = (std::max)(0.0f, m_Geometry.height - m_HeaderRect.height);
            if (bodyH > 0.0f) {
                Chrome::PaintPanelSurface(
                    context,
                    Rect{ m_Geometry.x, bodyY, m_Geometry.width, bodyH });
            }
        }
    }

    if (m_HeaderHeight > 0.0f) {
        std::vector<Chrome::FloatingHeaderAction> chromeActions;
        chromeActions.reserve(m_HeaderActions.size());
        for (size_t i = 0; i < m_HeaderActions.size(); ++i) {
            Chrome::FloatingHeaderAction action{};
            action.icon = m_HeaderActions[i].icon;
            action.hovered = static_cast<int>(i) == m_HoveredActionIndex;
            action.pressed = static_cast<int>(i) == m_PressedActionIndex;
            chromeActions.push_back(action);
        }

        Chrome::PaintFloatingPanelHeader(
            context,
            m_HeaderRect,
            m_Title,
            m_TabIcon,
            HasTabBrand(),
            m_TabBrandDescriptor,
            m_TabBrandLogicalSize,
            chromeActions,
            HasOptionsMenuHandler(),
            m_OptionsMenuHovered,
            m_OptionsMenuRect);
    }

    if (m_BodyLayout) {
        m_BodyLayout->Paint(context);
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

    if (m_BodyLayout) {
        m_BodyLayout->OnMouseDown(event);
    }
}

void Panel::OnMouseWheel(const MouseEvent& event) {
    if (m_BodyLayout) {
        m_BodyLayout->OnMouseWheel(event);
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

    if (m_BodyLayout) {
        m_BodyLayout->OnMouseMove(event);
    }
}

void Panel::OnMouseUp(const MouseEvent& event) {
    (void)event;
    m_PressedActionIndex = -1;

    if (m_BodyLayout) {
        m_BodyLayout->OnMouseUp(event);
    }
}

void Panel::SetContent(const std::shared_ptr<Widget>& content) {
    AttachBodyLayout();
    m_BodyLayout->SetRegion(PanelBodyRegion::Content, content);
}

std::shared_ptr<Widget> Panel::GetContent() const {
    return m_BodyLayout->GetRegion(PanelBodyRegion::Content);
}

void Panel::SetExpanded(bool expanded) {
    if (m_Expanded == expanded) return;
    
    m_Expanded = expanded;
    Arrange(m_Geometry);
}

void Panel::AddHeaderAction(we::runtime::kindui::WindIconRef icon, std::function<void()> onClick) {
    HeaderAction action;
    action.icon = icon;
    action.onClick = onClick;
    m_HeaderActions.push_back(action);
    CalculateHeaderGeometries();
}

void Panel::InvokeOptionsMenu() const {
    if (m_OnOptionsMenu) {
        m_OnOptionsMenu();
    }
}

std::shared_ptr<Widget> Panel::HitTestPoint(const Point& pos, const Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }
    if ((clip != nullptr && !clip->Contains(pos)) || !m_Geometry.Contains(pos)) {
        return nullptr;
    }

    if (m_HeaderHeight > 0.0f && m_HeaderRect.Contains(pos)) {
        return shared_from_this();
    }

    if (m_BodyLayout) {
        if (auto hit = m_BodyLayout->HitTestPoint(pos, clip)) {
            return hit;
        }
    }

    return shared_from_this();
}

Panel::HeaderAction* Panel::GetActionAtPosition(const Point& pos) {
    for (auto& action : m_HeaderActions) {
        if (action.geometry.Contains(pos)) {
            return &action;
        }
    }
    return nullptr;
}

} // namespace we::editor::panels
