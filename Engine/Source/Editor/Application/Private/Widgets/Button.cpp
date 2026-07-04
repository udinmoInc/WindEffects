#include "Widgets/Button.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Style.hpp"
#include "Core/DPIContext.hpp"
#include "Core/Animator.hpp"

namespace we::UI {

Button::Button(const std::string& labelText, std::function<void()> onClicked)
    : m_Text(labelText)
    , m_OnClicked(onClicked)
    , m_Style(WidgetStyle::Button())
{}

Size Button::Measure(const Size& availableSize) {
    (void)availableSize;
    auto& theme = Theme::Get();
    
    // Calculate text width using proper font metrics
    float textWidth = m_Text.length() * theme.TextSizeBody * 0.6f;
    float width = textWidth + m_Style.padding.left + m_Style.padding.right;
    float height = theme.TextSizeBody + m_Style.padding.top + m_Style.padding.bottom;

    m_DesiredSize = Size{ width, height };
    return m_DesiredSize;
}

void Button::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void Button::Paint(PaintContext& context) {
    if (!m_Visible) return;

    // 1. Tick Animations
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 25.0f);

    // 2. Interpolate Colors using Style system
    Color currentBg = m_Style.background.color;
    if (m_HoverAnim > 0.01f) {
        currentBg = Color::Lerp(currentBg, m_Style.backgroundHover.color, m_HoverAnim);
    }
    if (m_PressAnim > 0.01f) {
        currentBg = Color::Lerp(currentBg, m_Style.backgroundPressed.color, m_PressAnim);
    }

    // 3. Draw rounded button background
    context.DrawRoundedRect(m_Geometry, currentBg, m_Style.background.cornerRadius);

    // 4. Draw subtle border
    Color borderColor = m_Style.border.color;
    if (m_HoverAnim > 0.01f) {
        borderColor.a = std::min(1.0f, borderColor.a + m_HoverAnim * 0.3f);
    }
    context.DrawRoundedRectOutline(m_Geometry, borderColor, m_Style.border.width, m_Style.background.cornerRadius);

    // 5. Draw text centered
    float textWidth = m_Text.length() * m_Style.text.size * 0.6f;
    float textX = m_Geometry.x + (m_Geometry.width - textWidth) * 0.5f;
    float textY = m_Geometry.y + (m_Geometry.height - m_Style.text.size) * 0.5f;
    
    Color textColor = m_Style.text.color;
    if (m_PressAnim > 0.5f) {
        textColor = Color::Lerp(textColor, Color{0.1f, 0.1f, 0.1f, 1.0f}, m_PressAnim);
    }
    
    context.DrawText(m_Text, Point{ textX, textY }, textColor, m_Style.text.size);
}

void Button::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = true;
    }
}

void Button::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

} // namespace we::editor::application::UI
