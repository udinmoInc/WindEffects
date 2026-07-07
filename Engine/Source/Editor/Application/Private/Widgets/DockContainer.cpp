#include "Widgets/DockContainer.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"
#include "Core/DockTabIconRegistry.h"
#include "Core/DockTabBrandRegistry.h"
#include <algorithm>
#include <cmath>

namespace we::UI {

namespace {
constexpr float kTabDragThreshold = 6.0f;
}

DockContainer::DockContainer() {}

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
    for (auto& tab : m_Tabs) {
        const float target = tab.isHovered ? 1.0f : 0.0f;
        tab.hoverAnim += (target - tab.hoverAnim) * std::min(1.0f, deltaTime * speed);
    }
}

Size DockContainer::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;

        Size contentAvailable = availableSize;
        contentAvailable.height -= m_HeaderHeight;

        float usedHeight = m_HeaderHeight;

        if (auto toolbar = activePanel->GetToolbar()) {
            Size tbSize = toolbar->Measure(contentAvailable);
            contentAvailable.height -= tbSize.height;
            usedHeight += tbSize.height;
        }

        if (auto content = activePanel->GetContent()) {
            Size cSize = content->Measure(contentAvailable);
            usedHeight += cSize.height;
        }

        m_DesiredSize.height = std::max(m_DesiredSize.height, usedHeight);
    }

    return m_DesiredSize;
}

void DockContainer::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    m_HeaderRect = Rect{
        allottedRect.x,
        allottedRect.y,
        allottedRect.width,
        m_HeaderHeight
    };

    m_ContentRect = Rect{
        allottedRect.x,
        allottedRect.y + m_HeaderHeight,
        allottedRect.width,
        allottedRect.height - m_HeaderHeight
    };

    for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
        auto panel = m_Tabs[static_cast<size_t>(i)].panel;
        if (i == m_ActiveTabIndex) {
            panel->Arrange(m_ContentRect);
        } else {
            panel->Arrange(Rect{0.0f, 0.0f, 0.0f, 0.0f});
        }
    }
}

float DockContainer::MeasureTabWidth(PaintContext& context, const TabInfo& tabInfo, bool isActive) const {
    const auto& theme = Theme::Get();
    const float fontSize = theme.TextSizeTabs;
    const float iconSize = 14.0f;
    const float leftPadding = 10.0f;
    const float rightPadding = 10.0f;
    const float iconTextSpacing = 6.0f;
    const float textCloseSpacing = 6.0f;

    const std::string title = tabInfo.panel->GetTitle();
    const float textWidth = context.GetTextWidth(title, fontSize);

    float leadingWidth = 0.0f;
    if (DockTabBrandRegistry::Get().HasBrand(title)) {
        leadingWidth = DockTabBrandRegistry::Get().GetBrand(title).logicalSize + iconTextSpacing;
    } else {
        const std::string panelIcon = DockTabIconRegistry::Get().GetIcon(title);
        if (!panelIcon.empty()) {
            leadingWidth = iconSize + iconTextSpacing;
        }
    }

    const float closeBtnWidth = (isActive || tabInfo.isHovered) ? iconSize : 0.0f;
    return leftPadding + leadingWidth + textWidth + textCloseSpacing + closeBtnWidth + rightPadding;
}

