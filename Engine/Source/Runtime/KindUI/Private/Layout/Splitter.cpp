// ==============================================================================
// WindEffects — KindUI — Splitter
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Layout/Splitter.h"
#include "KindUI/Layout/LayoutAssert.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Theming/StyleRole.h"
#include "Platform/Platform.h"
#include "Platform/Types.h"

#include <algorithm>
#include <atomic>
#include <cmath>

namespace we::runtime::kindui {
namespace {
std::atomic<int> g_SplitterDragCount{0};

void ComputePaneExtents(
    Splitter::ResizeMode mode,
    float fixedFirst,
    float fixedSecond,
    float availMain,
    float barThickness,
    bool firstVisible,
    bool secondVisible,
    float minFirstPx,
    float minSecondPx,
    float& splitRatio,
    float& first,
    float& second) {
    first = 0.0f;
    second = 0.0f;
    if (firstVisible && secondVisible) {
        if (mode == Splitter::ResizeMode::FixedFirst) {
            first = std::min(fixedFirst, std::max(0.0f, availMain - barThickness));
            second = std::max(0.0f, availMain - barThickness - first);
        } else if (mode == Splitter::ResizeMode::FixedSecond) {
            second = std::min(fixedSecond, std::max(0.0f, availMain - barThickness));
            first = std::max(0.0f, availMain - barThickness - second);
        } else {
            const float usable = std::max(0.0f, availMain - barThickness);
            float minFirst = minFirstPx;
            float minSecond = minSecondPx;
            if (usable > 0.0f && minFirst + minSecond > usable) {
                const float scale = usable / (minFirst + minSecond);
                minFirst *= scale;
                minSecond *= scale;
            }
            if (usable > 0.0f) {
                const float minRatio = minFirst / usable;
                const float maxRatio = 1.0f - (minSecond / usable);
                splitRatio = std::clamp(splitRatio, minRatio, std::max(minRatio, maxRatio));
                first = usable * splitRatio;
                second = usable - first;
            }
        }
    } else if (firstVisible) {
        first = availMain;
    } else if (secondVisible) {
        second = availMain;
    }
}
}

bool Splitter::AnySplitterDragging() {
    return g_SplitterDragCount.load(std::memory_order_relaxed) > 0;
}

void Splitter::ApplyResizeCursor(bool overResizeEdge) const {
    auto& platform = we::platform::Platform::Get();
    if (!overResizeEdge) {
        platform.SetSystemCursor(we::platform::SystemCursor::Arrow);
        return;
    }
    platform.SetSystemCursor(
        m_Orientation == Orientation::Horizontal
            ? we::platform::SystemCursor::SizeWE
            : we::platform::SystemCursor::SizeNS);
}

Splitter::Splitter(Orientation orientation, float initialRatio)
    : m_Orientation(orientation), m_SplitRatio(initialRatio) {
    const float defaultPane = ResolveMetric(MetricToken::PropertyLabelColumnWidth) * 2.0f;
    m_FixedFirstWidth = defaultPane;
    m_FixedSecondWidth = defaultPane;
    const float scale = DPIContext::GetScale();
    m_MinFirstPx = ResolveMetric(MetricToken::NavigationButtonSize) * 2.5f * scale;
    m_MinSecondPx = m_MinFirstPx;
    m_BarThicknessLogical = ResolveMetric(MetricToken::SplitterThickness);
    m_HitThicknessLogical = ResolveMetric(MetricToken::Space2);
}

void Splitter::SetFirstChild(const std::shared_ptr<Widget>& child) {
    if (m_FirstChild) RemoveChild(m_FirstChild);
    m_FirstChild = child;
    AddChild(child);
}

void Splitter::SetSecondChild(const std::shared_ptr<Widget>& child) {
    if (m_SecondChild) RemoveChild(m_SecondChild);
    m_SecondChild = child;
    AddChild(child);
}

void Splitter::SetSplitRatio(float ratio) {
    m_SplitRatio = std::clamp(ratio, 0.0f, 1.0f);
}

void Splitter::SetFixedFirstWidth(float width) {
    m_FixedFirstWidth = std::max(0.0f, width);
}

void Splitter::SetFixedSecondWidth(float width) {
    m_FixedSecondWidth = std::max(0.0f, width);
}

void Splitter::SetResizeMode(ResizeMode mode) {
    m_ResizeMode = mode;
}

void Splitter::SetMinPaneSizes(float minFirstPx, float minSecondPx) {
    m_MinFirstPx = std::max(0.0f, minFirstPx);
    m_MinSecondPx = std::max(0.0f, minSecondPx);
}

float Splitter::GetEffectiveBarThickness() const {
    const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
    const bool secondVisible = m_SecondChild && m_SecondChild->IsVisible();
    if (!firstVisible || !secondVisible) {
        return 0.0f;
    }
    const float scale = DPIContext::GetScale();
    if (m_PanelGapEnabled) {
        // Gap-cuts: one 1px splitter band between panels; workspace padding handles outer edges.
        if (ChromeSeparation::kGapCutsEnabled) {
            return ChromeSeparation::DockSplitterGapPx();
        }
        const float gapLogical = m_PanelGapLogical > 0.0f
            ? m_PanelGapLogical
            : ResolveMetric(MetricToken::DockPanelGap);
        return (std::max)(1.0f, gapLogical * scale);
    }
    return DPIContext::Snap(m_BarThicknessLogical * scale);
}

void Splitter::ClampSplitToMins(float availMain, float barThickness) {
    const float usable = std::max(0.0f, availMain - barThickness);
    if (usable <= 0.0f) {
        m_SplitRatio = 0.0f;
        return;
    }

    float minFirst = m_MinFirstPx;
    float minSecond = m_MinSecondPx;
    if (minFirst + minSecond > usable) {
        const float scale = usable / (minFirst + minSecond);
        minFirst *= scale;
        minSecond *= scale;
    }

    const float minRatio = minFirst / usable;
    const float maxRatio = 1.0f - (minSecond / usable);
    m_SplitRatio = std::clamp(m_SplitRatio, minRatio, std::max(minRatio, maxRatio));
}

void Splitter::SplitAvailable(float availMain, float barThickness, float& first, float& second) const {
    const float usable = std::max(0.0f, availMain - barThickness);
    first = usable * m_SplitRatio;
    second = usable - first;
}

Rect Splitter::GetSplitterBarRect() const {
    const float barThickness = GetEffectiveBarThickness();
    if (m_Orientation == Orientation::Horizontal) {
        float x;
        if (m_ResizeMode == ResizeMode::FixedFirst) {
            x = m_Geometry.x + m_FixedFirstWidth;
        } else if (m_ResizeMode == ResizeMode::FixedSecond) {
            x = m_Geometry.x + m_Geometry.width - barThickness - m_FixedSecondWidth;
        } else {
            x = m_Geometry.x + (m_Geometry.width - barThickness) * m_SplitRatio;
        }
        return Rect{ x, m_Geometry.y, barThickness, m_Geometry.height };
    }

    float y;
    if (m_ResizeMode == ResizeMode::FixedFirst) {
        y = m_Geometry.y + m_FixedFirstWidth;
    } else if (m_ResizeMode == ResizeMode::FixedSecond) {
        y = m_Geometry.y + m_Geometry.height - barThickness - m_FixedSecondWidth;
    } else {
        y = m_Geometry.y + (m_Geometry.height - barThickness) * m_SplitRatio;
    }
    return Rect{ m_Geometry.x, y, m_Geometry.width, barThickness };
}

Size Splitter::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    AssertNonNegativeSize(m_SlotId.empty() ? "Splitter" : m_SlotId, availableSize.width, availableSize.height);

