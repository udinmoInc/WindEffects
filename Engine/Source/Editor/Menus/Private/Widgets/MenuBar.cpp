// ==============================================================================
// WindEffects — Menus — MenuBar
// UI widget used by the Menus module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Widgets/MenuBar.h"
#include "Widgets/DropdownMenu.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "Text/Layout/TextStyle.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::TextMetrics;

namespace we::editor::menus {
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

MenuBar::MenuBar()
    : m_Style(WidgetStyle::Panel())
    , m_Height(we::runtime::kindui::ResolveMetric(MetricToken::TitleBarHeight))
    , m_ItemSpacing(0.0f)
    , m_ItemPaddingH(10.0f)
{}

MenuBar::~MenuBar() = default;

void MenuBar::AddMenu(const std::string& label, const std::vector<std::shared_ptr<MenuItem>>& items) {
    MenuInfo menu;
    menu.label = label;
    for (const auto& item : items) {
        if (item) {
            menu.items.push_back(item);
        }
    }
    m_Menus.push_back(std::move(menu));
    CalculateMenuGeometries();
}

void MenuBar::SetItemSpacing(float spacing) {
    m_ItemSpacing = spacing;
    CalculateMenuGeometries();
}

Size MenuBar::Measure(const Size& availableSize) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float textSize = ThemeMetric(MetricToken::TextSizeMenu) * uiScale;

    float totalWidth = 0.0f;
    for (size_t i = 0; i < m_Menus.size(); ++i) {
        float textWidth = TextMetrics::MeasureWidth(m_Menus[i].label, textSize);
        totalWidth += textWidth + (m_ItemPaddingH * uiScale) * 2.0f;
        if (i + 1 < m_Menus.size()) {
            totalWidth += m_ItemSpacing * uiScale;
        }
    }
    m_DesiredSize = Size{ totalWidth, m_Height };
    return m_DesiredSize;
}

void MenuBar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateMenuGeometries();
}

void MenuBar::Paint(PaintContext& context) {
    if (!m_Visible) return;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float textSize = ThemeMetric(MetricToken::TextSizeMenu) * uiScale;

    auto drawMenu = [&](const MenuInfo& menu, int index) {
        bool isActive = m_MenuOpen && index == m_HoveredMenu;
        const bool isHighlighted = (index == m_HoveredMenu) || isActive;
        const float radius = ThemeMetric(MetricToken::CornerRadiusSmall) * uiScale;
        const float insetV = ThemeMetric(MetricToken::Space1) * uiScale;
        const float insetH = ThemeMetric(MetricToken::Space1) * 0.5f * uiScale;

        if (isHighlighted) {
            Rect highlightRect{
                menu.geometry.x + insetH,
                menu.geometry.y + insetV,
                std::max(0.0f, menu.geometry.width - insetH * 2.0f),
                std::max(0.0f, menu.geometry.height - insetV * 2.0f)
            };
            const Color base = ThemeColor(ColorToken::WindowBackground);
            const Color target = isActive
                ? ThemeColor(ColorToken::SelectInactiveBackground)
                : ThemeColor(ColorToken::HoverBackground);
            const float strength = isActive ? 0.5f : 0.62f;
            context.DrawRoundedRect(
                highlightRect,
                Color::Pick(base, target, strength),
                radius * 0.5f);
        }

        const float centerY = std::floor(menu.geometry.y + menu.geometry.height * 0.5f);
        float textX = menu.geometry.x + m_ItemPaddingH * uiScale;
        float textY = std::floor(::we::runtime::kindui::LayoutMetrics::AlignTextTopAtCenterY(centerY, textSize));

        Color textColor = isHighlighted
            ? ThemeColor(ColorToken::TextPrimary)
            : ThemeColor(ColorToken::TextSecondary);
        context.DrawText(
            menu.label,
            Point{ textX, textY },
            textColor,
            textSize,
            we::runtime::text::layout::FontWeight::Regular);
    };

    for (size_t i = 0; i < m_VisibleMenus.size(); ++i) {
        drawMenu(m_VisibleMenus[i], (int)i);
    }

    if (m_ShowsMore) {
        drawMenu(m_MoreMenu, (int)m_VisibleMenus.size());
    }
}

void MenuBar::OnMouseDown(const MouseEvent& event) {
    MenuInfo* menu = GetMenuAtPosition(event.position);
    if (menu) {
        int idx = -1;
        for (size_t i = 0; i < m_VisibleMenus.size(); ++i) {
            if (&m_VisibleMenus[i] == menu) { idx = static_cast<int>(i); break; }
        }
        if (m_ShowsMore && &m_MoreMenu == menu) {
            idx = static_cast<int>(m_VisibleMenus.size());
        }

        if (auto* overlay = GetPopupHost()) {
            bool wasOpen = m_MenuOpen && (m_HoveredMenu == idx);
            overlay->CloseAllPopups();

            if (wasOpen) {
                m_MenuOpen = false;
            } else {
                m_MenuOpen = true;
                m_HoveredMenu = idx;
                std::vector<std::shared_ptr<MenuItem>> itemsToShow = menu->items;

                if (itemsToShow.empty()) {
                    auto emptyItem = std::make_shared<MenuItem>();
                    emptyItem->label = "(Empty)";
                    emptyItem->enabled = false;
                    itemsToShow.push_back(emptyItem);
                }

                auto dropdown = std::make_shared<DropdownMenu>(itemsToShow);
                overlay->ShowPopup(dropdown, Point{menu->geometry.x, menu->geometry.y + menu->geometry.height});
            }
        }
    }
}

