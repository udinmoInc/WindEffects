#include "WindEffects/Editor/UI/Widgets/DockContainer.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Layout/LayoutAssert.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/UiMetrics.h"
#include "KindUI/Core/Animator.h"
#include <algorithm>
#include <cmath>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::AssertLayoutRectValid;
using ::we::runtime::kindui::ClampRectToParent;

namespace we::editor::docking {
namespace PanelChrome = ::we::editor::panels::PanelChrome;

DockContainer::DockContainer() {
    m_HeaderHeightLogical = ThemeMetric(MetricToken::PanelTabHeight);
}

float DockContainer::GetHeaderHeightDevice() const {
    return DPIContext::Snap(m_HeaderHeightLogical * DPIContext::GetScale());
}

void DockContainer::AddPanel(const std::shared_ptr<Panel>& panel) {
    if (!panel) return;
    panel->SetHeaderHeight(0.0f);
    m_Tabs.push_back({panel, Rect{}, Rect{}, false, false, 0.0f});
    if (m_ActiveTabIndex == -1) {
        m_ActiveTabIndex = 0;
    }
    AddChild(panel);
}

void DockContainer::RemovePanel(const std::shared_ptr<Panel>& panel) {
    auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(),
        [&](const TabInfo& info) { return info.panel == panel; });

    if (it != m_Tabs.end()) {
        int index = static_cast<int>(std::distance(m_Tabs.begin(), it));
        m_Tabs.erase(it);
        RemoveChild(panel);

        if (m_Tabs.empty()) {
            m_ActiveTabIndex = -1;
        } else if (m_ActiveTabIndex >= static_cast<int>(m_Tabs.size())) {
            m_ActiveTabIndex = static_cast<int>(m_Tabs.size()) - 1;
        } else if (m_ActiveTabIndex == index) {
            m_ActiveTabIndex = std::max(0, m_ActiveTabIndex - 1);
        }
    }
}

bool DockContainer::ContainsPanel(const std::shared_ptr<Panel>& panel) const {
    return std::any_of(m_Tabs.begin(), m_Tabs.end(),
        [&](const TabInfo& info) { return info.panel == panel; });
}

void DockContainer::FocusPanel(const std::shared_ptr<Panel>& panel) {
    for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
        if (m_Tabs[static_cast<size_t>(i)].panel == panel) {
            SetActiveTab(i);
            return;
        }
    }
}

void DockContainer::SetActiveTab(int index) {
    if (index >= 0 && index < static_cast<int>(m_Tabs.size())) {
        if (m_ActiveTabIndex != index) {
            m_ActiveTabIndex = index;
            if (m_OnActiveTabChanged) {
                m_OnActiveTabChanged(m_ActiveTabIndex);
            }
        }
    }
}

void DockContainer::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    const float speed = 12.0f;
    bool hoverChanged = false;
    for (auto& tab : m_Tabs) {
        const float target = tab.isHovered ? 1.0f : 0.0f;
        const float previous = tab.hoverAnim;
        tab.hoverAnim += (target - tab.hoverAnim) * std::min(1.0f, deltaTime * speed);
        if (std::abs(tab.hoverAnim - previous) > we::runtime::kindui::Animator::kSettleEpsilon) {
            hoverChanged = true;
        }
    }
    if (hoverChanged) {
        InvalidateLayout();
        InvalidatePaint();
    }
}

