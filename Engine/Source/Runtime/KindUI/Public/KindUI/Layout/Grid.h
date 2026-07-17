#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"

#include <string>
#include <vector>

namespace we::runtime::kindui {

enum class GridTrackSizeKind { Fixed, Fr, Auto };

struct KINDUI_API GridTrackSize {
    GridTrackSizeKind kind = GridTrackSizeKind::Auto;
    float value = 0.0f; // px for Fixed, fraction for Fr

    static GridTrackSize Fixed(float px) { return { GridTrackSizeKind::Fixed, px }; }
    static GridTrackSize Fr(float fr) { return { GridTrackSizeKind::Fr, fr }; }
    static GridTrackSize Auto() { return { GridTrackSizeKind::Auto, 0.0f }; }
};

/// CSS-Grid-inspired container.
class KINDUI_API Grid : public Widget {
public:
    Grid() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    Grid& Columns(std::vector<GridTrackSize> tracks) { m_Columns = std::move(tracks); InvalidateLayout(); return *this; }
    Grid& Rows(std::vector<GridTrackSize> tracks) { m_Rows = std::move(tracks); InvalidateLayout(); return *this; }
    Grid& Gap(float columnGap, float rowGap) {
        m_ColumnGap = columnGap;
        m_RowGap = rowGap;
        InvalidateLayout();
        return *this;
    }
    Grid& Gap(float gap) { return Gap(gap, gap); }
    Grid& Padding(const Margin& p) { m_Padding = p; InvalidateLayout(); return *this; }
    Grid& Background(const Color& c) { m_Background = c; m_HasBackground = true; InvalidatePaint(); return *this; }

    void SetChildPlacement(const std::shared_ptr<Widget>& child, int column, int row, int colSpan = 1, int rowSpan = 1);

private:
    struct Placement {
        std::weak_ptr<Widget> widget;
        int column = 0;
        int row = 0;
        int colSpan = 1;
        int rowSpan = 1;
    };

    std::vector<float> ResolveTracks(
        const std::vector<GridTrackSize>& tracks,
        float available,
        float gap,
        bool measuringColumns) const;

    std::vector<GridTrackSize> m_Columns{ GridTrackSize::Fr(1.0f) };
    std::vector<GridTrackSize> m_Rows{ GridTrackSize::Auto() };
    float m_ColumnGap = 0.0f;
    float m_RowGap = 0.0f;
    Margin m_Padding{};
    Color m_Background{};
    bool m_HasBackground = false;
    std::vector<Placement> m_Placements;
};

[[nodiscard]] KINDUI_API std::shared_ptr<Grid> MakeGrid();

} // namespace we::runtime::kindui
