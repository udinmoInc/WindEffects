#include "Core/Widgets/ToolbarIconButton.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include <algorithm>

namespace WindEffects::Editor::UI {

ToolbarIconButton::ToolbarIconButton(const std::string& iconName, const char* tooltip)
    : m_IconName(iconName)
{
    (void)tooltip;
}

Size ToolbarIconButton::Measure(const Size& availableSize) {
    (void)availableSize;
    m_DesiredSize = Size{ DesignTokens::kIconButtonSize, DesignTokens::kIconButtonSize };
    return m_DesiredSize;
}

void ToolbarIconButton::Arrange(const Rect& allottedRect) {
    const float size = DesignTokens::kIconButtonSize;
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - size) * 0.5f,
        size,
        size
    };
}

void ToolbarIconButton::Paint(PaintContext& context) {
    using namespace DesignTokens;
    
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && m_Enabled ? 1.0f : 0.0f, kHoverDamping);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed && m_Enabled ? 1.0f : 0.0f, kPressDamping);
    
    // Animate press offset
    const float targetOffset = m_Pressed && m_Enabled ? kPressOffset : 0.0f;
    m_PressOffset = Animator::Damp(m_PressOffset, targetOffset, kPressDamping);

    const auto& theme = Theme::Get();
    const float radius = kIconButtonRadius;
    
    // Determine colors based on state
    Color bgColor = m_Enabled ? IconButtonNormal() : DisabledBackground();
    Color borderColor = m_Enabled ? IconButtonBorder() : DisabledBorder();
    Color iconColor = m_Enabled ? (m_Selected ? IconButtonIconHover() : IconButtonIcon()) : DisabledIcon();
    
    if (m_Enabled && m_HoverAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, IconButtonHover(), m_HoverAnim);
        iconColor = Color::Lerp(iconColor, IconButtonIconHover(), m_HoverAnim);
        borderColor = Color::Lerp(borderColor, IconButtonBorderHover(), m_HoverAnim);
    }
    if (m_Enabled && m_PressAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, IconButtonPressed(), m_PressAnim);
        iconColor = Color::Lerp(iconColor, theme.IconActive, m_PressAnim);
    }

    // Calculate button rect with press offset
    Rect buttonRect = m_Geometry;
    buttonRect.y += m_PressOffset;

    // Draw button background (transparent by default, recessed)
    if (bgColor.a > 0.01f) {
        context.DrawRoundedRect(buttonRect, bgColor, radius);
    }

    // Draw subtle border (recessed appearance)
    context.DrawRoundedRectOutline(buttonRect, borderColor, 0.5f, radius);

    // Draw icon centered
    const float iconSize = kIconSize;
    const float iconX = buttonRect.x + (buttonRect.width - iconSize) * 0.5f;
    const float iconY = buttonRect.y + (buttonRect.height - iconSize) * 0.5f;
    IconPainter::DrawIcon(context, m_IconName, Rect{ iconX, iconY, iconSize, iconSize }, iconColor);
}

void ToolbarIconButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Enabled) m_Pressed = true;
}

void ToolbarIconButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Enabled && m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void ToolbarIconButton::OnMouseMove(const MouseEvent& event) {
    m_Hovered = m_Geometry.Contains(event.position);
}

} // namespace WindEffects::Editor::UI
