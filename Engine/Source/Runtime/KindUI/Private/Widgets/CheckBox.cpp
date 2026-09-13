// ==============================================================================
// WindEffects — KindUI — CheckBox
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/CheckBox.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"


namespace we::runtime::kindui {

CheckBox::CheckBox(const std::string& label, bool initialState)
    : m_Label(label)
    , m_Checked(initialState)
    , m_Style(TextStyle::Body())
{
}

Size CheckBox::Measure(const Size& availableSize) {
    (void)availableSize;
    if (!m_BoxStyleCacheValid || m_NeedsStyle) {
        m_CachedBoxStyle = ThemeManager::Get().Resolve(StyleRole::Checkbox);
        m_BoxStyleCacheValid = true;
        ClearStyleDirty();
    }
    m_BoxSize = m_CachedBoxStyle.height > 0.0f ? m_CachedBoxStyle.height : 14.0f;
    const float textWidth = TextMetrics::MeasureWidth(m_Label, m_Style.size, m_Style.bold);
    const float gap = ResolveMetric(MetricToken::Space2);
    m_DesiredSize = Size{ m_BoxSize + gap + textWidth, std::max(m_BoxSize, m_Style.size + 4.0f) };
    return m_DesiredSize;
}

void CheckBox::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void CheckBox::Tick(float deltaTime) {
    (void)deltaTime;
    const float targetHover = m_Hovered ? 1.0f : 0.0f;
    m_HoverAnim = Animator::Damp(m_HoverAnim, targetHover, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}

void CheckBox::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }

    const Rect boxRect{
        m_Geometry.x,
        m_Geometry.y + (m_Geometry.height - m_BoxSize) * 0.5f,
        m_BoxSize,
        m_BoxSize
    };
    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, m_Checked, m_Focused, false };
    ControlChrome::PaintCheckbox(context, boxRect, m_Checked, state);

    const float gap = ResolveMetric(MetricToken::Space2);
    context.DrawText(
        m_Label,
        Point{ m_Geometry.x + m_BoxSize + gap, m_Geometry.y + (m_Geometry.height - m_Style.size) * 0.5f },
        m_Style.color,
        m_Style.size,
        m_Style.bold,
        m_Style.italic);
}

void CheckBox::OnMouseDown(const MouseEvent& event) {
    if (!m_Visible || !IsEnabled()) {
        return;
    }
    if (m_Geometry.Contains(event.position) && event.button == MouseButton::Left) {
        m_Checked = !m_Checked;
        InvalidatePaint();
        if (m_OnChanged) {
            m_OnChanged(m_Checked);
        }
    }
}

} // namespace we::runtime::kindui