    const float barThickness = GetEffectiveBarThickness();
    float availW = std::max(0.0f, availableSize.width);
    float availH = std::max(0.0f, availableSize.height);
    const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
    const bool secondVisible = m_SecondChild && m_SecondChild->IsVisible();

    if (m_Orientation == Orientation::Horizontal) {
        float w1 = 0.0f;
        float w2 = 0.0f;
        ComputePaneExtents(
            m_ResizeMode, m_FixedFirstWidth, m_FixedSecondWidth, availW, barThickness,
            firstVisible, secondVisible, m_MinFirstPx, m_MinSecondPx, m_SplitRatio, w1, w2);
        if (firstVisible) {
            m_FirstChild->Measure(Size{ w1, availH });
        }
        if (secondVisible) {
            m_SecondChild->Measure(Size{ w2, availH });
        }
    } else {
        float h1 = 0.0f;
        float h2 = 0.0f;
        ComputePaneExtents(
            m_ResizeMode, m_FixedFirstWidth, m_FixedSecondWidth, availH, barThickness,
            firstVisible, secondVisible, m_MinFirstPx, m_MinSecondPx, m_SplitRatio, h1, h2);
        if (firstVisible) {
            m_FirstChild->Measure(Size{ availW, h1 });
        }
        if (secondVisible) {
            m_SecondChild->Measure(Size{ availW, h2 });
        }
    }

