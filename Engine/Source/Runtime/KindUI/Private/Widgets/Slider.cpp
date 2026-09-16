// ==============================================================================
// WindEffects — KindUI — Slider
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/Slider.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/InputEvents.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Theme/DesignToken.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Theme/ThemeManager.h"
#include "Platform/Platform.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace we::runtime::kindui {

Slider::Slider(double initialValue, double minValue, double maxValue, double step)
    : m_MinValue(minValue)
    , m_MaxValue(maxValue > minValue ? maxValue : minValue + 1.0)
    , m_Step(step > 0.0 ? step : 0.01)
    , m_StyleCacheValid(false)
{
    m_Value = ClampAndSnapValue(initialValue);
    SetFocusable(true);
}

Size Slider::Measure(const Size& availableSize) {
    const float scale = DPIContext::GetScale();
    const float h = LayoutMetrics::PropertyControlHeight();
    const float minW = 80.0f * scale;
    const float w = availableSize.width < 1.0e8f ? std::max(minW, availableSize.width) : minW;
    m_DesiredSize = Size{ w, h };
    return m_DesiredSize;
}

void Slider::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void Slider::Tick(float deltaTime) {
    if (!IsVisible()) {
        return;
    }
    (void)deltaTime;
    const float targetHover = m_Hovered || m_Dragging ? 1.0f : 0.0f;
    const float targetFocus = m_Focused ? 1.0f : 0.0f;
    m_HoverAnim = Animator::Damp(m_HoverAnim, targetHover, ControlChrome::HoverDamping());
    m_FocusAnim = Animator::Damp(m_FocusAnim, targetFocus, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}

double Slider::ClampAndSnapValue(double val) const {
    const double clamped = std::clamp(val, m_MinValue, m_MaxValue);
    if (m_Step <= 0.0) {
        return clamped;
    }
    const double steps = std::round((clamped - m_MinValue) / m_Step);
    const double snapped = m_MinValue + steps * m_Step;
    return std::clamp(snapped, m_MinValue, m_MaxValue);
}

void Slider::SetValue(double val) {
    const double newSnapped = ClampAndSnapValue(val);
    if (std::abs(m_Value - newSnapped) > 1e-9) {
        m_Value = newSnapped;
        InvalidatePaint();
        if (m_OnValueChanged) {
            m_OnValueChanged(m_Value);
        }
    }
}

void Slider::SetRange(double minVal, double maxVal) {
    m_MinValue = minVal;
    m_MaxValue = maxVal > minVal ? maxVal : minVal + 1.0;
    SetValue(m_Value);
}

void Slider::SetStep(double step) {
    m_Step = step > 0.0 ? step : 0.01;
    SetValue(m_Value);
}

std::string Slider::FormatValueString() const {
    char buf[64]{};
    const double roundVal = std::round(m_Value);
    if (std::abs(m_Value - roundVal) < 1e-4 && std::abs(m_Value) < 1e6) {
        std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(roundVal));
    } else {
        std::snprintf(buf, sizeof(buf), m_FormatString.c_str(), m_Value);
    }
    return std::string(buf);
}

void Slider::UpdateValueFromMouseX(float mouseX) {
    const float scale = DPIContext::GetScale();
    const float thumbRadius = 6.0f * scale;
    const float trackX = m_Geometry.x + thumbRadius;
    const float trackW = std::max(1.0f, m_Geometry.width - thumbRadius * 2.0f);

    const float normalized = std::clamp((mouseX - trackX) / trackW, 0.0f, 1.0f);
    const double rawVal = m_MinValue + static_cast<double>(normalized) * (m_MaxValue - m_MinValue);
    SetValue(rawVal);
}

void Slider::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left || m_ReadOnly) {
        return;
    }
    m_Focused = true;
    m_Dragging = true;
    UpdateValueFromMouseX(event.position.x);
}

void Slider::OnMouseMove(const MouseEvent& event) {
    if (!m_Dragging || m_ReadOnly) {
        return;
    }
    UpdateValueFromMouseX(event.position.x);
}

void Slider::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Dragging) {
        m_Dragging = false;
        if (m_OnValueCommitted) {
            m_OnValueCommitted(m_Value);
        }
        InvalidatePaint();
    }
}