Size DockContainer::Measure(const Size& availableSize) {
    const float headerHeight = GetHeaderHeightDevice();

    PaintContext context;
    float requiredTabWidth = 0.0f;
    for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
        const auto& tabInfo = m_Tabs[static_cast<size_t>(i)];
        const bool isActive = (i == m_ActiveTabIndex);
        requiredTabWidth += MeasureTabWidth(context, tabInfo, isActive, i == 0);
        if (i + 1 < static_cast<int>(m_Tabs.size())) {
            requiredTabWidth += PanelChrome::TabGap();
        }
    }

    float desiredW = (availableSize.width < 1.0e8f) ? availableSize.width : requiredTabWidth;
    float desiredH = (availableSize.height < 1.0e8f) ? availableSize.height : headerHeight;

    Size panelContentDesired{ 0.0f, 0.0f };
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;

        Size contentAvailable = availableSize;
        if (contentAvailable.height < 1.0e8f) {
            contentAvailable.height = (std::max)(0.0f, contentAvailable.height - headerHeight);
        }

        panelContentDesired = activePanel->Measure(contentAvailable);
    }

    if (availableSize.width >= 1.0e8f) {
        desiredW = (std::max)({ desiredW, requiredTabWidth, panelContentDesired.width });
    }
    if (availableSize.height >= 1.0e8f) {
        desiredH = headerHeight + panelContentDesired.height;
    }

    m_DesiredSize = ClampDesiredSize(Size{ desiredW, desiredH });
    return m_DesiredSize;
}

void DockContainer::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float headerHeight = GetHeaderHeightDevice();

    m_HeaderRect = ClampRectToParent(
        Rect{ allottedRect.x, allottedRect.y, allottedRect.width, headerHeight },
        allottedRect);

    m_ContentRect = ClampRectToParent(
        Rect{
            allottedRect.x,
            allottedRect.y + headerHeight,
            allottedRect.width,
            (std::max)(0.0f, allottedRect.height - headerHeight)
        },
        allottedRect);

    AssertLayoutRectValid("DockContainer.header", m_HeaderRect, allottedRect);
    AssertLayoutRectValid("DockContainer.content", m_ContentRect, allottedRect);

    for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
        auto panel = m_Tabs[static_cast<size_t>(i)].panel;
        if (i == m_ActiveTabIndex) {
            panel->Arrange(m_ContentRect);
        } else {
            panel->Arrange(Rect{0.0f, 0.0f, 0.0f, 0.0f});
        }
    }

    LayoutTabGeometries();
}

void DockContainer::LayoutTabGeometries() {
    PaintContext context;
    std::vector<PanelChrome::DockTabDescriptor> descriptors;
    descriptors.reserve(m_Tabs.size());
    for (const auto& tabInfo : m_Tabs) {
        PanelChrome::DockTabDescriptor descriptor{};
        descriptor.title = tabInfo.panel->GetTitle();
        descriptor.iconName = tabInfo.panel->GetTabIcon();
        descriptor.hasBrand = tabInfo.panel->HasTabBrand();
        descriptor.brandDescriptor = tabInfo.panel->GetTabBrandDescriptor();
        descriptor.brandLogicalSize = tabInfo.panel->GetTabBrandLogicalSize();
        descriptors.push_back(descriptor);
    }

    PanelChrome::DockTabStripState state{};
    if (m_ActiveTabIndex >= 0) {
        state.activeIndex = static_cast<size_t>(m_ActiveTabIndex);
    }
    state.showClose = [this](size_t index, bool isActive, bool /*isHovered*/) {
        const auto& tabInfo = m_Tabs[index];
        return isActive || tabInfo.isHovered;
    };

    const auto layout = PanelChrome::LayoutDockTabStrip(context, m_HeaderRect, descriptors, state);
    for (size_t i = 0; i < m_Tabs.size(); ++i) {
        m_Tabs[i].tabRect = layout.tabs[i].tabRect;
        m_Tabs[i].closeRect = layout.tabs[i].closeRect;
    }
}

std::shared_ptr<Widget> DockContainer::HitTestPoint(const Point& pos, const Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }
    if ((clip != nullptr && !clip->Contains(pos)) || !m_Geometry.Contains(pos)) {
        return nullptr;
    }

    if (m_HeaderRect.Contains(pos)) {
        for (const auto& tabInfo : m_Tabs) {
            if (tabInfo.tabRect.Contains(pos)) {
                return shared_from_this();
            }
        }
        return shared_from_this();
    }

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        Rect contentClip = m_ContentRect;
        if (clip != nullptr) {
            contentClip = contentClip.Intersect(*clip);
        }
        if (!contentClip.IsEmpty() && contentClip.Contains(pos)) {
            const auto& activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
            if (auto hit = activePanel->HitTestPoint(pos, &contentClip)) {
                return hit;
            }
        }
    }

    return shared_from_this();
}

