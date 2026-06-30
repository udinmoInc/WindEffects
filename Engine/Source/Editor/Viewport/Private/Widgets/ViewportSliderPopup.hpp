#pragma once

#include "Core/Widget.hpp"
#include <functional>
#include <string>

namespace we::programs::editor {

class ViewportSliderPopup : public we::UI::Widget {
public:
    using ValueFormatter = std::function<std::string(float)>;
    using ValueSnapper = std::function<float(float)>;
    using ValueChangedFn = std::function<void(float)>;

    ViewportSliderPopup(
        std::string title,
        float value,
        float minValue,
        float maxValue,
        bool useLogScale,
        ValueFormatter format,
        ValueSnapper snap,
        ValueChangedFn onChanged);

    we::UI::Size Measure(const we::UI::Size& availableSize) override;
    void Arrange(const we::UI::Rect& allottedRect) override;
    void Paint(we::UI::PaintContext& context) override;

    void OnMouseDown(const we::UI::MouseEvent& event) override;
    void OnMouseMove(const we::UI::MouseEvent& event) override;
    void OnMouseUp(const we::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::UI::Point& position) const override;

private:
    float ValueFromNormalized(float t) const;
    float NormalizedFromValue(float value) const;
    void SetValueFromMouseX(float mouseX);
    we::UI::Rect SliderTrackRect() const;

    std::string m_Title;
    float m_Value = 0.0f;
    float m_MinValue = 0.0f;
    float m_MaxValue = 1.0f;
    bool m_UseLogScale = false;
    bool m_Dragging = false;
    ValueFormatter m_Format;
    ValueSnapper m_Snap;
    ValueChangedFn m_OnChanged;
};

} // namespace we::programs::editor
