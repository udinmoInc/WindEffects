#include "KindUI/Layout/Grid.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {

std::shared_ptr<Grid> MakeGrid() {
    return std::make_shared<Grid>();
}

void Grid::SetChildPlacement(const std::shared_ptr<Widget>& child, int column, int row, int colSpan, int rowSpan) {
    if (!child) return;
    for (auto& p : m_Placements) {
        if (p.widget.lock() == child) {
            p.column = column;
            p.row = row;
            p.colSpan = std::max(1, colSpan);
            p.rowSpan = std::max(1, rowSpan);
            InvalidateLayout();
            return;
        }
    }
    m_Placements.push_back({ child, column, row, std::max(1, colSpan), std::max(1, rowSpan) });
    InvalidateLayout();
}

std::vector<float> Grid::ResolveTracks(
    const std::vector<GridTrackSize>& tracks,
    float available,
    float gap,
    bool /*measuringColumns*/) const
{
    const int n = static_cast<int>(tracks.size());
    std::vector<float> sizes(n, 0.0f);
    if (n == 0) return sizes;

    const float totalGap = n > 1 ? gap * static_cast<float>(n - 1) : 0.0f;
    float remaining = std::max(0.0f, available - totalGap);
    float totalFr = 0.0f;

    for (int i = 0; i < n; ++i) {
        if (tracks[i].kind == GridTrackSizeKind::Fixed) {
            sizes[i] = tracks[i].value;
            remaining -= sizes[i];
        } else if (tracks[i].kind == GridTrackSizeKind::Fr) {
            totalFr += tracks[i].value;
        } else {
            // Auto: use equal share of remaining for measure fallback
            sizes[i] = 0.0f;
        }
    }

    remaining = std::max(0.0f, remaining);

    // Give Auto tracks a fair share of remaining before fr
    int autoCount = 0;
    for (const auto& t : tracks) {
        if (t.kind == GridTrackSizeKind::Auto) ++autoCount;
    }
    if (autoCount > 0 && remaining > 0.0f) {
        const float autoShare = remaining / static_cast<float>(autoCount + (totalFr > 0.0f ? 0 : 0));
        // Prefer leaving free space for fr tracks when present
        const float autoBudget = totalFr > 0.0f ? remaining * 0.0f : remaining;
        const float each = autoCount > 0 ? autoBudget / static_cast<float>(autoCount) : 0.0f;
        for (int i = 0; i < n; ++i) {
            if (tracks[i].kind == GridTrackSizeKind::Auto) {
                sizes[i] = each;
                remaining -= each;
            }
        }
    }

    remaining = std::max(0.0f, remaining);
    if (totalFr > 0.0f) {
        for (int i = 0; i < n; ++i) {
            if (tracks[i].kind == GridTrackSizeKind::Fr) {
                sizes[i] = remaining * (tracks[i].value / totalFr);
            }
        }
    }

    return sizes;
}

Size Grid::Measure(const Size& availableSize) {
    const float padW = m_Padding.left + m_Padding.right;
    const float padH = m_Padding.top + m_Padding.bottom;
    const Size content{
        std::max(0.0f, availableSize.width - padW),
        std::max(0.0f, availableSize.height - padH)
    };

    // Ensure every child is measured
    for (const auto& child : m_Children) {
        if (child && child->IsVisible()) {
            child->Measure(content);
        }
    }

    auto colSizes = ResolveTracks(m_Columns, content.width, m_ColumnGap, true);
    auto rowSizes = ResolveTracks(m_Rows, content.height, m_RowGap, false);

    float w = 0.0f;
    float h = 0.0f;
    for (float s : colSizes) w += s;
    for (float s : rowSizes) h += s;
    if (colSizes.size() > 1) w += m_ColumnGap * static_cast<float>(colSizes.size() - 1);
    if (rowSizes.size() > 1) h += m_RowGap * static_cast<float>(rowSizes.size() - 1);

    m_DesiredSize = { w + padW, h + padH };
    return m_DesiredSize;
}

void Grid::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    ClearLayoutDirty();

    const float padW = m_Padding.left + m_Padding.right;
    const float padH = m_Padding.top + m_Padding.bottom;
    const float contentW = std::max(0.0f, allottedRect.width - padW);
    const float contentH = std::max(0.0f, allottedRect.height - padH);

    auto colSizes = ResolveTracks(m_Columns, contentW, m_ColumnGap, true);
    auto rowSizes = ResolveTracks(m_Rows, contentH, m_RowGap, false);

    std::vector<float> colOffsets(colSizes.size(), 0.0f);
    std::vector<float> rowOffsets(rowSizes.size(), 0.0f);
    float x = allottedRect.x + m_Padding.left;
    for (size_t i = 0; i < colSizes.size(); ++i) {
        colOffsets[i] = x;
        x += colSizes[i] + (i + 1 < colSizes.size() ? m_ColumnGap : 0.0f);
    }
    float y = allottedRect.y + m_Padding.top;
    for (size_t i = 0; i < rowSizes.size(); ++i) {
        rowOffsets[i] = y;
        y += rowSizes[i] + (i + 1 < rowSizes.size() ? m_RowGap : 0.0f);
    }

    auto placeOf = [&](const std::shared_ptr<Widget>& child) -> Placement {
        for (const auto& p : m_Placements) {
            if (p.widget.lock() == child) return p;
        }
        // Default: flow into children order across columns
        const int index = [&]() {
            int i = 0;
            for (const auto& c : m_Children) {
                if (c == child) return i;
                if (c && c->IsVisible()) ++i;
            }
            return 0;
        }();
        const int cols = std::max(1, static_cast<int>(m_Columns.size()));
        Placement p;
        p.widget = child;
        p.column = index % cols;
        p.row = index / cols;
        p.colSpan = 1;
        p.rowSpan = 1;
        return p;
    };

    for (const auto& child : m_Children) {
        if (!child || !child->IsVisible()) continue;
        const Placement p = placeOf(child);
        if (p.column < 0 || p.row < 0) continue;
        if (p.column >= static_cast<int>(colSizes.size()) || p.row >= static_cast<int>(rowSizes.size())) {
            continue;
        }

        const int colEnd = std::min(static_cast<int>(colSizes.size()), p.column + p.colSpan);
        const int rowEnd = std::min(static_cast<int>(rowSizes.size()), p.row + p.rowSpan);

        float w = 0.0f;
        for (int c = p.column; c < colEnd; ++c) {
            w += colSizes[c];
            if (c + 1 < colEnd) w += m_ColumnGap;
        }
        float h = 0.0f;
        for (int r = p.row; r < rowEnd; ++r) {
            h += rowSizes[r];
            if (r + 1 < rowEnd) h += m_RowGap;
        }

        const Margin& m = child->GetMargin();
        Rect cell{
            colOffsets[p.column] + m.left,
            rowOffsets[p.row] + m.top,
            std::max(0.0f, w - m.left - m.right),
            std::max(0.0f, h - m.top - m.bottom)
        };
        child->Arrange(cell);
    }
}

void Grid::Paint(PaintContext& context) {
    ClearPaintDirty();
    if (m_HasBackground) {
        context.DrawRect(m_Geometry, m_Background);
    }
    for (auto& child : m_Children) {
        if (child && child->IsVisible()) {
            child->Paint(context);
        }
    }
}

} // namespace we::runtime::kindui