float DockContainer::MeasureTabWidth(PaintContext& context, const TabInfo& tabInfo, bool isActive, bool flushLeft) {
    PanelChrome::DockTabDescriptor descriptor{};
    descriptor.title = tabInfo.panel->GetTitle();
    descriptor.iconName = tabInfo.panel->GetTabIcon();
    descriptor.hasBrand = tabInfo.panel->HasTabBrand();
    descriptor.brandDescriptor = tabInfo.panel->GetTabBrandDescriptor();
    descriptor.brandLogicalSize = tabInfo.panel->GetTabBrandLogicalSize();
    const bool showClose = isActive || tabInfo.isHovered;
    return PanelChrome::MeasureDockTabWidth(context, descriptor, isActive, showClose, flushLeft);
}

void DockContainer::PaintTab(PaintContext& context, TabInfo& tabInfo, int index, float& currentX) {
    (void)currentX;
    const bool isActive = (index == m_ActiveTabIndex);
    const bool showClose = isActive || tabInfo.isHovered;
    const bool flushLeft = (index == 0);

    PanelChrome::DockTabDescriptor descriptor{};
    descriptor.title = tabInfo.panel->GetTitle();
    descriptor.iconName = tabInfo.panel->GetTabIcon();
    descriptor.hasBrand = tabInfo.panel->HasTabBrand();
    descriptor.brandDescriptor = tabInfo.panel->GetTabBrandDescriptor();
    descriptor.brandLogicalSize = tabInfo.panel->GetTabBrandLogicalSize();

    PanelChrome::DockTabLayout layout{ tabInfo.tabRect, tabInfo.closeRect };
    PanelChrome::PaintDockTab(
        context,
        descriptor,
        layout,
        m_HeaderRect,
        isActive,
        tabInfo.hoverAnim,
        showClose,
        tabInfo.isCloseHovered,
        flushLeft);
}

void DockContainer::Paint(PaintContext& context) {
    std::vector<PanelChrome::DockTabDescriptor> descriptors;
    descriptors.reserve(m_Tabs.size());
    PanelChrome::DockTabStripLayout stripLayout{};
    stripLayout.tabs.reserve(m_Tabs.size());
    for (const auto& tabInfo : m_Tabs) {
        PanelChrome::DockTabDescriptor descriptor{};
        descriptor.title = tabInfo.panel->GetTitle();
        descriptor.iconName = tabInfo.panel->GetTabIcon();
        descriptor.hasBrand = tabInfo.panel->HasTabBrand();
        descriptor.brandDescriptor = tabInfo.panel->GetTabBrandDescriptor();
        descriptor.brandLogicalSize = tabInfo.panel->GetTabBrandLogicalSize();
        descriptors.push_back(descriptor);
        stripLayout.tabs.push_back({ tabInfo.tabRect, tabInfo.closeRect });
    }

    PanelChrome::DockTabStripState state{};
    if (m_ActiveTabIndex >= 0) {
        state.activeIndex = static_cast<size_t>(m_ActiveTabIndex);
    }
    state.showClose = [this](size_t index, bool isActive, bool /*isHovered*/) {
        const auto& tabInfo = m_Tabs[index];
        return isActive || tabInfo.isHovered;
    };
    state.hoverAnim = [this](size_t index) {
        return m_Tabs[index].hoverAnim;
    };
    state.closeHovered = [this](size_t index) {
        return m_Tabs[index].isCloseHovered;
    };
    PanelChrome::PaintDockTabStrip(context, m_HeaderRect, descriptors, stripLayout, state);

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        if (!m_ContentRect.IsEmpty()) {
            context.DrawRect(m_ContentRect, we::runtime::kindui::ds::Surface::Panel());
        }
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;

        context.PushClipRect(m_ContentRect);
        activePanel->Paint(context);
        context.PopClipRect();
    }
}

