// ==============================================================================
// WindEffects — KindUI — DropdownMenu
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/DropdownMenu.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Theme/DesignToken.h"
#include "KindUI/Theme/StyleRole.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/UI/OverlayManager.h"
#include <algorithm>

#include "KindUI/Theme/TypographySystem.h"

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::TextMetrics;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::TypographySystem;
using ::we::runtime::kindui::TypographyToken;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

namespace we::runtime::kindui {
namespace {

class TooltipBubble final : public Widget {
public:
    explicit TooltipBubble(std::string text) : m_Text(std::move(text)) {}
    Size Measure(const Size& availableSize) override {
        (void)availableSize;
        const float padX = 8.0f;
        const float padY = 5.0f;
        const float textSize = TypographySystem::GetFontSize(TypographyToken::Caption);
        const float textW = TextMetrics::MeasureWidth(m_Text, textSize);
        m_DesiredSize = Size{ textW + padX * 2.0f, textSize + padY * 2.0f };
        return m_DesiredSize;
    }
    void Arrange(const Rect& allottedRect) override {
        CommitGeometry(allottedRect);
        ClearLayoutDirty();
    }
    void Paint(PaintContext& context) override {
        ClearPaintDirty();
        ControlChrome::PaintTooltipSurface(context, m_Geometry);
        const float textSize = TypographySystem::GetFontSize(TypographyToken::Caption);
        context.DrawText(
            m_Text,
            Point{ m_Geometry.x + 8.0f, TypographySystem::AlignTextTopY(m_Geometry, textSize) },
            ResolveColor(ColorToken::TextPrimary),
            textSize);
    }
    [[nodiscard]] bool IsPointerTransparent() const override { return true; }
private:
    std::string m_Text;
};

} // namespace

DropdownMenu::DropdownMenu(const std::vector<std::shared_ptr<MenuItem>>& items)
    : m_Items(items)
{
    m_ItemHeight = 30.0f;
    m_PaddingY = 6.0f;
    m_PaddingX = 10.0f;
}

Size DropdownMenu::Measure(const Size& availableSize) {
    const float textSize = TypographySystem::GetFontSize(TypographyToken::Menu);
    float maxContentW = 200.0f;

    const float checkSize = 16.0f;
    const float iconSize = 16.0f;
    const float iconGap = ThemeMetric(MetricToken::Space2);
    const float rightSlotGap = ThemeMetric(MetricToken::Space3);

    for (const auto& item : m_Items) {
        if (!item) continue;
        if (item->label.empty()) continue;

        float itemW = m_PaddingX * 2.0f;
        if (item->icon.IsValid()) {
            itemW += iconSize + iconGap;
        }

        itemW += TextMetrics::MeasureWidth(item->label, textSize);

        float rightSlotW = 0.0f;
        if (item->checked) {
            rightSlotW = checkSize;
        } else if (!item->submenu.empty()) {
            rightSlotW = iconSize;
        } else if (!item->shortcut.empty()) {
            rightSlotW = TextMetrics::MeasureWidth(item->shortcut, textSize);
        }

        if (rightSlotW > 0.0f) {
            itemW += rightSlotGap + rightSlotW;
        }

        maxContentW = std::max(maxContentW, itemW);
    }

    const float calcW = std::clamp(maxContentW, 200.0f, 320.0f);
    const float fullH = m_PaddingY * 2.0f + static_cast<float>(m_Items.size()) * m_ItemHeight;
    const float maxAllowedH = availableSize.height > 0.0f
        ? std::min(availableSize.height, ThemeMetric(MetricToken::PopupMaxHeight))
        : ThemeMetric(MetricToken::PopupMaxHeight);
    const float calcH = std::min(fullH, maxAllowedH);

    m_DesiredSize = Size{ calcW, calcH };
    return m_DesiredSize;
}

void DropdownMenu::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

int DropdownMenu::HitItemAt(const Point& pos) const {
    if (!m_Geometry.Contains(pos)) {
        return -1;
    }
    float y = m_Geometry.y + m_PaddingY - m_ScrollOffset;
    for (size_t i = 0; i < m_Items.size(); ++i) {
        const Rect itemRect{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, m_ItemHeight };
        if (itemRect.Contains(pos) && y >= m_Geometry.y && y + m_ItemHeight <= m_Geometry.y + m_Geometry.height) {
            return static_cast<int>(i);
        }
        y += m_ItemHeight;
    }
    return -1;
}

void DropdownMenu::Paint(PaintContext& context) {
    ControlChrome::PaintPopupSurface(context, m_Geometry);

    context.PushClipRect(m_Geometry);

    const float textSize = TypographySystem::GetFontSize(TypographyToken::Menu);
    const float checkSize = 16.0f;
    const float iconSize = 16.0f;
    const float iconGap = ThemeMetric(MetricToken::Space2);
    float y = m_Geometry.y + m_PaddingY - m_ScrollOffset;

    for (size_t i = 0; i < m_Items.size(); ++i) {
        const auto& item = m_Items[i];
        if (!item) continue;

        const Rect itemRect{ m_Geometry.x + m_PaddingX, y, m_Geometry.width - m_PaddingX * 2.0f, m_ItemHeight };

        if (y + m_ItemHeight >= m_Geometry.y && y <= m_Geometry.y + m_Geometry.height) {
            if (item->label.empty()) {
                const float sepY = y + m_ItemHeight * 0.5f;
                context.DrawLine(
                    Point{ m_Geometry.x + m_PaddingX, sepY },
                    Point{ m_Geometry.x + m_Geometry.width - m_PaddingX, sepY },
                    ResolveColor(ColorToken::Separator),
                    1.0f);
            } else {
                const bool isHovered = (m_HoveredItem == static_cast<int>(i)) || (m_ActiveSubmenuIndex == static_cast<int>(i));

                if (isHovered && item->enabled) {
                    ControlChrome::InteractionState state{};
                    state.hoverAnim = 1.0f;
                    ControlChrome::PaintListRow(
                        context, itemRect, state, ColorToken::PopupBackground);
                }

                float textX = itemRect.x;
                if (item->icon.IsValid()) {
                    const float iconY = itemRect.y + (m_ItemHeight - iconSize) * 0.5f;
                    IconPainter::Draw(context, item->icon, Rect{ textX, iconY, iconSize, iconSize });
                    textX += iconSize + iconGap;
                }

                const Color textColor = item->enabled
                    ? ResolveColor(ColorToken::TextPrimary)
                    : ResolveColor(ColorToken::TextDisabled);
                const float centerY = itemRect.y + itemRect.height * 0.5f;
                const float textY = TypographySystem::AlignTextTopAtCenterY(centerY, textSize);

                context.DrawText(item->label, Point{ textX, textY }, textColor, textSize);

                const float rightX = itemRect.x + itemRect.width - checkSize;
                const float rightY = itemRect.y + (m_ItemHeight - checkSize) * 0.5f;

                if (item->checked || item->isCheckable) {
                    ControlChrome::InteractionState state{};
                    state.disabled = !item->enabled;
                    state.hoverAnim = 0.0f;
                    ControlChrome::PaintCheckbox(context, Rect{ rightX, rightY, checkSize, checkSize }, item->checked, state);
                } else if (!item->submenu.empty()) {
                    IconPainter::Draw(context, WindIcons::ChevronRight16, Rect{ rightX, rightY, iconSize, iconSize }, textColor);
                } else if (!item->shortcut.empty()) {
                    const float shortcutW = TextMetrics::MeasureWidth(item->shortcut, textSize);
                    const float shortcutX = itemRect.x + itemRect.width - shortcutW;
                    const Color shortcutColor = item->enabled
                        ? ResolveColor(ColorToken::TextSecondary)
                        : ResolveColor(ColorToken::TextDisabled);
                    context.DrawText(item->shortcut, Point{ shortcutX, textY }, shortcutColor, textSize);
                }
            }
        }
        y += m_ItemHeight;
    }

    context.PopClipRect();
}

void DropdownMenu::OpenSubmenu(size_t index) {
    if (index >= m_Items.size() || !m_Items[index] || m_Items[index]->submenu.empty()) {
        return;
    }
    if (m_ActiveSubmenuIndex == static_cast<int>(index) && m_ActiveSubmenu) {
        return;
    }
    CloseActiveSubmenu();

    m_ActiveSubmenuIndex = static_cast<int>(index);
    m_ActiveSubmenu = std::make_shared<DropdownMenu>(m_Items[index]->submenu);

    auto* overlay = GetPopupHost();
    if (overlay) {
        const float itemY = m_Geometry.y + m_PaddingY + static_cast<float>(index) * m_ItemHeight - m_ScrollOffset;
        const Rect anchorRect{
            m_Geometry.x + m_Geometry.width - 2.0f,
            itemY - 2.0f,
            1.0f,
            m_ItemHeight
        };
        overlay->ShowAnchoredPopup(m_ActiveSubmenu, anchorRect, PopupPlacementMode::SidePreferred);
    }
}

void DropdownMenu::CloseActiveSubmenu() {
    if (m_ActiveSubmenu) {
        auto submenuToClose = m_ActiveSubmenu;
        m_ActiveSubmenu = nullptr;
        m_ActiveSubmenuIndex = -1;
        submenuToClose->CloseActiveSubmenu();
        if (auto* overlay = dynamic_cast<OverlayHost*>(GetPopupHost())) {
            overlay->ClosePopup(submenuToClose);
        } else if (auto* popupHost = GetPopupHost()) {
            popupHost->CloseTopPopup();
        }
        InvalidatePaint();
    }
}

void DropdownMenu::OnMouseMove(const MouseEvent& event) {
    const int hovered = HitItemAt(event.position);
    if (hovered != m_HoveredItem) {
        m_HoveredItem = hovered;
        InvalidatePaint();

        if (m_HoveredItem >= 0 && m_HoveredItem < static_cast<int>(m_Items.size())) {
            const auto& item = m_Items[static_cast<size_t>(m_HoveredItem)];
            if (item && item->enabled && !item->submenu.empty()) {
                OpenSubmenu(static_cast<size_t>(m_HoveredItem));
            } else if (m_ActiveSubmenuIndex != m_HoveredItem) {
                CloseActiveSubmenu();
            }
        }
    }

    auto* overlay = dynamic_cast<OverlayHost*>(GetPopupHost());
    if (!overlay) {
        return;
    }
    if (m_HoveredItem < 0 || m_HoveredItem >= static_cast<int>(m_Items.size())) {
        overlay->CloseTooltips();
        return;
    }
    const auto& hoveredItem = m_Items[static_cast<size_t>(m_HoveredItem)];
    if (!hoveredItem || hoveredItem->tooltip.empty()) {
        overlay->CloseTooltips();
        return;
    }
    const float hoveredY = m_Geometry.y + m_PaddingY + static_cast<float>(m_HoveredItem) * m_ItemHeight
        - m_ScrollOffset;
    const Rect anchor{
        m_Geometry.x + m_Geometry.width,
        hoveredY,
        1.0f,
        m_ItemHeight
    };
    overlay->ShowTooltip(std::make_shared<TooltipBubble>(hoveredItem->tooltip), anchor);
}

void DropdownMenu::OnHoverLost() {
    m_HoveredItem = -1;
    InvalidatePaint();
    if (auto* overlay = dynamic_cast<OverlayHost*>(GetPopupHost())) {
        overlay->CloseTooltips();
    }
}

void DropdownMenu::OnMouseWheel(const MouseEvent& event) {
    const float fullH = m_PaddingY * 2.0f + static_cast<float>(m_Items.size()) * m_ItemHeight;
    const float maxScroll = std::max(0.0f, fullH - m_Geometry.height);
    m_ScrollOffset = std::clamp(m_ScrollOffset - event.wheelDeltaY * ThemeMetric(MetricToken::ListRowHeight), 0.0f,
        maxScroll);
    InvalidatePaint();
}

void DropdownMenu::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        const int clickedItem = HitItemAt(event.position);
        if (clickedItem >= 0 && clickedItem < static_cast<int>(m_Items.size())) {
            const auto& item = m_Items[static_cast<size_t>(clickedItem)];
            if (item && item->enabled) {
                if (!item->submenu.empty()) {
                    OpenSubmenu(static_cast<size_t>(clickedItem));
                    return;
                }
                if (item->isCheckable || item->checked) {
                    item->checked = !item->checked;
                    InvalidatePaint();
                }
                std::function<void()> callback = item->onClick;
                if (auto* overlay = GetPopupHost()) {
                    overlay->CloseAllPopups();
                }
                if (callback) {
                    callback();
                }
            }
        }
    }
}

} // namespace we::runtime::kindui

