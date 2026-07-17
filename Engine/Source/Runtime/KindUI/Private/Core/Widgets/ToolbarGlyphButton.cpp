#include "KindUI/Core/Widgets/ToolbarGlyphButton.h"

#include "KindUI/Core/Animator.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>


namespace we::runtime::kindui {

ToolbarGlyphButton::ToolbarGlyphButton(
    std::string iconName,
    StyleRole role,
    MetricToken sizeToken,
    MetricToken iconSizeToken)
    : m_IconName(std::move(iconName))
    , m_Role(role)
    , m_SizeToken(sizeToken)
    , m_IconSizeToken(iconSizeToken)
{
    SetFocusable(false);
}

Size ToolbarGlyphButton::Measure(const Size& availableSize) {
    (void)availableSize;
    const float size = ThemeMetric(m_SizeToken);
    m_DesiredSize = Size{ size, size };
    return m_DesiredSize;
}

void ToolbarGlyphButton::Arrange(const Rect& allottedRect) {
    const float size = ThemeMetric(m_SizeToken);
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - size) * 0.5f,
        size,
        size
    };
}

void ToolbarGlyphButton::Paint(PaintContext& context) {
    const auto baseStyle = ResolveStyle(m_Role);
    const auto hoverStyle = ResolveStyle(StyleRole::IconButtonHover);
    const auto pressStyle = ResolveStyle(StyleRole::IconButtonPressed);

    const float hoverDamping = ThemeMetric(MetricToken::HoverAnimationDamping);
    const float pressDamping = ThemeMetric(MetricToken::PressAnimationDamping);
    const float pressOffsetTarget = ThemeMetric(MetricToken::PressOffset);

    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && IsEnabled() ? 1.0f : 0.0f, hoverDamping);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed && IsEnabled() ? 1.0f : 0.0f, pressDamping);

    const float targetOffset = m_Pressed && IsEnabled() ? pressOffsetTarget : 0.0f;
    m_PressOffset = Animator::Damp(m_PressOffset, targetOffset, pressDamping);

    Color bgColor = IsEnabled() ? baseStyle.background : ThemeColor(ColorToken::DisabledBackground);
    Color borderColor = IsEnabled() ? baseStyle.border : ThemeColor(ColorToken::BorderDark);
    Color iconColor = IsEnabled()
        ? (IsSelected() ? hoverStyle.icon : baseStyle.icon)
        : ThemeColor(ColorToken::IconDisabled);

    if (IsEnabled() && m_HoverAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, hoverStyle.background, m_HoverAnim);
        iconColor = Color::Lerp(iconColor, hoverStyle.icon, m_HoverAnim);
        borderColor = Color::Lerp(borderColor, hoverStyle.border, m_HoverAnim);
    }
    if (IsEnabled() && m_PressAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, pressStyle.background, m_PressAnim);
        iconColor = Color::Lerp(iconColor, pressStyle.icon, m_PressAnim);
    }

    Rect buttonRect = m_Geometry;
    buttonRect.y += m_PressOffset;

    if (bgColor.a > 0.01f) {
        context.DrawRoundedRect(buttonRect, bgColor, baseStyle.cornerRadius);
    }

    if (IsEnabled() && (m_Role == StyleRole::IconButton || m_HoverAnim > 0.01f)) {
        context.DrawRoundedRectOutline(buttonRect, borderColor, baseStyle.borderWidth * 0.5f, baseStyle.cornerRadius);
    }

    const float iconSize = static_cast<float>(IconMetrics::GlyphTierPx(m_IconSizeToken));
    IconPainter::DrawIcon(context, m_IconName, IconMetrics::PlaceGlyphCentered(buttonRect, iconSize), iconColor);
}

void ToolbarGlyphButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && IsEnabled()) {
        SetPressed(true);
    }
}

void ToolbarGlyphButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        SetPressed(false);
        if (IsEnabled() && m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void ToolbarGlyphButton::Tick(float deltaTime) {
    (void)deltaTime;
    Widget::Tick(deltaTime);
}

} // namespace we::runtime::kindui
