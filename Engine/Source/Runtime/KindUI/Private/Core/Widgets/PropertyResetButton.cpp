// ==============================================================================
// WindEffects — KindUI — PropertyResetButton
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/PropertyResetButton.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>

namespace we::runtime::kindui {

PropertyResetButton::PropertyResetButton(bool isModified)
    : m_IsModified(isModified) {}

PropertyResetButton::~PropertyResetButton() = default;

void PropertyResetButton::SetModified(bool modified) {
    if (m_IsModified != modified) {
        m_IsModified = modified;
        InvalidatePaint();
    }
}

void PropertyResetButton::SetOnResetClicked(std::function<void()> cb) {
    m_OnResetClicked = std::move(cb);
}

Size PropertyResetButton::Measure(const Size& availableSize) {
    (void)availableSize;
    m_DesiredSize = Size{ 16.0f, 16.0f };
    return m_DesiredSize;
}

void PropertyResetButton::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void PropertyResetButton::Paint(PaintContext& context) {
    if (!m_IsModified) return;

    if (m_Hovered) {
        IconPainter::Draw(context, WindIcons::Undo16, m_Geometry, ResolveColor(ColorToken::IconPrimary));
    } else {
        const float dotRadius = 2.5f;
        const Point center{ m_Geometry.x + m_Geometry.width * 0.5f, m_Geometry.y + m_Geometry.height * 0.5f };
        const Rect dotRect{ center.x - dotRadius, center.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f };
        context.DrawRoundedRect(dotRect, ResolveColor(ColorToken::AccentOrange), dotRadius);
    }
}

void PropertyResetButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_IsModified) {
        if (m_OnResetClicked) {
            m_OnResetClicked();
        }
    }
}

void PropertyResetButton::OnMouseMove(const MouseEvent& event) {
    (void)event;
    if (!m_Hovered) {
        m_Hovered = true;
        InvalidatePaint();
    }
}

void PropertyResetButton::OnHoverLost() {
    if (m_Hovered) {
        m_Hovered = false;
        InvalidatePaint();
    }
}

} // namespace we::runtime::kindui
