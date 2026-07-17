#include "Widgets/DropdownMenu.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Layout/OverlayManager.h"

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::menus {
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyEventType;
using ::we::runtime::kindui::IconPainter;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;


DropdownMenu::DropdownMenu(const std::vector<std::shared_ptr<MenuItem>>& items)
    : m_Items(items)
{
}

Size DropdownMenu::Measure(const Size& availableSize) {
    float maxWidth = 100.0f; // min width
    for (const auto& item : m_Items) {
        float textWidth = item->label.length() * ThemeMetric(MetricToken::TextSizeMenu) * 0.6f; // rough estimate
        if (!item->shortcut.empty()) {
            textWidth += item->shortcut.length() * ThemeMetric(MetricToken::TextSizeMenu) * 0.6f + 20.0f;
        }
        maxWidth = std::max(maxWidth, textWidth + m_PaddingX * 2.0f);
    }
    
    float height = m_PaddingY * 2.0f + m_Items.size() * m_ItemHeight;
    m_DesiredSize = Size{ maxWidth, height };
    return m_DesiredSize;
}

void DropdownMenu::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

int DropdownMenu::HitItemAt(const Point& pos) const {
    if (!m_Geometry.Contains(pos)) {
        return -1;
    }
    float y = m_Geometry.y + m_PaddingY;
    for (size_t i = 0; i < m_Items.size(); ++i) {
        const Rect itemRect{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, m_ItemHeight };
        if (itemRect.Contains(pos)) {
            return static_cast<int>(i);
        }
        y += m_ItemHeight;
    }
    return -1;
}

void DropdownMenu::Paint(PaintContext& context) {
    context.DrawRoundedRect(m_Geometry, ThemeColor(ColorToken::PopupBackground), ThemeMetric(MetricToken::CornerRadiusSmall));

    float y = m_Geometry.y + m_PaddingY;

    for (size_t i = 0; i < m_Items.size(); ++i) {
        const auto& item = m_Items[i];
        Rect itemRect{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, m_ItemHeight };

        if (m_HoveredItem == (int)i) {
            context.DrawRect(itemRect, ThemeColor(ColorToken::HoverBackground));
        }

        const float textSize = ThemeMetric(MetricToken::TextSizeSmall);
        float textY = itemRect.y + (m_ItemHeight - textSize) * 0.5f;
        context.DrawText(item->label, Point{ itemRect.x + m_PaddingX, textY }, ThemeColor(ColorToken::TextPrimary), textSize);
        
        if (!item->shortcut.empty()) {
            float shortcutWidth = item->shortcut.length() * textSize * 0.55f;
            float shortcutX = itemRect.x + itemRect.width - m_PaddingX - shortcutWidth;
            context.DrawText(item->shortcut, Point{ shortcutX, textY }, ThemeColor(ColorToken::TextSecondary), textSize);
        }
        
        y += m_ItemHeight;
    }
}

void DropdownMenu::OnMouseMove(const MouseEvent& event) {
    m_HoveredItem = HitItemAt(event.position);
}

void DropdownMenu::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        const int clickedItem = HitItemAt(event.position);
        if (auto* overlay = GetPopupHost()) {
            overlay->CloseTopPopup();
        }
        if (clickedItem >= 0 && clickedItem < static_cast<int>(m_Items.size())) {
            const auto& item = m_Items[static_cast<size_t>(clickedItem)];
            if (item->enabled && item->onClick) {
                item->onClick();
            }
        }
    }
}

} // namespace we::editor::menus