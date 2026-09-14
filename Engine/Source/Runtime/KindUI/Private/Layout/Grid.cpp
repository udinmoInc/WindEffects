// ==============================================================================
// WindEffects — KindUI — Grid
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/Grid.h"

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

void Grid::ResolveTracks(
    const std::vector<GridTrackSize>& tracks,
    float available,
    float gap,
    bool /*measuringColumns*/,
    std::vector<float>& sizes) const
{
    const int n = static_cast<int>(tracks.size());
    sizes.assign(static_cast<size_t>(n), 0.0f);
    if (n == 0) return;

    const float totalGap = n > 1 ? gap * static_cast<float>(n - 1) : 0.0f;
    float remaining = std::max(0.0f, available - totalGap);
    float totalFr = 0.0f;

    for (int i = 0; i < n; ++i) {
        if (tracks[i].kind == GridTrackSizeKind::Fixed) {
            sizes[static_cast<size_t>(i)] = tracks[i].value;
            remaining -= sizes[static_cast<size_t>(i)];
        } else if (tracks[i].kind == GridTrackSizeKind::Fr) {
            totalFr += tracks[i].value;
        } else {
            sizes[static_cast<size_t>(i)] = 0.0f;
        }
    }

    remaining = std::max(0.0f, remaining);

    int autoCount = 0;
    for (const auto& t : tracks) {
        if (t.kind == GridTrackSizeKind::Auto) ++autoCount;
    }
    if (autoCount > 0 && remaining > 0.0f) {
        const float autoBudget = totalFr > 0.0f ? remaining * 0.0f : remaining;
        const float each = autoCount > 0 ? autoBudget / static_cast<float>(autoCount) : 0.0f;
        for (int i = 0; i < n; ++i) {
            if (tracks[i].kind == GridTrackSizeKind::Auto) {
                sizes[static_cast<size_t>(i)] = each;
                remaining -= each;
            }
        }
    }

    remaining = std::max(0.0f, remaining);
    if (totalFr > 0.0f) {
        for (int i = 0; i < n; ++i) {
            if (tracks[i].kind == GridTrackSizeKind::Fr) {
                sizes[static_cast<size_t>(i)] = remaining * (tracks[i].value / totalFr);
            }
        }
    }
}

Size Grid::Measure(const Size& availableSize) {
    if (CanSkipMeasure(availableSize)) {
        return m_DesiredSize;
    }

    const float padW = m_Padding.left + m_Padding.right;
    const float padH = m_Padding.top + m_Padding.bottom;
    const Size content{
        std::max(0.0f, availableSize.width - padW),
        std::max(0.0f, availableSize.height - padH)
    };

    for (const auto& child : m_Children) {
        if (child && child->IsVisible()) {
            // Available size can change without dirty bits — always measure visible children.
            (void)MeasureChild(child, content);
        }
    }

    ResolveTracks(m_Columns, content.width, m_ColumnGap, true, m_ColSizes);
    ResolveTracks(m_Rows, content.height, m_RowGap, false, m_RowSizes);

    float w = 0.0f;
    float h = 0.0f;
    for (float s : m_ColSizes) w += s;
    for (float s : m_RowSizes) h += s;
    if (m_ColSizes.size() > 1) w += m_ColumnGap * static_cast<float>(m_ColSizes.size() - 1);
    if (m_RowSizes.size() > 1) h += m_RowGap * static_cast<float>(m_RowSizes.size() - 1);

    m_DesiredSize = { w + padW, h + padH };
    NoteMeasureCache(availableSize);
    return m_DesiredSize;
}

void Grid::Arrange(const Rect& allottedRect) {
    if (CanSkipArrange(allottedRect)) {
        return;
    }

    CommitGeometry(allottedRect);
    ClearLayoutDirty();

    const float padW = m_Padding.left + m_Padding.right;
    const float padH = m_Padding.top + m_Padding.bottom;
    const float contentW = std::max(0.0f, allottedRect.width - padW);
    const float contentH = std::max(0.0f, allottedRect.height - padH);

    ResolveTracks(m_Columns, contentW, m_ColumnGap, true, m_ColSizes);
    ResolveTracks(m_Rows, contentH, m_RowGap, false, m_RowSizes);

    m_ColOffsets.resize(m_ColSizes.size());
    m_RowOffsets.resize(m_RowSizes.size());
    float x = allottedRect.x + m_Padding.left;
    for (size_t i = 0; i < m_ColSizes.size(); ++i) {
        m_ColOffsets[i] = x;
        x += m_ColSizes[i] + (i + 1 < m_ColSizes.size() ? m_ColumnGap : 0.0f);
    }
    float y = allottedRect.y + m_Padding.top;
    for (size_t i = 0; i < m_RowSizes.size(); ++i) {
        m_RowOffsets[i] = y;
        y += m_RowSizes[i] + (i + 1 < m_RowSizes.size() ? m_RowGap : 0.0f);
    }

    auto placeOf = [&](const std::shared_ptr<Widget>& child) -> Placement {
        for (const auto& p : m_Placements) {
            if (p.widget.lock() == child) return p;
        }
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
        if (p.column >= static_cast<int>(m_ColSizes.size()) || p.row >= static_cast<int>(m_RowSizes.size())) {
            continue;
        }

        const int colEnd = std::min(static_cast<int>(m_ColSizes.size()), p.column + p.colSpan);
        const int rowEnd = std::min(static_cast<int>(m_RowSizes.size()), p.row + p.rowSpan);

        float w = 0.0f;
        for (int c = p.column; c < colEnd; ++c) {
            w += m_ColSizes[static_cast<size_t>(c)];
            if (c + 1 < colEnd) w += m_ColumnGap;
        }
        float h = 0.0f;
        for (int r = p.row; r < rowEnd; ++r) {
            h += m_RowSizes[static_cast<size_t>(r)];
            if (r + 1 < rowEnd) h += m_RowGap;
        }

        const Margin& m = child->GetMargin();
        Rect cell{
            m_ColOffsets[static_cast<size_t>(p.column)] + m.left,
            m_RowOffsets[static_cast<size_t>(p.row)] + m.top,
            std::max(0.0f, w - m.left - m.right),
            std::max(0.0f, h - m.top - m.bottom)
        };
        ArrangeChild(child, cell);
    }
}

void Grid::Paint(PaintContext& context) {
    ClearPaintDirty();
    if (m_HasBackground) {
        context.DrawRect(m_Geometry, m_Background);
    }
    PaintVisibleChildren(context);
}

} // namespace we::runtime::kindui