    return m_DesiredSize;
}

void Splitter::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    AssertNonNegativeSize(m_SlotId.empty() ? "Splitter" : m_SlotId, allottedRect.width, allottedRect.height);

    const float barThickness = GetEffectiveBarThickness();
    float availW = std::max(0.0f, allottedRect.width);
    float availH = std::max(0.0f, allottedRect.height);
    const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
    const bool secondVisible = m_SecondChild && m_SecondChild->IsVisible();

    if (m_Orientation == Orientation::Horizontal) {
        float w1 = 0.0f;
        float w2 = 0.0f;
        ComputePaneExtents(
            m_ResizeMode, m_FixedFirstWidth, m_FixedSecondWidth, availW, barThickness,
            firstVisible, secondVisible, m_MinFirstPx, m_MinSecondPx, m_SplitRatio, w1, w2);
        const float barX = allottedRect.x + w1;

        m_FirstChildRect = {};
        m_SecondChildRect = {};
        if (firstVisible) {
            Rect firstRect = ClampRectToParent(
                Rect{
                    std::round(allottedRect.x),
                    std::round(allottedRect.y),
                    std::round(allottedRect.x + w1) - std::round(allottedRect.x),
                    std::round(allottedRect.y + availH) - std::round(allottedRect.y)
                },
                allottedRect);
            AssertLayoutRectValid("Splitter.first", firstRect, allottedRect);
            m_FirstChildRect = firstRect;
            m_FirstChild->Arrange(firstRect);
        }
        if (secondVisible) {
            const float secondX = (firstVisible && w1 > 0.0f) ? barX + barThickness : allottedRect.x;
            const float secondW = (firstVisible && secondVisible) ? w2 : availW;
            Rect secondRect = ClampRectToParent(
                Rect{
                    std::round(secondX),
                    std::round(allottedRect.y),
                    std::round(secondX + secondW) - std::round(secondX),
                    std::round(allottedRect.y + availH) - std::round(allottedRect.y)
                },
                allottedRect);
            AssertLayoutRectValid("Splitter.second", secondRect, allottedRect);
            m_SecondChildRect = secondRect;
            m_SecondChild->Arrange(secondRect);
        }

        if (firstVisible && secondVisible) {
            m_CachedBarRect = Rect{ barX, allottedRect.y, barThickness, availH };
        } else {
            m_CachedBarRect = {};
        }
        UpdateCachedBarHitRect();
    } else {
        float h1 = 0.0f;
        float h2 = 0.0f;
        ComputePaneExtents(
            m_ResizeMode, m_FixedFirstWidth, m_FixedSecondWidth, availH, barThickness,
            firstVisible, secondVisible, m_MinFirstPx, m_MinSecondPx, m_SplitRatio, h1, h2);
        const float barY = allottedRect.y + h1;

        m_FirstChildRect = {};
        m_SecondChildRect = {};
        if (firstVisible) {
            Rect firstRect = ClampRectToParent(
                Rect{
                    std::round(allottedRect.x),
                    std::round(allottedRect.y),
                    std::round(allottedRect.x + availW) - std::round(allottedRect.x),
                    std::round(allottedRect.y + h1) - std::round(allottedRect.y)
                },
                allottedRect);
            AssertLayoutRectValid("Splitter.first", firstRect, allottedRect);
            m_FirstChildRect = firstRect;
            m_FirstChild->Arrange(firstRect);
        }
        if (secondVisible) {
            const float secondY = (firstVisible && h1 > 0.0f) ? barY + barThickness : allottedRect.y;
            const float secondH = (firstVisible && secondVisible) ? h2 : availH;
            Rect secondRect = ClampRectToParent(
                Rect{
                    std::round(allottedRect.x),
                    std::round(secondY),
                    std::round(allottedRect.x + availW) - std::round(allottedRect.x),
                    std::round(secondY + secondH) - std::round(secondY)
                },
                allottedRect);
            AssertLayoutRectValid("Splitter.second", secondRect, allottedRect);
            m_SecondChildRect = secondRect;
            m_SecondChild->Arrange(secondRect);
        }

        if (firstVisible && secondVisible) {
            m_CachedBarRect = Rect{ allottedRect.x, barY, availW, barThickness };
        } else {
            m_CachedBarRect = {};
        }
        UpdateCachedBarHitRect();
    }
}

