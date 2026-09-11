// ==============================================================================
// WindEffects — KindUI — ToolbarGlyphButton
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/Widgets/ToolbarGlyphButton.h"

#include "KindUI/Core/Animator.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>

namespace we::runtime::kindui {

ToolbarGlyphButton::ToolbarGlyphButton(
    WindIconRef icon,
    StyleRole role,
    MetricToken sizeToken,
    MetricToken iconSizeToken)
    : m_Icon(icon)
    , m_Role(role)
    , m_SizeToken(sizeToken)
    , m_IconSizeToken(iconSizeToken)
{
    SetFocusable(false);
}

void ToolbarGlyphButton::SetOnClicked(std::function<void()> callback) {
    m_OnClicked = std::move(callback);
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
    Rect buttonRect = m_Geometry;
    buttonRect.y += m_PressOffset;

    const float activeStrength = IsEnabled() ? (std::max)(m_HoverAnim, m_PressAnim) : 0.0f;
    if (activeStrength > 0.01f) {
        Color hoverBg = m_Pressed ? ThemeColor(ColorToken::ControlBackgroundPressed) : ThemeColor(ColorToken::HoverBackground);
        hoverBg.a *= activeStrength;
        context.DrawRoundedRect(buttonRect, hoverBg, 4.0f);
    }

    if (m_Icon.IsValid()) {
        const float iconPx = ThemeMetric(m_IconSizeToken);
        we::runtime::kindui::ToolbarButtonChrome::PaintFloatingIcon(
            context,
            m_Icon,
            buttonRect,
            iconPx,
            IsEnabled() ? m_HoverAnim : 0.0f,
            IsEnabled() ? m_PressAnim : 0.0f,
            IsSelected());
    }
}

void ToolbarGlyphButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && IsEnabled()) {
        SetPressed(true);
    }
}

void ToolbarGlyphButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        const bool wasPressed = m_Pressed;
        SetPressed(false);
        if (IsEnabled() && (wasPressed || m_Geometry.Contains(event.position)) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void ToolbarGlyphButton::Tick(float deltaTime) {
    (void)deltaTime;
    const float hoverDamping = ThemeMetric(MetricToken::HoverAnimationDamping);
    const float pressDamping = ThemeMetric(MetricToken::PressAnimationDamping);
    const float pressOffsetTarget = ThemeMetric(MetricToken::PressOffset);

    const float targetHover = m_Hovered && IsEnabled() ? 1.0f : 0.0f;
    const float targetPress = m_Pressed && IsEnabled() ? 1.0f : 0.0f;
    const float targetOffset = m_Pressed && IsEnabled() ? pressOffsetTarget : 0.0f;

    m_HoverAnim = Animator::Damp(m_HoverAnim, targetHover, hoverDamping);
    m_PressAnim = Animator::Damp(m_PressAnim, targetPress, pressDamping);
    m_PressOffset = Animator::Damp(m_PressOffset, targetOffset, pressDamping);

    if (std::abs(m_HoverAnim - targetHover) > 0.001f ||
        std::abs(m_PressAnim - targetPress) > 0.001f ||
        std::abs(m_PressOffset - targetOffset) > 0.001f) {
        InvalidatePaint();
    }

    Widget::Tick(deltaTime);
}

} // namespace we::runtime::kindui
 