void DockContainer::PaintTab(PaintContext& context, TabInfo& tabInfo, int index, float& currentX) {
    const auto& theme = Theme::Get();
    const bool isActive = (index == m_ActiveTabIndex);

    Color tabBg = isActive ? Color{0.165f, 0.165f, 0.165f, 1.0f}
                           : Color{0.125f, 0.125f, 0.125f, 1.0f};
    if (!isActive && tabInfo.hoverAnim > 0.001f) {
        tabBg = Color{
            tabBg.r + (theme.HoverOverlay.r - tabBg.r) * tabInfo.hoverAnim * 0.65f,
            tabBg.g + (theme.HoverOverlay.g - tabBg.g) * tabInfo.hoverAnim * 0.65f,
            tabBg.b + (theme.HoverOverlay.b - tabBg.b) * tabInfo.hoverAnim * 0.65f,
            1.0f
        };
    }

    const float tabWidth = MeasureTabWidth(context, tabInfo, isActive);
    Rect tabRect{ currentX, m_HeaderRect.y + 2.0f, tabWidth, m_HeaderHeight - 2.0f };
    tabInfo.tabRect = tabRect;

    context.DrawRoundedRect(tabRect, tabBg, theme.CornerRadiusSmall);

    if (isActive) {
        context.DrawRoundedRectOutline(tabRect, theme.BorderDefault, 1.0f, theme.CornerRadiusSmall);
        context.DrawRect(Rect{ tabRect.x + 1.0f, tabRect.y, tabRect.width - 2.0f, 1.0f }, theme.ActiveTabLine);
    }

    const float fontSize = theme.TextSizeTabs;
    const float iconSize = 14.0f;
    const float leftPadding = 10.0f;
    const float rightPadding = 10.0f;
    const float iconTextSpacing = 6.0f;
    const float textCloseSpacing = 6.0f;

    float itemX = tabRect.x + leftPadding;
    const std::string title = tabInfo.panel->GetTitle();

    if (DockTabBrandRegistry::Get().HasBrand(title)) {
        const DockTabBrand brand = DockTabBrandRegistry::Get().GetBrand(title);
        const float logoY = tabRect.y + (tabRect.height - brand.logicalSize) * 0.5f;
        const auto snap = [](float v) { return std::floor(v + 0.5f); };
        Rect logoRect{ snap(itemX), snap(logoY), brand.logicalSize, brand.logicalSize };
        if (brand.texture != VK_NULL_HANDLE) {
            context.DrawTexture(logoRect, brand.texture, theme.TextPrimary);
        }
        itemX += brand.logicalSize + iconTextSpacing;
    } else {
        const std::string panelIcon = DockTabIconRegistry::Get().GetIcon(title);
        if (!panelIcon.empty()) {
            const float pIconY = tabRect.y + (tabRect.height - iconSize) * 0.5f;
            const Color iconColor = isActive ? theme.TextPrimary : theme.TextSecondary;
            IconPainter::DrawIcon(context, panelIcon, Rect{ itemX, pIconY, iconSize, iconSize }, iconColor);
            itemX += iconSize + iconTextSpacing;
        }
    }

    const Color textColor = isActive ? theme.TextPrimary : theme.TextSecondary;
    const float titleY = tabRect.y + (tabRect.height - fontSize) * 0.5f;
    context.DrawText(title, Point{ itemX, titleY }, textColor, fontSize, isActive);

    if (isActive || tabInfo.isHovered) {
        const float closeX = tabRect.x + tabRect.width - rightPadding - iconSize;
        const float closeY = tabRect.y + (tabRect.height - iconSize) * 0.5f;
        tabInfo.closeRect = Rect{ closeX, closeY, iconSize, iconSize };

        const Color closeColor = tabInfo.isCloseHovered ? theme.TextPrimary : theme.TextSecondary;
        IconPainter::DrawIcon(context, Icons::XName, tabInfo.closeRect, closeColor);
    } else {
        tabInfo.closeRect = {};
    }

    currentX += tabWidth + 2.0f;
}

void DockContainer::Paint(PaintContext& context) {
    const auto& theme = Theme::Get();

    context.DrawRect(m_Geometry, theme.PanelBackground);
    context.DrawRect(m_HeaderRect, theme.HeaderBackground);
    context.DrawRect(
        Rect{ m_HeaderRect.x, m_HeaderRect.y + m_HeaderRect.height - 1.0f, m_HeaderRect.width, 1.0f },
        theme.Separator);

    float currentX = m_HeaderRect.x + 4.0f;
    for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
        PaintTab(context, m_Tabs[static_cast<size_t>(i)], i, currentX);
    }

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            toolbar->Paint(context);

            Rect tbRect = toolbar->GetGeometry();
            context.DrawRect(
                Rect{ tbRect.x, tbRect.y + tbRect.height - 1.0f, tbRect.width, 1.0f },
                theme.Separator);
        }
        if (auto content = activePanel->GetContent()) {
            content->Paint(context);
        }
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
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(event.position)) {
                toolbar->OnMouseDown(event);
                return;
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(event.position)) {
                content->OnMouseDown(event);
            }
        }
    }
}

void DockContainer::OnMouseMove(const MouseEvent& event) {
    if (m_TabDragCandidate && m_DragTabIndex >= 0) {
        const float dx = event.position.x - m_DragStart.x;
        const float dy = event.position.y - m_DragStart.y;
        if (std::sqrt(dx * dx + dy * dy) >= kTabDragThreshold) {
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
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(event.position)) {
                toolbar->OnMouseMove(event);
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(event.position)) {
                content->OnMouseMove(event);
            }
        }
    }
}

void DockContainer::OnMouseUp(const MouseEvent& event) {
    m_TabDragCandidate = false;
    m_DragTabIndex = -1;

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(event.position)) {
                toolbar->OnMouseUp(event);
                return;
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(event.position)) {
                content->OnMouseUp(event);
            }
        }
    }
}

void DockContainer::OnMouseWheel(const MouseEvent& event) {
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(event.position)) {
                toolbar->OnMouseWheel(event);
                return;
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(event.position)) {
                content->OnMouseWheel(event);
            }
        }
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

} // namespace we::UI