void DockContainer::OnMouseDown(const MouseEvent& event) {
    if (m_HeaderRect.Contains(event.position)) {
        for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
            auto& tabInfo = m_Tabs[static_cast<size_t>(i)];
            if (tabInfo.tabRect.Contains(event.position)) {
                if ((i == m_ActiveTabIndex || tabInfo.isHovered) && tabInfo.closeRect.Contains(event.position)) {
                    if (m_OnTabClosed) {
                        m_OnTabClosed(tabInfo.panel);
                    }
                    return;
                }

                m_DragTabIndex = i;
                m_DragStart = event.position;
                m_TabDragCandidate = true;
                SetActiveTab(i);
                return;
            }
        }
        return;
    }

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        const auto toolbar = activePanel->GetToolbar();
        const auto content = activePanel->GetContent();
        PanelChrome::RoutePanelBodyPointer(
            event,
            toolbar,
            toolbar ? toolbar->GetGeometry() : Rect{},
            content,
            content ? content->GetGeometry() : Rect{},
            &Widget::OnMouseDown);
    }
}

void DockContainer::OnMouseMove(const MouseEvent& event) {
    if (m_TabDragCandidate && m_DragTabIndex >= 0) {
        const float dx = event.position.x - m_DragStart.x;
        const float dy = event.position.y - m_DragStart.y;
        if (we::runtime::kindui::UiMetrics::ExceedsDragThreshold(dx, dy)) {
            m_TabDragCandidate = false;
            if (m_OnTabDragStarted && m_DragTabIndex < static_cast<int>(m_Tabs.size())) {
                m_OnTabDragStarted(m_Tabs[static_cast<size_t>(m_DragTabIndex)].panel, event.position);
            }
            m_DragTabIndex = -1;
        }
    }

    if (m_HeaderRect.Contains(event.position)) {
        for (auto& tabInfo : m_Tabs) {
            tabInfo.isHovered = tabInfo.tabRect.Contains(event.position);
            tabInfo.isCloseHovered = tabInfo.isHovered && tabInfo.closeRect.Contains(event.position);
        }
    } else {
        for (auto& tabInfo : m_Tabs) {
            tabInfo.isHovered = false;
            tabInfo.isCloseHovered = false;
        }
    }

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        const auto toolbar = activePanel->GetToolbar();
        const auto content = activePanel->GetContent();
        PanelChrome::RoutePanelBodyPointer(
            event,
            toolbar,
            toolbar ? toolbar->GetGeometry() : Rect{},
            content,
            content ? content->GetGeometry() : Rect{},
            &Widget::OnMouseMove);
    }
}

void DockContainer::OnMouseUp(const MouseEvent& event) {
    m_TabDragCandidate = false;
    m_DragTabIndex = -1;

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        const auto toolbar = activePanel->GetToolbar();
        const auto content = activePanel->GetContent();
        PanelChrome::RoutePanelBodyPointer(
            event,
            toolbar,
            toolbar ? toolbar->GetGeometry() : Rect{},
            content,
            content ? content->GetGeometry() : Rect{},
            &Widget::OnMouseUp);
    }
}

void DockContainer::OnMouseWheel(const MouseEvent& event) {
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        const auto toolbar = activePanel->GetToolbar();
        const auto content = activePanel->GetContent();
        PanelChrome::RoutePanelBodyPointer(
            event,
            toolbar,
            toolbar ? toolbar->GetGeometry() : Rect{},
            content,
            content ? content->GetGeometry() : Rect{},
            &Widget::OnMouseWheel);
    }
}

bool DockContainer::ShowsPointerCursor(const Point& position) const {
    if (m_HeaderRect.Contains(position)) {
        for (const auto& tabInfo : m_Tabs) {
            if (tabInfo.tabRect.Contains(position)) {
                return true;
            }
        }
    }

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        const auto& activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(position)) {
                return toolbar->ShowsPointerCursor(position);
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(position)) {
                return content->ShowsPointerCursor(position);
            }
        }
    }

    return false;
}

} // namespace we::editor::docking
