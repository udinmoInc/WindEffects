#include "Widgets/ViewportSliderPopup.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include <algorithm>
#include <cmath>

namespace we::programs::editor {

namespace {
constexpr float kPopupWidth = 220.0f;
constexpr float kPopupHeight = 64.0f;
constexpr float kPadding = 10.0f;
constexpr float kSliderHeight = 6.0f;
constexpr float kThumbRadius = 6.0f;
constexpr float kSliderTop = 38.0f;
} // namespace

using we::UI::Color;
using we::UI::MouseButton;
using we::UI::MouseEvent;
using we::UI::PaintContext;
using we::UI::Point;
using we::UI::Rect;
using we::UI::Size;
using we::UI::Theme;

ViewportSliderPopup::ViewportSliderPopup(
    std::string title,
    float value,
    float minValue,
    float maxValue,
    bool useLogScale,
    ValueFormatter format,
    ValueSnapper snap,
    ValueChangedFn onChanged)
    : m_Title(std::move(title))
    , m_Value(value)
    , m_MinValue(minValue)
    , m_MaxValue(maxValue)
    , m_UseLogScale(useLogScale)
    , m_Format(std::move(format))
    , m_Snap(std::move(snap))
    , m_OnChanged(std::move(onChanged))
{
    if (m_Snap) {
        m_Value = m_Snap(m_Value);
    }
    m_Value = std::clamp(m_Value, m_MinValue, m_MaxValue);
}

Size ViewportSliderPopup::Measure(const Size& /*availableSize*/) {
    m_DesiredSize = Size{ kPopupWidth, kPopupHeight };
    return m_DesiredSize;
}

void ViewportSliderPopup::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

Rect ViewportSliderPopup::SliderTrackRect() const {
    return Rect{
        m_Geometry.x + kPadding,
        m_Geometry.y + kSliderTop,
        m_Geometry.width - 2.0f * kPadding,
        kSliderHeight
    };
}

float ViewportSliderPopup::NormalizedFromValue(float value) const {
    const float clamped = std::clamp(value, m_MinValue, m_MaxValue);
    if (m_UseLogScale) {
        const float logMin = std::log(m_MinValue);
        const float logMax = std::log(m_MaxValue);
        const float denom = std::max(logMax - logMin, 1e-6f);
        return (std::log(clamped) - logMin) / denom;
    }
    const float denom = std::max(m_MaxValue - m_MinValue, 1e-6f);
    return (clamped - m_MinValue) / denom;
}

float ViewportSliderPopup::ValueFromNormalized(float t) const {
    const float normalized = std::clamp(t, 0.0f, 1.0f);
    float value = m_MinValue;
    if (m_UseLogScale) {
        const float logMin = std::log(m_MinValue);
        const float logMax = std::log(m_MaxValue);
        value = std::exp(logMin + normalized * (logMax - logMin));
    } else {
        value = m_MinValue + normalized * (m_MaxValue - m_MinValue);
    }
    if (m_Snap) {
        value = m_Snap(value);
    }
    return std::clamp(value, m_MinValue, m_MaxValue);
}

void ViewportSliderPopup::SetValueFromMouseX(float mouseX) {
    const Rect track = SliderTrackRect();
    const float t = (mouseX - track.x) / std::max(track.width, 1.0f);
    const float next = ValueFromNormalized(t);
    if (std::abs(next - m_Value) < 1e-4f) {
        return;
    }
    m_Value = next;
    if (m_OnChanged) {
        m_OnChanged(m_Value);
    }
}

void ViewportSliderPopup::Paint(PaintContext& context) {
    context.DrawShadow(m_Geometry, Color{ 0.0f, 0.0f, 0.0f, 0.15f }, 4.0f, 12.0f);
    context.DrawRoundedRect(m_Geometry, Theme::Get().PanelBackground, 4.0f);
    context.DrawRoundedRectOutline(m_Geometry, Theme::Get().BorderDefault, 1.0f, 4.0f);

    context.DrawText(m_Title, Point{ m_Geometry.x + kPadding, m_Geometry.y + 8.0f },
        Theme::Get().TextSecondary, 10.0f);

    const std::string valueText = m_Format ? m_Format(m_Value) : std::to_string(m_Value);
    context.DrawText(valueText,
        Point{ m_Geometry.x + m_Geometry.width - kPadding - valueText.length() * 6.5f, m_Geometry.y + 8.0f },
        Theme::Get().TextPrimary, 11.0f);

    const Rect track = SliderTrackRect();
    context.DrawRoundedRect(track, Theme::Get().BorderDefault, 3.0f);

    const float thumbX = track.x + NormalizedFromValue(m_Value) * track.width;
    const Rect thumb{
        thumbX - kThumbRadius,
        track.y + track.height * 0.5f - kThumbRadius,
        kThumbRadius * 2.0f,
        kThumbRadius * 2.0f
    };
    context.DrawRoundedRect(thumb, Theme::Get().SelectedAccent, kThumbRadius);
    context.DrawRoundedRectOutline(thumb, Theme::Get().BorderDefault, 1.0f, kThumbRadius);
}

void ViewportSliderPopup::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const Rect track = SliderTrackRect();
    const Rect hit{
        track.x - kThumbRadius,
        track.y - kThumbRadius,
        track.width + 2.0f * kThumbRadius,
        track.height + 2.0f * kThumbRadius
    };
    if (!hit.Contains(event.position)) {
        return;
    }
    m_Dragging = true;
    SetValueFromMouseX(event.position.x);
}

void ViewportSliderPopup::OnMouseMove(const MouseEvent& event) {
    if (!m_Dragging) {
        return;
    }
    SetValueFromMouseX(event.position.x);
}

void ViewportSliderPopup::OnMouseUp(const MouseEvent& /*event*/) {
    m_Dragging = false;
}

bool ViewportSliderPopup::ShowsPointerCursor(const Point& position) const {
    const Rect track = SliderTrackRect();
    const Rect hit{
        track.x - kThumbRadius,
        track.y - kThumbRadius,
        track.width + 2.0f * kThumbRadius,
        track.height + 2.0f * kThumbRadius
    };
    return hit.Contains(position);
}

} // namespace we::programs::editor
