// ==============================================================================
// WindEffects — KindUI — CheckBox
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Style.h"
#include <string>
#include <functional>

namespace we::runtime::kindui {

class KINDUI_API CheckBox : public Widget {
public:
    CheckBox(const std::string& label, bool initialState = false);
    virtual ~CheckBox() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    bool IsChecked() const { return m_Checked; }
    void SetChecked(bool checked) { m_Checked = checked; }

    void SetOnChanged(std::function<void(bool)> callback) { m_OnChanged = callback; }

private:
    std::string m_Label;
    bool m_Checked;
    std::function<void(bool)> m_OnChanged;
    TextStyle m_Style;
    float m_BoxSize = 14.0f;
    float m_HoverAnim = 0.0f;
};

} // namespace we::runtime::kindui