Rect Splitter::GetSplitterHitRect() const {
    return ComputeBarHitRect();
}

Rect Splitter::ComputeBarHitRect() const {
    if (!m_CachedBarHitRect.IsEmpty()) {
        return m_CachedBarHitRect;
    }

    const Rect barRect = m_CachedBarRect.IsEmpty() ? GetSplitterBarRect() : m_CachedBarRect;
    if (barRect.IsEmpty() && m_Geometry.IsEmpty()) {
        return {};
    }

    const float barThickness = m_Orientation == Orientation::Horizontal
        ? std::max(1.0f, barRect.width)
        : std::max(1.0f, barRect.height);
    const float scale = std::max(1.0f, DPIContext::GetScale());
    const float hitThickness = std::max(8.0f, DPIContext::Snap(m_HitThicknessLogical * scale));
    if (m_Orientation == Orientation::Horizontal) {
        const float padding = (hitThickness - barThickness) * 0.5f;
        const float height = std::max(1.0f, barRect.height > 0.0f ? barRect.height : m_Geometry.height);
        return Rect{ barRect.x - padding, barRect.y, hitThickness, height };
    }

    const float padding = (hitThickness - barThickness) * 0.5f;
    const float width = std::max(1.0f, barRect.width > 0.0f ? barRect.width : m_Geometry.width);
    return Rect{ barRect.x, barRect.y - padding, width, hitThickness };
}

void Splitter::UpdateCachedBarHitRect() {
    const Rect barRect = m_CachedBarRect.IsEmpty() ? GetSplitterBarRect() : m_CachedBarRect;
    if (barRect.IsEmpty() && m_Geometry.IsEmpty()) {
        m_CachedBarHitRect = {};
        return;
    }

    const float scale = std::max(1.0f, DPIContext::GetScale());
    const float hitThickness = std::max(8.0f, DPIContext::Snap(m_HitThicknessLogical * scale));

    if (m_PanelGapEnabled && ChromeSeparation::kGapCutsEnabled) {
        if (m_Orientation == Orientation::Horizontal) {
            const float boundaryX = !m_SecondChildRect.IsEmpty()
                ? m_SecondChildRect.x
                : (barRect.width > 0.0f ? barRect.x : barRect.x);
            const float height = std::max(
                1.0f,
                !m_FirstChildRect.IsEmpty() ? m_FirstChildRect.height : m_Geometry.height);
            m_CachedBarHitRect = Rect{ boundaryX - hitThickness * 0.5f, m_Geometry.y, hitThickness, height };
            return;
        }

        const float boundaryY = !m_SecondChildRect.IsEmpty()
            ? m_SecondChildRect.y
            : (barRect.height > 0.0f ? barRect.y : barRect.y);
        const float width = std::max(
            1.0f,
            !m_FirstChildRect.IsEmpty() ? m_FirstChildRect.width : m_Geometry.width);
        m_CachedBarHitRect = Rect{ m_Geometry.x, boundaryY - hitThickness * 0.5f, width, hitThickness };
        return;
    }

    const float barThickness = m_Orientation == Orientation::Horizontal
        ? std::max(1.0f, barRect.width)
        : std::max(1.0f, barRect.height);
    if (m_Orientation == Orientation::Horizontal) {
        const float padding = (hitThickness - barThickness) * 0.5f;
        const float height = std::max(1.0f, barRect.height > 0.0f ? barRect.height : m_Geometry.height);
        m_CachedBarHitRect = Rect{ barRect.x - padding, barRect.y, hitThickness, height };
        return;
    }

    const float padding = (hitThickness - barThickness) * 0.5f;
    const float width = std::max(1.0f, barRect.width > 0.0f ? barRect.width : m_Geometry.width);
    m_CachedBarHitRect = Rect{ barRect.x, barRect.y - padding, width, hitThickness };
}

