// ==============================================================================
// WindEffects — KindUI — PropertyRowLayout
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Layout/PropertyRowLayout.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PropertyPanelChrome.h"

namespace we::runtime::kindui {

PropertyRowLayout::PropertyRowLayout(std::string label, std::shared_ptr<Widget> valueWidget, float labelColumnRatio)
    : m_Label(std::move(label)), m_ValueWidget(std::move(valueWidget)), m_Ratio(labelColumnRatio) {
    if (m_ValueWidget) {
        AddChild(m_ValueWidget);
    }
}

PropertyRowLayout::~PropertyRowLayout() = default;

void PropertyRowLayout::SetLabel(std::string label) {
    if (m_Label != label) {
        m_Label = std::move(label);
        InvalidatePaint();
    }
}

void PropertyRowLayout::SetValueWidget(std::shared_ptr<Widget> valueWidget) {
    if (m_ValueWidget != valueWidget) {
        if (m_ValueWidget) {
            RemoveChild(m_ValueWidget);
        }
        m_ValueWidget = std::move(valueWidget);
        if (m_ValueWidget) {
            AddChild(m_ValueWidget);
        }
        InvalidateLayout();
        InvalidatePaint();
    }
}

void PropertyRowLayout::SetLabelColumnRatio(float ratio) {
    if (m_Ratio != ratio) {
        m_Ratio = ratio;
        InvalidateLayout();
        InvalidatePaint();
    }
}

void PropertyRowLayout::SetModified(bool modified) {
    if (m_IsModified != modified) {
        m_IsModified = modified;
        InvalidatePaint();
    }
}

void PropertyRowLayout::SetReadOnly(bool readOnly) {
    Widget::SetReadOnly(readOnly);
    if (m_ValueWidget) {
        m_ValueWidget->SetReadOnly(readOnly);
    }
}

void PropertyRowLayout::SetOnResetClicked(std::function<void()> cb) {
    m_OnResetClicked = std::move(cb);
}

Size PropertyRowLayout::Measure(const Size& availableSize) {
    const float h = LayoutMetrics::PropertyControlHeight();
    m_DesiredSize = Size{ availableSize.width, h };
    return m_DesiredSize;
}

void PropertyRowLayout::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const auto layout = PropertyPanelChrome::LayoutPropertyRow(allottedRect, 0, {}, false, m_Ratio);
    if (m_ValueWidget) {
        const auto controlRect = LayoutMetrics::LayoutPropertyControlInRow(layout.value);
        m_ValueWidget->Arrange(controlRect);
    }
}

void PropertyRowLayout::Paint(PaintContext& context) {
    const auto layout = PropertyPanelChrome::LayoutPropertyRow(m_Geometry, 0, {}, false, m_Ratio);
    PropertyPanelChrome::PaintPropertyRowBackground(context, m_Geometry, m_Hovered, false);
    PropertyPanelChrome::PaintPropertyRowLabel(context, layout.label, m_Label, false);
    if (m_ValueWidget) {
        m_ValueWidget->Paint(context);
    }
}

void PropertyRowLayout::OnMouseMove(const MouseEvent& event) {
    (void)event;
    if (!m_Hovered) {
        m_Hovered = true;
        InvalidatePaint();
    }
}

void PropertyRowLayout::OnHoverLost() {
    if (m_Hovered) {
        m_Hovered = false;
        m_ResetHovered = false;
        InvalidatePaint();
    }
}

void PropertyRowLayout::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_IsModified && m_ResetHovered) {
        if (m_OnResetClicked) {
            m_OnResetClicked();
        }
    }
}

} // namespace we::runtime::kindui
