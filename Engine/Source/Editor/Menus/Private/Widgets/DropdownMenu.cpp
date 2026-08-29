#include "Widgets/DropdownMenu.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/ControlChrome.h"
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
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;

DropdownMenu::DropdownMenu(const std::vector<std::shared_ptr<MenuItem>>& items)
    : m_Items(items)
{
    m_ItemHeight = ThemeMetric(MetricToken::MenuItemHeight);
    m_PaddingY = ThemeMetric(MetricToken::MenuPadding);
    m_PaddingX = ThemeMetric(MetricToken::Space2);
}

Size DropdownMenu::Measure(const Size& availableSize) {
    const float textSize = ThemeMetric(MetricToken::TextSizeSmall);
    float maxContentW = ThemeMetric(MetricToken::PopupMinWidth);

    for (const auto& item : m_Items) {
        if (!item) continue;
        float itemW = m_PaddingX * 2.0f;
        if (!item->label.empty()) {
            itemW += TextMetrics::MeasureWidth(item->label, textSize);
        }
        if (item->checked) {
            itemW += ThemeMetric(MetricToken::MenuTextIndent);
        }
        if (!item->shortcut.empty()) {
            itemW += ThemeMetric(MetricToken::MenuTextIndent) + TextMetrics::MeasureWidth(item->shortcut, textSize);
        }
        maxContentW = std::max(maxContentW, itemW);
    }

    const float calcW = std::clamp(maxContentW, ThemeMetric(MetricToken::PopupMinWidth), ThemeMetric(MetricToken::PopupMaxWidth));
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

    const float textSize = ThemeMetric(MetricToken::TextSizeSmall);
    const float checkSize = ThemeMetric(MetricToken::CheckMarkSize);
    float y = m_Geometry.y + m_PaddingY - m_ScrollOffset;

    for (size_t i = 0; i < m_Items.size(); ++i) {
        const auto& item = m_Items[i];
        if (!item) continue;

        const Rect itemRect{ m_Geometry.x + m_PaddingX, y, m_Geometry.width - m_PaddingX * 2.0f, m_ItemHeight };

        if (y + m_ItemHeight >= m_Geometry.y && y <= m_Geometry.y + m_Geometry.height) {
            if (item->label.empty()) {
                const float sepY = itemRect.y + m_ItemHeight * 0.5f;
                context.DrawRect(
                    Rect{ itemRect.x, sepY, itemRect.width, ThemeMetric(MetricToken::BorderWidth) },
                    ResolveColor(ColorToken::Separator));
            } else {
                if (m_HoveredItem == static_cast<int>(i) && item->enabled) {
                    ControlChrome::InteractionState state{};
                    state.hoverAnim = 1.0f;
                    ControlChrome::PaintListRow(context, itemRect, state);
                }

                const Color textColor = item->enabled
                    ? ResolveColor(ColorToken::TextPrimary)
                    : ResolveColor(ColorToken::TextDisabled);
                const float textY = itemRect.y + (m_ItemHeight - textSize) * 0.5f;

                context.DrawText(item->label, Point{ itemRect.x, textY }, textColor, textSize);

                if (item->checked) {
                    const float iconX = itemRect.x + itemRect.width - m_PaddingX - checkSize;
                    const float iconY = itemRect.y + (m_ItemHeight - checkSize) * 0.5f;
                    IconPainter::DrawIcon(
                        context,
                        Icons::CheckName,
                        Rect{ iconX, iconY, checkSize, checkSize },
                        ResolveColor(ColorToken::AccentPrimary));
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
    m_ScrollOffset = std::clamp(m_ScrollOffset - event.wheelDeltaY * ThemeMetric(MetricToken::ListRowHeight), 0.0f, maxScroll);
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