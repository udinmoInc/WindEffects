#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Input/InputEvents.h"

#include <array>
#include <memory>

namespace we::editor::panels {

/// Fixed vertical regions inside a dock panel body (below the tab strip / floating header).
enum class PanelBodyRegion : std::uint8_t {
    ModeTabs,
    Search,
    Toolbar,
    ColumnHeader,
    Content,
    Footer,
    Count
};

/// Shared vertical stack: ModeTabs → Search → Toolbar → ColumnHeader → Content → Footer.
/// Fixed regions use intrinsic DPI-aware heights; Content receives all remaining space.
class UIFRAMEWORK_API PanelBodyLayout : public we::runtime::kindui::Widget {
public:
    PanelBodyLayout();

    void SetRegion(PanelBodyRegion region, const std::shared_ptr<we::runtime::kindui::Widget>& widget);
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> GetRegion(PanelBodyRegion region) const;
    [[nodiscard]] we::runtime::kindui::Rect GetRegionRect(PanelBodyRegion region) const;

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseWheel(const we::runtime::kindui::MouseEvent& event) override;

    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> HitTestPoint(
        const we::runtime::kindui::Point& pos,
        const we::runtime::kindui::Rect* clip = nullptr) override;
    [[nodiscard]] bool IsInteractiveContainer() const override { return true; }

    void ClearRegions();

private:
    struct RegionSlot {
        std::shared_ptr<we::runtime::kindui::Widget> widget;
        we::runtime::kindui::Rect geometry;
    };

    [[nodiscard]] float IntrinsicRegionHeight(PanelBodyRegion region) const;
    void ArrangeFixedRegion(PanelBodyRegion region, float& currentY, const we::runtime::kindui::Rect& allottedRect);
    void RoutePointer(
        const we::runtime::kindui::MouseEvent& event,
        void (we::runtime::kindui::Widget::*handler)(const we::runtime::kindui::MouseEvent&));

    std::array<RegionSlot, static_cast<size_t>(PanelBodyRegion::Count)> m_Regions{};
    we::runtime::kindui::Rect m_ContentClipRect;
};

} // namespace we::editor::panels
