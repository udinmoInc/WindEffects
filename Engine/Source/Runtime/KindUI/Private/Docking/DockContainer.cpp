#include "KindUI/Docking/DockContainer.h"
#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "KindUI/Core/WindIcon.h"
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

using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::AssertLayoutRectValid;

namespace we::runtime::kindui::docking {
namespace PanelChrome = ::we::runtime::kindui::panels::PanelChrome;

DockContainer::DockContainer() {
    m_HeaderHeightLogical = ThemeMetric(MetricToken::PanelTabHeight);
}

float DockContainer::GetHeaderHeightDevice() const {
    return DPIContext::Snap(m_HeaderHeightLogical * DPIContext::GetScale());
}

void DockContainer::SetTrailingReservedWidth(float width) {
    m_TrailingReservedWidth = (std::max)(0.0f, width);
}

void DockContainer::SetLeadingReservedWidth(float width) {
    m_LeadingReservedWidth = (std::max)(0.0f, width);
}

void DockContainer::SetShowOptionsMenu(bool show) {
    m_ShowOptionsMenu = show;
    if (!show) {
        m_OptionsMenuRect = {};
        m_OptionsMenuHovered = false;
    }
}

std::shared_ptr<Panel> DockContainer::GetActivePanel() const {
    if (m_ActiveTabIndex < 0 || m_ActiveTabIndex >= static_cast<int>(m_Tabs.size())) {
        return nullptr;
    }
    return m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
}

std::vector<std::shared_ptr<Panel>> DockContainer::GetPanels() const {
    std::vector<std::shared_ptr<Panel>> panels;
    panels.reserve(m_Tabs.size());
    for (const auto& tab : m_Tabs) {
        panels.push_back(tab.panel);
    }
    return panels;
}

bool DockContainer::IsTabStripInteractiveHit(const Point& pos) const {
    if (!m_HeaderRect.Contains(pos)) {
        return false;
    }
    if (m_ShowOptionsMenu && m_OptionsMenuRect.Contains(pos)) {
        return true;
    }
    for (const auto& tab : m_Tabs) {
        if (tab.tabRect.Contains(pos)) {
            return true;
        }
    }
    return false;
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
    const float speed = 18.0f;
    for (auto& tab : m_Tabs) {
        const float target = tab.isHovered ? 1.0f : 0.0f;
        tab.hoverAnim = we::runtime::kindui::Animator::Damp(tab.hoverAnim, target, speed, deltaTime);
    }
}

Size DockContainer::Measure(const Size& availableSize) {

    const float headerHeight = GetHeaderHeightDevice();
    const float headerContentGap = PanelChrome::DockHeaderContentGap();
    const Size measuredAvail = PanelChrome::InsetDockMeasureAvailable(availableSize);

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

    float desiredW = (measuredAvail.width < 1.0e8f) ? measuredAvail.width : requiredTabWidth;
    float desiredH = (measuredAvail.height < 1.0e8f) ? measuredAvail.height : headerHeight;

    Size panelContentDesired{ 0.0f, 0.0f };
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;

        Size contentAvailable = measuredAvail;
        if (contentAvailable.height < 1.0e8f) {
            contentAvailable.height = std::max(0.0f, contentAvailable.height - headerHeight - headerContentGap);
        }

        panelContentDesired = activePanel->Measure(contentAvailable);
    }

    if (availableSize.width >= 1.0e8f) {
        desiredW = std::max({ desiredW, requiredTabWidth, panelContentDesired.width });
    } else {
        desiredW = std::max(desiredW, panelContentDesired.width);
    }
    if (availableSize.height >= 1.0e8f) {
        desiredH = headerHeight + headerContentGap + panelContentDesired.height;
    } else {
        desiredH = std::max(desiredH, headerHeight + headerContentGap + panelContentDesired.height);
    }

    m_DesiredSize = ClampDesiredSize(
        PanelChrome::ExpandDockMeasuredSize(Size{ desiredW, desiredH }, availableSize));
    return m_DesiredSize;
}

