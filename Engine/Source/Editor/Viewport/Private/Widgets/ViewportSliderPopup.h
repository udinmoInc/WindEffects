#pragma once

#include "KindUI/Core/Widget.h"
#include <functional>
#include <string>

namespace we::programs::editor {

class ViewportSliderPopup : public we::runtime::kindui::Widget {
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

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;

private:
    float ValueFromNormalized(float t) const;
    float NormalizedFromValue(float value) const;
    void SetValueFromMouseX(float mouseX);
    we::runtime::kindui::Rect SliderTrackRect() const;

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