void Slider::OnKeyDown(const KeyEvent& event) {
    if (!m_Focused || m_ReadOnly) {
        return;
    }
    double delta = 0.0;
    const double span = m_MaxValue - m_MinValue;
    const double stepVal = m_Step > 0.0 ? m_Step : span * 0.01;

    if (event.key == we::platform::KeyCode::Left || event.key == we::platform::KeyCode::Down) {
        delta = -stepVal;
    } else if (event.key == we::platform::KeyCode::Right || event.key == we::platform::KeyCode::Up) {
        delta = stepVal;
    } else if (event.key == we::platform::KeyCode::PageDown) {
        delta = -stepVal * 10.0;
    } else if (event.key == we::platform::KeyCode::PageUp) {
        delta = stepVal * 10.0;
    } else if (event.key == we::platform::KeyCode::Home) {
        SetValue(m_MinValue);
        if (m_OnValueCommitted) m_OnValueCommitted(m_Value);
        return;
    } else if (event.key == we::platform::KeyCode::End) {
        SetValue(m_MaxValue);
        if (m_OnValueCommitted) m_OnValueCommitted(m_Value);
        return;
    }

    if (delta != 0.0) {
        SetValue(m_Value + delta);
        if (m_OnValueCommitted) {
            m_OnValueCommitted(m_Value);
        }
    }
}

void Slider::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }

    const float scale = DPIContext::GetScale();
    const float trackH = 4.0f * scale;
    const float thumbRadius = 6.0f * scale;
    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;

    // Outer input frame for control container background
    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, m_ReadOnly, m_Focused, false };
    ControlChrome::PaintInputFrame(context, m_Geometry, state);

    // Track Geometry inside m_Geometry
    const float padH = 6.0f * scale;
    const float trackX = m_Geometry.x + padH;
    const float trackW = std::max(1.0f, m_Geometry.width - padH * 2.0f);
    const Rect trackRect{ trackX, centerY - trackH * 0.5f, trackW, trackH };

    // Draw Track Trough
    const Color troughColor = ThemeColor(ColorToken::InputBackground);
    context.DrawRoundedRect(trackRect, troughColor, trackH * 0.5f);

    // Draw Active Fill
    const double span = (m_MaxValue > m_MinValue) ? (m_MaxValue - m_MinValue) : 1.0;
    const float fillNorm = static_cast<float>(std::clamp((m_Value - m_MinValue) / span, 0.0, 1.0));
    const float fillW = trackW * fillNorm;
    if (fillW > 0.0f) {
        const Rect fillRect{ trackX, centerY - trackH * 0.5f, fillW, trackH };
        const Color accentColor = ThemeColor(ColorToken::AccentPrimary);
        context.DrawRoundedRect(fillRect, accentColor, trackH * 0.5f);
    }

    // Draw Thumb Handle
    const float thumbX = trackX + fillW;
    const Rect thumbRect{ thumbX - thumbRadius, centerY - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f };

    const Color thumbColor = m_Dragging ? ThemeColor(ColorToken::AccentHover)
                                       : (m_Hovered ? ThemeColor(ColorToken::TextPrimary) : ThemeColor(ColorToken::TextSecondary));
    context.DrawRoundedRect(thumbRect, thumbColor, thumbRadius);

    if (m_Focused) {
        context.DrawRoundedRectOutline(thumbRect, ThemeColor(ColorToken::AccentPrimary), 1.5f * scale, thumbRadius);
    }

    // Value Text Overlay (if enabled)
    if (m_ShowValueText) {
        const std::string text = FormatValueString();
        const float fontSize = ResolveMetric(MetricToken::TextSizeProperty);
        const float textY = LayoutMetrics::AlignTextTopY(m_Geometry, fontSize);

        const Rect clipR{ m_Geometry.x + 2.0f, m_Geometry.y, std::max(0.0f, m_Geometry.width - 4.0f), m_Geometry.height };
        context.PushClipRect(clipR);
        context.DrawText(
            text,
            Point{ m_Geometry.x + m_Geometry.width - padH - TextMetrics::MeasureWidth(text, fontSize), textY },
            ThemeColor(ColorToken::TextPrimary),
            fontSize);
        context.PopClipRect();
    }
}

} // namespace we::runtime::kindui