void DockContainer::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const auto layout = PanelChrome::LayoutDockPanel(allottedRect, GetHeaderHeightDevice());
    m_HeaderRect = layout.headerRect;
    m_HeaderContentGapRect = layout.headerContentGapRect;
    m_ContentRect = layout.contentRect;

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

    if (we::runtime::kindui::UiGeometryDebug::IsEnabled()) {
        we::runtime::kindui::UiGeometryDebug::Get().TraceRegion(
            "DockTabStrip",
            m_HeaderRect,
            "DockContainer",
            we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::TabPaddingH),
            we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::TabPaddingV),
            we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::TextSizeTabs));
    }
}

void DockContainer::LayoutTabGeometries() {
    PaintContext context;
    std::vector<PanelChrome::DockTabDescriptor> descriptors;
    descriptors.reserve(m_Tabs.size());
    for (const auto& tabInfo : m_Tabs) {
        PanelChrome::DockTabDescriptor descriptor{};
        descriptor.title = tabInfo.panel->GetTitle();
        descriptor.icon = tabInfo.panel->GetTabIcon();
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

    Rect tabStripRect = m_HeaderRect;
    tabStripRect.x += m_LeadingReservedWidth;
    tabStripRect.width = (std::max)(
        0.0f,
        tabStripRect.width - m_LeadingReservedWidth - m_TrailingReservedWidth);

    const auto layout = PanelChrome::LayoutDockTabStrip(context, tabStripRect, descriptors, state);
    for (size_t i = 0; i < m_Tabs.size(); ++i) {
        m_Tabs[i].tabRect = layout.tabs[i].tabRect;
        m_Tabs[i].closeRect = layout.tabs[i].closeRect;
    }

    const float buttonSize = PanelChrome::HeaderButtonSize();
    const float rightPad = (std::max)(PanelChrome::TabStripPadH(), PanelChrome::TabPadH());
    if (m_ShowOptionsMenu) {
        const float optionsRight = m_HeaderRect.x + m_HeaderRect.width - m_TrailingReservedWidth - rightPad;
        m_OptionsMenuRect = Rect{
            optionsRight - buttonSize,
            std::floor(m_HeaderRect.y + (m_HeaderRect.height - buttonSize) * 0.5f),
            buttonSize,
            buttonSize
        };
    } else {
        m_OptionsMenuRect = {};
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
    descriptor.icon = tabInfo.panel->GetTabIcon();
    descriptor.hasBrand = tabInfo.panel->HasTabBrand();
    descriptor.brandDescriptor = tabInfo.panel->GetTabBrandDescriptor();
    descriptor.brandLogicalSize = tabInfo.panel->GetTabBrandLogicalSize();
    const bool showClose = isActive || tabInfo.isHovered;
    return PanelChrome::MeasureDockTabWidth(context, descriptor, isActive, showClose, flushLeft);
}

void DockContainer::PaintTab(PaintContext& context, TabInfo& tabInfo, int index, float& currentX) {
    (void)context;
    (void)tabInfo;
    (void)index;
    (void)currentX;
}

void DockContainer::Paint(PaintContext& context) {
    if (!m_Visible || m_ActiveTabIndex < 0 || m_ActiveTabIndex >= static_cast<int>(m_Tabs.size())) {
        return;
    }

    std::vector<PanelChrome::DockTabDescriptor> descriptors;
    descriptors.reserve(m_Tabs.size());
    PanelChrome::DockTabStripLayout stripLayout{};
    stripLayout.tabs.reserve(m_Tabs.size());
    for (const auto& tabInfo : m_Tabs) {
        PanelChrome::DockTabDescriptor descriptor{};
        descriptor.title = tabInfo.panel->GetTitle();
        descriptor.icon = tabInfo.panel->GetTabIcon();
        descriptor.hasBrand = tabInfo.panel->HasTabBrand();
        descriptor.brandDescriptor = tabInfo.panel->GetTabBrandDescriptor();
        descriptor.brandLogicalSize = tabInfo.panel->GetTabBrandLogicalSize();
        descriptors.push_back(descriptor);
        stripLayout.tabs.push_back({ tabInfo.tabRect, tabInfo.closeRect });
    }

    PanelChrome::DockTabStripState state{};
    state.activeIndex = static_cast<size_t>(m_ActiveTabIndex);
    state.optionsMenuHovered = m_OptionsMenuHovered;
    state.showOptionsMenu = m_ShowOptionsMenu;
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

    auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
    // Paint strip only over the tab region (exclude leading logo / trailing window controls).
    Rect paintHeader = m_HeaderRect;
    paintHeader.x += m_LeadingReservedWidth;
    paintHeader.width = (std::max)(
        0.0f,
        paintHeader.width - m_LeadingReservedWidth - m_TrailingReservedWidth);
    PanelChrome::PaintDockPanelChrome(
        context,
        paintHeader,
        m_HeaderContentGapRect,
        m_ContentRect,
        descriptors,
        stripLayout,
        state,
        [&](PaintContext& paintContext) {
            activePanel->Paint(paintContext);
        },
        // Floating window frames own the ambient shadow for the whole section.
        m_LeadingReservedWidth <= 0.5f && m_TrailingReservedWidth <= 0.5f);
}

void DockContainer::OnMouseDown(const MouseEvent& event) {
    if (m_HeaderRect.Contains(event.position)) {
        if (m_ShowOptionsMenu && m_OptionsMenuRect.Contains(event.position)) {
            ShowPanelOptionsMenu(event.position);
            return;
        }
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

void DockContainer::OnHoverLost() {
    m_OptionsMenuHovered = false;
    for (auto& tabInfo : m_Tabs) {
        tabInfo.isHovered = false;
        tabInfo.isCloseHovered = false;
    }
}

void DockContainer::OnMouseMove(const MouseEvent& event) {
    if (m_TabDragCandidate && m_DragTabIndex >= 0) {
        const float dx = event.position.x - m_DragStart.x;
        const float dy = event.position.y - m_DragStart.y;
        if (we::runtime::kindui::UiMetrics::ExceedsDragThreshold(dx, dy)) {
            // Only undock when the cursor leaves the dock — avoids accidental floats
            // from small tab reordering / click jitter inside the panel.
            if (!m_Geometry.Contains(event.position)) {
                m_TabDragCandidate = false;
                const int dragIndex = m_DragTabIndex;
                m_DragTabIndex = -1;
                if (m_OnTabDragStarted && dragIndex >= 0 && dragIndex < static_cast<int>(m_Tabs.size())) {
                    const auto panel = m_Tabs[static_cast<size_t>(dragIndex)].panel;
                    m_OnTabDragStarted(panel, event.position);
                }
                return;
            }
        }
    }

    if (m_HeaderRect.Contains(event.position)) {
        m_OptionsMenuHovered = m_OptionsMenuRect.Contains(event.position);
        for (auto& tabInfo : m_Tabs) {
            tabInfo.isHovered = tabInfo.tabRect.Contains(event.position);
            tabInfo.isCloseHovered = tabInfo.isHovered && tabInfo.closeRect.Contains(event.position);
        }
    } else {
        m_OptionsMenuHovered = false;
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
        if (m_ShowOptionsMenu && m_OptionsMenuRect.Contains(position)) {
            return true;
        }
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

void DockContainer::ShowPanelOptionsMenu(const Point& pos) {
    (void)pos;
    if (m_ActiveTabIndex < 0 || m_ActiveTabIndex >= static_cast<int>(m_Tabs.size())) {
        return;
    }
    const auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
    if (m_OnOptionsMenuRequested) {
        m_OnOptionsMenuRequested(activePanel, Point{ m_OptionsMenuRect.x, m_OptionsMenuRect.y + m_OptionsMenuRect.height + 2.0f });
    }
}

} // namespace we::runtime::kindui::docking

 
