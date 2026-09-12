// ==============================================================================
// WindEffects — KindUI — PropertyRowLayout
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include <functional>
#include <memory>
#include <string>

namespace we::runtime::kindui {

/// Reusable 2-column property row layout hosting label, splitter, value widget, and reset indicator.
class KINDUI_API PropertyRowLayout : public Widget {
public:
    PropertyRowLayout(std::string label, std::shared_ptr<Widget> valueWidget, float labelColumnRatio = 0.40f);
    ~PropertyRowLayout() override;

    void SetLabel(std::string label);
    void SetValueWidget(std::shared_ptr<Widget> valueWidget);
    void SetLabelColumnRatio(float ratio);

    void SetModified(bool modified);
    [[nodiscard]] bool IsModified() const { return m_IsModified; }

    void SetReadOnly(bool readOnly);

    void SetOnResetClicked(std::function<void()> cb);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnHoverLost() override;
    void OnMouseDown(const MouseEvent& event) override;

private:
    std::string m_Label;
    std::shared_ptr<Widget> m_ValueWidget;
    float m_Ratio = 0.40f;
    bool m_IsModified = false;
    bool m_Hovered = false;
    bool m_ResetHovered = false;
    std::function<void()> m_OnResetClicked;
};

} // namespace we::runtime::kindui
