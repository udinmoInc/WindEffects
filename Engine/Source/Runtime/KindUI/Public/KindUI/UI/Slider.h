// ==============================================================================
// WindEffects — KindUI — Slider
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Theme/ResolvedStyle.h"
#include <functional>
#include <string>

namespace we::runtime::kindui {

class KINDUI_API Slider : public Widget {
public:
    explicit Slider(
        double initialValue = 0.0,
        double minValue = 0.0,
        double maxValue = 1.0,
        double step = 0.01);
    ~Slider() override = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;

    [[nodiscard]] bool IsFocusable() const override { return true; }
    [[nodiscard]] bool ShowsPointerCursor(const Point& position) const override {
        return m_Geometry.Contains(position);
    }

    [[nodiscard]] double GetValue() const { return m_Value; }
    void SetValue(double val);

    [[nodiscard]] double GetMinValue() const { return m_MinValue; }
    [[nodiscard]] double GetMaxValue() const { return m_MaxValue; }
    void SetRange(double minVal, double maxVal);

    [[nodiscard]] double GetStep() const { return m_Step; }
    void SetStep(double step);

    [[nodiscard]] bool ShowsValueText() const { return m_ShowValueText; }
    void SetShowValueText(bool show) {
        if (m_ShowValueText != show) {
            m_ShowValueText = show;
            InvalidatePaint();
        }
    }

    void SetFormatString(const std::string& fmt) {
        m_FormatString = fmt;
        InvalidatePaint();
    }

    void SetOnValueChanged(std::function<void(double)> callback) {
        m_OnValueChanged = std::move(callback);
    }

    void SetOnValueCommitted(std::function<void(double)> callback) {
        m_OnValueCommitted = std::move(callback);
    }

private:
    void UpdateValueFromMouseX(float mouseX);
    [[nodiscard]] double ClampAndSnapValue(double val) const;
    [[nodiscard]] std::string FormatValueString() const;

    double m_Value = 0.0;
    double m_MinValue = 0.0;
    double m_MaxValue = 1.0;
    double m_Step = 0.01;
    bool m_ShowValueText = true;
    std::string m_FormatString = "%.2f";

    std::function<void(double)> m_OnValueChanged;
    std::function<void(double)> m_OnValueCommitted;

    bool m_Dragging = false;
    float m_HoverAnim = 0.0f;
    float m_FocusAnim = 0.0f;

    ResolvedStyle m_CachedStyle;
    bool m_StyleCacheValid = false;
};

} // namespace we::runtime::kindui