void Splitter::Paint(PaintContext& context) {
    const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
    const bool secondVisible = m_SecondChild && m_SecondChild->IsVisible();

    if (m_FirstChild && firstVisible) {
        m_FirstChild->Paint(context);
    }
    if (m_SecondChild && secondVisible) {
        m_SecondChild->Paint(context);
    }

    if (!firstVisible || !secondVisible) {
        return;
    }

    Rect barRect = GetSplitterBarRect();
    const float scale = DPIContext::GetScale();
    const float thickness = std::max(1.0f, ResolveMetric(MetricToken::SplitterThickness) * scale);

    if (m_Orientation == Orientation::Horizontal) {
        Rect visualRect{ std::floor(barRect.x + (barRect.width - thickness) * 0.5f), barRect.y, thickness,
            barRect.height };
        context.DrawSurface(visualRect, SurfaceRole::Separator, 0.0f, "SplitterDivider");
    } else {
        Rect visualRect{ barRect.x, std::floor(barRect.y + (barRect.height - thickness) * 0.5f), barRect.width,
            thickness };
        context.DrawSurface(visualRect, SurfaceRole::Separator, 0.0f, "SplitterDivider");
    }
}

void Splitter::OnMouseDown(const MouseEvent& event) {
    const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
    const bool secondVisible = m_SecondChild && m_SecondChild->IsVisible();
    if (!firstVisible || !secondVisible) {
        if (m_FirstChild && firstVisible) m_FirstChild->OnMouseDown(event);
        else if (m_SecondChild && secondVisible) m_SecondChild->OnMouseDown(event);
        return;
    }

    Rect hitRect = GetSplitterHitRect();
    if (hitRect.Contains(event.position)) {
        if (!m_Dragging) {
            m_Dragging = true;
            g_SplitterDragCount.fetch_add(1, std::memory_order_relaxed);
        }
        ApplyResizeCursor(true);
    }
}

