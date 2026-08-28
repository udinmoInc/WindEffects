#include "Widgets/DropdownMenu.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Layout/OverlayManager.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::TextMetrics;
using ::we::runtime::kindui::IconPainter;
namespace Icons = ::we::runtime::kindui::Icons;

namespace we::editor::menus {
using ::we::runtime::kindui::MouseButton;

DropdownMenu::DropdownMenu(const std::vector<std::shared_ptr<MenuItem>>& items)
    : m_Items(items)
{
}

Size DropdownMenu::Measure(const Size& availableSize) {
    const float textSize = ThemeMetric(MetricToken::TextSizeSmall);
    float maxContentW = 140.0f; // min dropdown width

    for (const auto& item : m_Items) {
        if (!item) continue;
        float itemW = m_PaddingX * 2.0f;
        if (!item->label.empty()) {
            itemW += TextMetrics::MeasureWidth(item->label, textSize);
        }
        if (item->checked) {
            itemW += 24.0f;
        }
        if (!item->shortcut.empty()) {
            itemW += 24.0f + TextMetrics::MeasureWidth(item->shortcut, textSize);
        }
        maxContentW = std::max(maxContentW, itemW);
    }

    const float calcW = std::clamp(maxContentW, 140.0f, 340.0f);
    const float fullH = m_PaddingY * 2.0f + static_cast<float>(m_Items.size()) * m_ItemHeight;
    const float maxAllowedH = availableSize.height > 0.0f ? std::min(availableSize.height, 360.0f) : 360.0f;
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
    // 100% Solid Opaque Neutral Charcoal Popup Surface (#18191B / #222326)
    context.DrawShadow(m_Geometry, ResolveColor(ColorToken::ShadowSubtle), 4.0f, 10.0f);
    context.DrawRoundedRect(m_Geometry, ResolveColor(ColorToken::PopupBackground), 4.0f);
    context.DrawRoundedRectOutline(m_Geometry, ResolveColor(ColorToken::BorderLight), 1.0f, 4.0f);

    context.PushClipRect(m_Geometry);

    const float textSize = ThemeMetric(MetricToken::TextSizeSmall);
    float y = m_Geometry.y + m_PaddingY - m_ScrollOffset;

    for (size_t i = 0; i < m_Items.size(); ++i) {
        const auto& item = m_Items[i];
        if (!item) continue;

        const Rect itemRect{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, m_ItemHeight };

        if (y + m_ItemHeight >= m_Geometry.y && y <= m_Geometry.y + m_Geometry.height) {
            if (item->label.empty()) {
                // Separator line
                const float sepY = itemRect.y + m_ItemHeight * 0.5f;
                context.DrawRect(Rect{ itemRect.x + 4.0f, sepY, itemRect.width - 8.0f, 1.0f }, ResolveColor(ColorToken::BorderLight));
            } else {
                if (m_HoveredItem == static_cast<int>(i) && item->enabled) {
                    context.DrawRoundedRect(itemRect, ResolveColor(ColorToken::HoverBackground), 2.0f);
                }

                const Color textColor = item->enabled
                    ? ResolveColor(ColorToken::TextPrimary)
                    : ResolveColor(ColorToken::TextDisabled);
                const float textY = itemRect.y + (m_ItemHeight - textSize) * 0.5f;

                context.DrawText(item->label, Point{ itemRect.x + m_PaddingX, textY }, textColor, textSize);

                if (item->checked) {
                    const float iconSize = 14.0f;
                    const float iconX = itemRect.x + itemRect.width - m_PaddingX - iconSize;
                    const float iconY = itemRect.y + (m_ItemHeight - iconSize) * 0.5f;
                    IconPainter::DrawIcon(context, Icons::CheckName, Rect{ iconX, iconY, iconSize, iconSize }, ResolveColor(ColorToken::AccentPrimary));
                } else if (!item->shortcut.empty()) {
                    const float shortcutW = TextMetrics::MeasureWidth(item->shortcut, textSize);
                    const float shortcutX = itemRect.x + itemRect.width - m_PaddingX - shortcutW;
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

void DropdownMenu::OnMouseMove(const MouseEvent& event) {
    const int hovered = HitItemAt(event.position);
    if (hovered != m_HoveredItem) {
        m_HoveredItem = hovered;
        InvalidatePaint();
    }
}

void DropdownMenu::OnMouseWheel(const MouseEvent& event) {
    const float fullH = m_PaddingY * 2.0f + static_cast<float>(m_Items.size()) * m_ItemHeight;
    const float maxScroll = std::max(0.0f, fullH - m_Geometry.height);
    m_ScrollOffset = std::clamp(m_ScrollOffset - event.wheelDeltaY * 24.0f, 0.0f, maxScroll);
    InvalidatePaint();
}

void DropdownMenu::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        const int clickedItem = HitItemAt(event.position);
        if (auto* overlay = GetPopupHost()) {
            overlay->CloseTopPopup();
        }
        if (clickedItem >= 0 && clickedItem < static_cast<int>(m_Items.size())) {
            const auto& item = m_Items[static_cast<size_t>(clickedItem)];
            if (item && item->enabled && item->onClick) {
                item->onClick();
            }
        }
    }
}

} // namespace we::editor::menus