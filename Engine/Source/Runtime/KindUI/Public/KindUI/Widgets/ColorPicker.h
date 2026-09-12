// ==============================================================================
// WindEffects — KindUI — ColorPicker
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Types.h"
#include <string>
#include <functional>

namespace we::runtime::kindui {

class KINDUI_API ColorPicker : public Widget {
public:
    using ColorChangedCallback = std::function<void(const Color&)>;

    ColorPicker();
    explicit ColorPicker(const Color& initialColor);
    virtual ~ColorPicker() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    [[nodiscard]] Color GetColor() const { return m_Color; }
    void SetColor(const Color& color);
    void SetOnColorChanged(ColorChangedCallback callback) { m_OnColorChanged = std::move(callback); }

private:
    [[nodiscard]] Rect GetSwatchRect() const;
    [[nodiscard]] bool IsInSwatch(const Point& pos) const;

    Color m_Color;
    bool m_IsOpen = false;
    bool m_Hovered = false;
    float m_HoverAnim = 0.0f;
    ColorChangedCallback m_OnColorChanged;
};

} // namespace we::runtime::kindui