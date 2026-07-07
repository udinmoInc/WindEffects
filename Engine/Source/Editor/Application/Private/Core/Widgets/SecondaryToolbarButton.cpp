#include "Core/Widgets/SecondaryToolbarButton.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include <algorithm>

namespace we::UI {

SecondaryToolbarButton::SecondaryToolbarButton(const std::string& label, const std::string& iconName)
    : m_Label(label)
    , m_IconName(iconName)
{}

Size SecondaryToolbarButton::Measure(const Size& availableSize) {
    (void)availableSize;
    float width = DesignTokens::kButtonPaddingHorizontal * 2.0f;
    if (!m_IconName.empty()) width += DesignTokens::kIconSize + 6.0f;
    width += static_cast<float>(m_Label.size()) * 7.2f;
    m_DesiredSize = Size{ width, DesignTokens::kButtonHeight };
    return m_DesiredSize;
}

void SecondaryToolbarButton::Arrange(const Rect& allottedRect) {
    const float h = std::min(DesignTokens::kButtonHeight, allottedRect.height);
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        allottedRect.width,
        h
    };
}

void SecondaryToolbarButton::Paint(PaintContext& context) {
    using namespace DesignTokens;
    
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && m_Enabled ? 1.0f : 0.0f, kHoverDamping);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed && m_Enabled ? 1.0f : 0.0f, kPressDamping);
    
    // Animate press offset
    const float targetOffset = m_Pressed && m_Enabled ? kPressOffset : 0.0f;
    m_PressOffset = Animator::Damp(m_PressOffset, targetOffset, kPressDamping);

    const auto& theme = Theme::Get();
    const float radius = kButtonCornerRadius;
    
    // Determine colors based on state
    Color bgColor = m_Enabled ? SecondaryButtonNormal() : DisabledBackground();
    Color borderColor = m_Enabled ? SecondaryButtonBorder() : DisabledBorder();
    Color textColor = m_Enabled ? SecondaryButtonText() : DisabledText();
    
    if (m_Enabled && m_HoverAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, SecondaryButtonHover(), m_HoverAnim);
        textColor = Color::Lerp(textColor, SecondaryButtonTextHover(), m_HoverAnim);
        borderColor = Color::Lerp(borderColor, SecondaryButtonBorderHover(), m_HoverAnim);
    }
    if (m_Enabled && m_PressAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, SecondaryButtonPressed(), m_PressAnim);
    }

    // Calculate button rect with press offset
    Rect buttonRect = m_Geometry;
    buttonRect.y += m_PressOffset;

    // Draw button background (transparent by default)
    if (bgColor.a > 0.01f) {
        context.DrawRoundedRect(buttonRect, bgColor, radius);
    }

    // Draw border only on hover
    if (m_Enabled && m_HoverAnim > 0.01f) {
        context.DrawRoundedRectOutline(buttonRect, borderColor, 0.5f, radius);
    }

    // Draw icon and text
    const float hPad = kButtonPaddingHorizontal;
    float x = buttonRect.x + hPad;
    const float textSize = 13.0f;
    const float textY = buttonRect.y + (buttonRect.height - textSize) * 0.5f;

    if (!m_IconName.empty()) {
        const float iconSize = kIconSize;
        const float iconY = buttonRect.y + (buttonRect.height - iconSize) * 0.5f;
        Color iconColor = textColor;
        IconPainter::DrawIcon(context, m_IconName, Rect{ x, iconY, iconSize, iconSize }, iconColor);
        x += iconSize + 6.0f;
    }

    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, false);
}

void SecondaryToolbarButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Enabled) m_Pressed = true;
}

void SecondaryToolbarButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Enabled && m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void SecondaryToolbarButton::OnMouseMove(const MouseEvent& event) {
    m_Hovered = m_Geometry.Contains(event.position);
}

} // namespace we::UI
