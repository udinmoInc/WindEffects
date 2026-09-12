// ==============================================================================
// WindEffects — KindUI — PropertyResetButton
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"
#include <functional>

namespace we::runtime::kindui {

/// Reusable reset to default button and override indicator dot control.
class KINDUI_API PropertyResetButton : public Widget {
public:
    explicit PropertyResetButton(bool isModified = false);
    ~PropertyResetButton() override;

    void SetModified(bool modified);
    [[nodiscard]] bool IsModified() const { return m_IsModified; }

    void SetOnResetClicked(std::function<void()> cb);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnHoverLost() override;

private:
    bool m_IsModified = false;
    bool m_Hovered = false;
    std::function<void()> m_OnResetClicked;
};

} // namespace we::runtime::kindui
