// ==============================================================================
// WindEffects — KindUI — ColorPicker
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/ColorPicker.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

ColorPicker::ColorPicker()
    : m_Color{ 1.0f, 1.0f, 1.0f, 1.0f }
{
}

ColorPicker::ColorPicker(const Color& initialColor)
    : m_Color(initialColor)
{
}

Size ColorPicker::Measure(const Size& availableSize) {
    (void)availableSize;
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float size = ResolveMetric(MetricToken::ControlHeightCompact) * scale;
    return Size{ size, size };
}

void ColorPicker::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ColorPicker::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }

    const Rect swatchRect = GetSwatchRect();
    
    // Draw the color swatch
    context.DrawRoundedRect(swatchRect, m_Color, ResolveMetric(MetricToken::CornerRadiusSmall));
    
    // Draw border
    const float borderWidth = (std::max)(1.0f, ResolveMetric(MetricToken::BorderWidth));
    Color borderColor = ResolveColor(ColorToken::BorderDefault);
    if (m_HoverAnim > 0.001f) {
        borderColor = Color::Pick(borderColor, ResolveColor(ColorToken::BorderLight), m_HoverAnim);
    }
    context.DrawRoundedRectOutline(swatchRect, borderColor, borderWidth, ResolveMetric(MetricToken::CornerRadiusSmall));
    
    // Draw a small indicator if open
    if (m_IsOpen) {
        const float indicatorSize = 4.0f;
        const float scale = (std::max)(1.0f, DPIContext::GetScale());
        const float cornerRadius = ResolveMetric(MetricToken::CornerRadiusSmall) * scale;
        const Rect indicatorRect{
            swatchRect.x + swatchRect.width - indicatorSize - 2.0f,
            swatchRect.y + swatchRect.height - indicatorSize - 2.0f,
            indicatorSize,
            indicatorSize
        };
        context.DrawRoundedRect(indicatorRect, ResolveColor(ColorToken::TextOnAccent), 2.0f);
    }
}

void ColorPicker::Tick(float deltaTime) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}

void ColorPicker::OnMouseDown(const MouseEvent& event) {
    if (!m_Visible || event.button != MouseButton::Left) {
        return;
    }
    
    if (IsInSwatch(event.position)) {
        m_IsOpen = !m_IsOpen;
        InvalidatePaint();
    }
}

void ColorPicker::OnMouseMove(const MouseEvent& event) {
    if (!m_Visible) {
        return;
    }
    m_Hovered = IsInSwatch(event.position);
}

void ColorPicker::OnMouseUp(const MouseEvent& event) {
    (void)event;
}

bool ColorPicker::ShowsPointerCursor(const Point& position) const {
    return IsInSwatch(position);
}

void ColorPicker::SetColor(const Color& color) {
    if (m_Color.r != color.r || m_Color.g != color.g || m_Color.b != color.b || m_Color.a != color.a) {
        m_Color = color;
        InvalidatePaint();
        if (m_OnColorChanged) {
            m_OnColorChanged(m_Color);
        }
    }
}

Rect ColorPicker::GetSwatchRect() const {
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float size = ResolveMetric(MetricToken::ControlHeightCompact) * scale;
    
    return Rect{
        m_Geometry.x,
        m_Geometry.y + (m_Geometry.height - size) * 0.5f,
        size,
        size
    };
}

bool ColorPicker::IsInSwatch(const Point& pos) const {
    return GetSwatchRect().Contains(pos);
}

} // namespace we::runtime::kindui