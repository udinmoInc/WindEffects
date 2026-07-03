#include "Core/Widgets/PrimaryToolbarButton.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include "Core/Animator.hpp"
#include <algorithm>

namespace we::UI {

PrimaryToolbarButton::PrimaryToolbarButton(const std::string& label, const std::string& iconName)
    : m_Label(label)
    , m_IconName(iconName)
{}

Size PrimaryToolbarButton::Measure(const Size& availableSize) {
    (void)availableSize;
    float width = DesignTokens::kButtonPaddingHorizontal * 2.0f;
    if (!m_IconName.empty()) width += DesignTokens::kIconSize + 6.0f;
    width += static_cast<float>(m_Label.size()) * 7.2f;
    m_DesiredSize = Size{ width, DesignTokens::kButtonHeight };
    return m_DesiredSize;
}

void PrimaryToolbarButton::Arrange(const Rect& allottedRect) {
    const float h = std::min(DesignTokens::kButtonHeight, allottedRect.height);
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        allottedRect.width,
        h
    };
}

void PrimaryToolbarButton::Paint(PaintContext& context) {
    using namespace DesignTokens;
    
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && m_Enabled ? 1.0f : 0.0f, kHoverDamping);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed && m_Enabled ? 1.0f : 0.0f, kPressDamping);
    
    // Animate press offset
    const float targetOffset = m_Pressed && m_Enabled ? kPressOffset : 0.0f;
    m_PressOffset = Animator::Damp(m_PressOffset, targetOffset, kPressDamping);

    const auto& theme = Theme::Get();
    const float radius = kButtonCornerRadius;
    
    // Determine colors based on state
    Color bgColor = m_Enabled ? PrimaryButtonNormal() : DisabledBackground();
    Color borderColor = m_Enabled ? PrimaryButtonBorder() : DisabledBorder();
    Color textColor = m_Enabled ? PrimaryButtonText() : DisabledText();
    
    if (m_Enabled && m_HoverAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, PrimaryButtonHover(), m_HoverAnim);
    }
    if (m_Enabled && m_PressAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, PrimaryButtonPressed(), m_PressAnim);
    }

    // Calculate button rect with press offset
    Rect buttonRect = m_Geometry;
    buttonRect.y += m_PressOffset;

    // Draw button background (integrated surface)
    context.DrawRoundedRect(buttonRect, bgColor, radius);

    // Draw very subtle top highlight (integrated depth)
    if (m_Enabled) {
        Color highlight = PrimaryButtonHighlight();
        context.DrawRoundedRectOutline(
            Rect{ buttonRect.x + 0.5f, buttonRect.y + 0.5f, buttonRect.width - 1.0f, buttonRect.height - 1.0f },
            highlight, 0.5f, radius
        );
    }

    // Draw very subtle border (integrated look)
    context.DrawRoundedRectOutline(buttonRect, borderColor, 0.5f, radius);

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

void PrimaryToolbarButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Enabled) m_Pressed = true;
}

void PrimaryToolbarButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Enabled && m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void PrimaryToolbarButton::OnMouseMove(const MouseEvent& event) {
    m_Hovered = m_Geometry.Contains(event.position);
}

} // namespace we::UI
