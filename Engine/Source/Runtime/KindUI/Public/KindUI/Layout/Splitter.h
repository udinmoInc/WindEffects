#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Widget.h"

namespace we::runtime::kindui {

enum class Orientation {
    Horizontal,
    Vertical
};

class KINDUI_API Splitter : public Widget {
public:
    enum class ResizeMode { Ratio, FixedFirst };

    Splitter(Orientation orientation, float initialRatio = 0.5f);
    virtual ~Splitter() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;

    void SetFirstChild(const std::shared_ptr<Widget>& child);
    void SetSecondChild(const std::shared_ptr<Widget>& child);

    void SetSplitRatio(float ratio);
    [[nodiscard]] float GetSplitRatio() const { return m_SplitRatio; }
    
    void SetFixedFirstWidth(float width);
    [[nodiscard]] float GetFixedFirstWidth() const { return m_FixedFirstWidth; }
    
    void SetResizeMode(ResizeMode mode);
    [[nodiscard]] ResizeMode GetResizeMode() const { return m_ResizeMode; }

    void SetPanelGapEnabled(bool enabled) { m_PanelGapEnabled = enabled; }
    [[nodiscard]] bool IsPanelGapEnabled() const { return m_PanelGapEnabled; }

    /// Device-pixel minimum pane sizes (already DPI-scaled by caller).
    void SetMinPaneSizes(float minFirstPx, float minSecondPx);
    [[nodiscard]] float GetMinFirstSize() const { return m_MinFirstPx; }
    [[nodiscard]] float GetMinSecondSize() const { return m_MinSecondPx; }

    void SetSlotId(std::string id) { m_SlotId = std::move(id); }
    [[nodiscard]] const std::string& GetSlotId() const { return m_SlotId; }

private:
    Rect GetSplitterBarRect() const;
    Rect GetSplitterHitRect() const; // Wider area for grabbing
    [[nodiscard]] float GetEffectiveBarThickness() const;
    void ClampSplitToMins(float availMain, float barThickness);
    void SplitAvailable(float availMain, float barThickness, float& first, float& second) const;

    Orientation m_Orientation;
    float m_SplitRatio = 0.5f;
    float m_FixedFirstWidth = 280.0f;
    ResizeMode m_ResizeMode = ResizeMode::Ratio;
    float m_BarThicknessLogical = 2.0f;
    float m_HitThicknessLogical = 8.0f;
    float m_MinFirstPx = 80.0f;
    float m_MinSecondPx = 80.0f;
    bool m_PanelGapEnabled = false;
    bool m_Dragging = false;
    bool m_Hovered = false;
#pragma warning(push)
#pragma warning(disable: 4251)
    std::string m_SlotId;
    std::shared_ptr<Widget> m_FirstChild;
    std::shared_ptr<Widget> m_SecondChild;
#pragma warning(pop)
};

} // namespace we::runtime::kindui