void MenuBar::OnHoverLost() {
    if (m_HoveredMenu >= 0) {
        m_HoveredMenu = -1;
        InvalidatePaint();
    }
}

void MenuBar::OnMouseMove(const MouseEvent& event) {
    auto* overlay = GetPopupHost();
    if (m_MenuOpen && overlay && !overlay->HasOpenPopups()) {
        m_MenuOpen = false;
    }

    MenuInfo* menu = GetMenuAtPosition(event.position);
    int newHovered = -1;
    if (menu) {
        for (size_t i = 0; i < m_VisibleMenus.size(); ++i) {
            if (&m_VisibleMenus[i] == menu) { newHovered = static_cast<int>(i); break; }
        }
        if (newHovered < 0 && m_ShowsMore && &m_MoreMenu == menu) {
            newHovered = static_cast<int>(m_VisibleMenus.size());
        }
    }

    if (newHovered != m_HoveredMenu) {
        m_HoveredMenu = newHovered;
        InvalidatePaint();
    }

    if (menu && newHovered >= 0) {
        if (m_MenuOpen && overlay && m_HoveredMenu != newHovered) {
            overlay->CloseAllPopups();
            std::vector<std::shared_ptr<MenuItem>> itemsToShow = menu->items;
            if (itemsToShow.empty()) {
                auto emptyItem = std::make_shared<MenuItem>();
                emptyItem->label = "(Empty)";
                emptyItem->enabled = false;
                itemsToShow.push_back(emptyItem);
            }
            auto dropdown = std::make_shared<DropdownMenu>(itemsToShow);
            overlay->ShowPopup(dropdown, Point{menu->geometry.x, menu->geometry.y + menu->geometry.height});
        }
    }
}

void MenuBar::RemoveMenu(const std::string& label) {
    m_Menus.erase(
        std::remove_if(m_Menus.begin(), m_Menus.end(),
            [&label](const MenuInfo& m) { return m.label == label; }),
        m_Menus.end()
    );
    CalculateMenuGeometries();
}

void MenuBar::Clear() {
    m_Menus.clear();
    CalculateMenuGeometries();
}

void MenuBar::CalculateMenuGeometries() {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    float x = m_Geometry.x;
    float availableWidth = m_Geometry.width;
    const float textSize = ThemeMetric(MetricToken::TextSizeMenu) * uiScale;

    m_VisibleMenus.clear();
    m_HiddenMenus.clear();
    m_ShowsMore = false;

    float moreTextW = TextMetrics::MeasureWidth("More", textSize);
    float moreWidth = moreTextW + (m_ItemPaddingH * uiScale) * 2.0f;

    for (size_t i = 0; i < m_Menus.size(); ++i) {
        auto& menu = m_Menus[i];
        float textWidth = TextMetrics::MeasureWidth(menu.label, textSize);
        float itemWidth = textWidth + (m_ItemPaddingH * uiScale) * 2.0f;

        bool isLast = (i == m_Menus.size() - 1);
        float widthNeeded = isLast ? itemWidth : (itemWidth + m_ItemSpacing * uiScale + moreWidth);

        if (availableWidth > 0.0f && i >= 4 && (x - m_Geometry.x) + widthNeeded > availableWidth && !m_Menus.empty()) {
            m_ShowsMore = true;
            m_HiddenMenus.push_back(menu);
        } else {
            menu.geometry = Rect{ x, m_Geometry.y, itemWidth, m_Geometry.height };
            m_VisibleMenus.push_back(menu);
            x += itemWidth;
            if (!isLast) {
                x += m_ItemSpacing * uiScale;
            }
        }
    }

    if (m_ShowsMore) {
        m_MoreMenu.label = "More";
        m_MoreMenu.geometry = Rect{ x, m_Geometry.y, moreWidth, m_Geometry.height };
        m_MoreMenu.items.clear();
        for (const auto& hidden : m_HiddenMenus) {
            for (const auto& item : hidden.items) {
                if (!item) continue;
                auto prefixedItem = std::make_shared<MenuItem>(*item);
                prefixedItem->label = hidden.label + " > " + item->label;
                m_MoreMenu.items.push_back(prefixedItem);
            }
        }
    }
}

MenuBar::MenuInfo* MenuBar::GetMenuAtPosition(const Point& pos) {
    for (auto& menu : m_VisibleMenus) {
        if (menu.geometry.Contains(pos)) {
            return &menu;
        }
    }
    if (m_ShowsMore && m_MoreMenu.geometry.Contains(pos)) {
        return &m_MoreMenu;
    }
    return nullptr;
}

} // namespace we::editor::menus
