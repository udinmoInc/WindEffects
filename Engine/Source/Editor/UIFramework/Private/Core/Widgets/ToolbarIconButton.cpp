#include "Core/Widgets/ToolbarIconButton.h"
#include "Core/PaintContext.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include "Rendering/IconMetrics.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include <algorithm>

namespace WindEffects::Editor::UI {

ToolbarIconButton::ToolbarIconButton(const std::string& iconName, const char* tooltip)
    : m_IconName(iconName)
{
    (void)tooltip;
}

Size ToolbarIconButton::Measure(const Size& availableSize) {
    (void)availableSize;
    const float size = ThemeMetric(ThemeToken::IconButtonSize);
    m_DesiredSize = Size{ size, size };
    return m_DesiredSize;
}

void ToolbarIconButton::Arrange(const Rect& allottedRect) {
    const float size = ThemeMetric(ThemeToken::IconButtonSize);
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - size) * 0.5f,
        size,
        size
    };
}

void ToolbarIconButton::Paint(PaintContext& context) {
    const auto baseStyle = ResolveStyle(StyleRole::IconButton);
    const auto hoverStyle = ResolveStyle(StyleRole::IconButtonHover);
    const auto pressStyle = ResolveStyle(StyleRole::IconButtonPressed);

    const float hoverDamping = ThemeMetric(ThemeToken::HoverAnimationDamping);
    const float pressDamping = ThemeMetric(ThemeToken::PressAnimationDamping);
    const float pressOffsetTarget = ThemeMetric(ThemeToken::PressOffset);

    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && m_Enabled ? 1.0f : 0.0f, hoverDamping);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed && m_Enabled ? 1.0f : 0.0f, pressDamping);

    const float targetOffset = m_Pressed && m_Enabled ? pressOffsetTarget : 0.0f;
    m_PressOffset = Animator::Damp(m_PressOffset, targetOffset, pressDamping);

    const float radius = baseStyle.cornerRadius;

    Color bgColor = m_Enabled ? baseStyle.background : ThemeColor(ThemeToken::DisabledBackground);
    Color borderColor = m_Enabled ? baseStyle.border : ThemeColor(ThemeToken::BorderDark);
    Color iconColor = m_Enabled ? (m_Selected ? hoverStyle.icon : baseStyle.icon) : ThemeColor(ThemeToken::IconDisabled);

    if (m_Enabled && m_HoverAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, hoverStyle.background, m_HoverAnim);
        iconColor = Color::Lerp(iconColor, hoverStyle.icon, m_HoverAnim);
        borderColor = Color::Lerp(borderColor, hoverStyle.border, m_HoverAnim);
    }
    if (m_Enabled && m_PressAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, pressStyle.background, m_PressAnim);
        iconColor = Color::Lerp(iconColor, pressStyle.icon, m_PressAnim);
    }

    Rect buttonRect = m_Geometry;
    buttonRect.y += m_PressOffset;

    if (bgColor.a > 0.01f) {
        context.DrawRoundedRect(buttonRect, bgColor, radius);
    }

    context.DrawRoundedRectOutline(buttonRect, borderColor, baseStyle.borderWidth * 0.5f, radius);

    const float iconSize = static_cast<float>(IconMetrics::StandardGlyphTierPx());
    IconPainter::DrawIcon(context, m_IconName, IconMetrics::PlaceGlyphCentered(buttonRect, iconSize), iconColor);
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