void Splitter::OnMouseMove(const MouseEvent& event) {
    const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
    const bool secondVisible = m_SecondChild && m_SecondChild->IsVisible();
    if (!firstVisible || !secondVisible) {
        if (m_FirstChild && firstVisible) m_FirstChild->OnMouseMove(event);
        else if (m_SecondChild && secondVisible) m_SecondChild->OnMouseMove(event);
        return;
    }

    Rect hitRect = GetSplitterHitRect();
    const bool wasHovered = m_Hovered;
    m_Hovered = hitRect.Contains(event.position);
    if (!m_Dragging) {
        if (wasHovered != m_Hovered) {
            InvalidatePaint();
        }
        ApplyResizeCursor(m_Hovered);
        return;
    }

    ApplyResizeCursor(true);

    const float barThickness = GetEffectiveBarThickness();
    if (m_Orientation == Orientation::Horizontal) {
        if (m_ResizeMode == ResizeMode::FixedFirst) {
            const float relativeX = event.position.x - m_Geometry.x;
            const float maxFirst = std::max(m_MinFirstPx, m_Geometry.width - barThickness - m_MinSecondPx);
            m_FixedFirstWidth = std::clamp(relativeX, m_MinFirstPx, maxFirst);
        } else if (m_ResizeMode == ResizeMode::FixedSecond) {
            const float relativeX = m_Geometry.width - (event.position.x - m_Geometry.x) - barThickness;
            const float maxSecond = std::max(m_MinSecondPx, m_Geometry.width - barThickness - m_MinFirstPx);
            m_FixedSecondWidth = std::clamp(relativeX, m_MinSecondPx, maxSecond);
        } else {
            const float usable = std::max(1.0f, m_Geometry.width - barThickness);
            const float relativeX = event.position.x - m_Geometry.x;
            m_SplitRatio = relativeX / usable;
            ClampSplitToMins(m_Geometry.width, barThickness);
        }
    } else if (m_ResizeMode == ResizeMode::FixedFirst) {
        const float relativeY = event.position.y - m_Geometry.y;
        const float maxFirst = std::max(m_MinFirstPx, m_Geometry.height - barThickness - m_MinSecondPx);
        m_FixedFirstWidth = std::clamp(relativeY, m_MinFirstPx, maxFirst);
    } else if (m_ResizeMode == ResizeMode::FixedSecond) {
        const float relativeY = m_Geometry.height - (event.position.y - m_Geometry.y) - barThickness;
        const float maxSecond = std::max(m_MinSecondPx, m_Geometry.height - barThickness - m_MinFirstPx);
        m_FixedSecondWidth = std::clamp(relativeY, m_MinSecondPx, maxSecond);
    } else {
        const float usable = std::max(1.0f, m_Geometry.height - barThickness);
        const float relativeY = event.position.y - m_Geometry.y;
        m_SplitRatio = relativeY / usable;
        ClampSplitToMins(m_Geometry.height, barThickness);
    }

    // Split geometry changed inside a fixed allotted rect — local arrange + paint only.
    Arrange(m_Geometry);
    InvalidatePaint();
}

void Splitter::OnMouseUp(const MouseEvent& event) {
    if (m_Dragging) {
        m_Dragging = false;
        const int prev = g_SplitterDragCount.fetch_sub(1, std::memory_order_relaxed);
        if (prev <= 0) {
            g_SplitterDragCount.store(0, std::memory_order_relaxed);
        }
        ApplyResizeCursor(GetSplitterHitRect().Contains(event.position));
        // Dragging arranges this splitter locally for responsiveness. Commit one
        // full tree layout on release so nested dock panels (Inspector, Asset
        // Explorer, and Viewport) do not retain stale child geometry.
        UIRepaintGate::RequestLayout();
        UIRepaintGate::RequestPaint();
    }
}

void Splitter::OnHoverLost() {
    Widget::OnHoverLost();
    if (m_Hovered) {
        m_Hovered = false;
        InvalidatePaint();
    }
    if (m_Dragging) {
        m_Dragging = false;
        const int prev = g_SplitterDragCount.fetch_sub(1, std::memory_order_relaxed);
        if (prev <= 0) {
            g_SplitterDragCount.store(0, std::memory_order_relaxed);
        }
        // A captured drag can leave the window without a mouse-up event.
        // Finalize its geometry through the same path as a normal release.
        UIRepaintGate::RequestLayout();
        UIRepaintGate::RequestPaint();
    }
    if (!AnySplitterDragging()) {
        we::platform::Platform::Get().SetSystemCursor(we::platform::SystemCursor::Arrow);
    }
}

std::shared_ptr<Widget> Splitter::HitTestPoint(const Point& pos, const Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }
    if ((clip != nullptr && !clip->Contains(pos)) || !m_Geometry.Contains(pos)) {
        return nullptr;
    }

    const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
    const bool secondVisible = m_SecondChild && m_SecondChild->IsVisible();

    if (firstVisible && secondVisible) {
        if (ComputeBarHitRect().Contains(pos)) {
            return shared_from_this();
        }
    }

    if (m_SecondChild && secondVisible) {
        if (auto hit = m_SecondChild->HitTestPoint(pos, clip)) {
            return hit;
        }
    }
    if (m_FirstChild && firstVisible) {
        if (auto hit = m_FirstChild->HitTestPoint(pos, clip)) {
            return hit;
        }
    }

    return nullptr;
}

}

