#include "Core/Widgets/ToolbarNavigationButton.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include "Core/Animator.hpp"
#include <algorithm>

namespace we::UI {

ToolbarNavigationButton::ToolbarNavigationButton(const std::string& iconName, const char* tooltip)
    : m_IconName(iconName)
{
    (void)tooltip;
}

Size ToolbarNavigationButton::Measure(const Size& availableSize) {
    (void)availableSize;
    m_DesiredSize = Size{ DesignTokens::kNavigationButtonSize, DesignTokens::kNavigationButtonSize };
    return m_DesiredSize;
}

void ToolbarNavigationButton::Arrange(const Rect& allottedRect) {
    const float size = DesignTokens::kNavigationButtonSize;
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - size) * 0.5f,
        size,
        size
    };
}

void ToolbarNavigationButton::Paint(PaintContext& context) {
    using namespace DesignTokens;
    
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && m_Enabled ? 1.0f : 0.0f, kHoverDamping);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed && m_Enabled ? 1.0f : 0.0f, kPressDamping);
    
    // Animate press offset
    const float targetOffset = m_Pressed && m_Enabled ? kPressOffset : 0.0f;
    m_PressOffset = Animator::Damp(m_PressOffset, targetOffset, kPressDamping);

    const auto& theme = Theme::Get();
    const float radius = 4.0f; // Small rounded square
    
    // Determine colors based on state
    Color bgColor = m_Enabled ? NavigationButtonNormal() : DisabledBackground();
    Color borderColor = m_Enabled ? NavigationButtonBorder() : DisabledBorder();
    Color iconColor = m_Enabled ? (m_Selected ? NavigationButtonIconHover() : NavigationButtonIcon()) : DisabledIcon();
    
    if (m_Enabled && m_HoverAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, NavigationButtonHover(), m_HoverAnim);
        iconColor = Color::Lerp(iconColor, NavigationButtonIconHover(), m_HoverAnim);
        borderColor = Color::Lerp(borderColor, NavigationButtonBorderHover(), m_HoverAnim);
    }
    if (m_Enabled && m_PressAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, NavigationButtonPressed(), m_PressAnim);
    }

    // Calculate button rect with press offset
    Rect buttonRect = m_Geometry;
    buttonRect.y += m_PressOffset;

    // Draw button background (transparent by default, integrated)
    if (bgColor.a > 0.01f) {
        context.DrawRoundedRect(buttonRect, bgColor, radius);
    }

    // Draw border only on hover
    if (m_Enabled && m_HoverAnim > 0.01f) {
        context.DrawRoundedRectOutline(buttonRect, borderColor, 0.5f, radius);
    }

    // Draw icon centered
    const float iconSize = kNavigationIconSize;
    const float iconX = buttonRect.x + (buttonRect.width - iconSize) * 0.5f;
    const float iconY = buttonRect.y + (buttonRect.height - iconSize) * 0.5f;
    IconPainter::DrawIcon(context, m_IconName, Rect{ iconX, iconY, iconSize, iconSize }, iconColor);
}

void ToolbarNavigationButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Enabled) m_Pressed = true;
}

void ToolbarNavigationButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Enabled && m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void ToolbarNavigationButton::OnMouseMove(const MouseEvent& event) {
    m_Hovered = m_Geometry.Contains(event.position);
}

} // namespace we::UI
