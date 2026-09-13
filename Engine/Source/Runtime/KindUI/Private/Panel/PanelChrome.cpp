// ==============================================================================
// WindEffects — KindUI — PanelChrome
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Panel/PanelChrome.h"

#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/PropertyPanelChrome.h"

#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "KindUI/Layout/LayoutAssert.h"
#include "Text/Layout/TextStyle.h"
#include <algorithm>
#include <cmath>
#include <functional>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::IconColorRole;
using ::we::runtime::kindui::ResolveIconColor;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::ClampRectToParent;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;

namespace we::runtime::kindui::panels {
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::Point;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;
namespace ChromeSeparation = ::we::runtime::kindui::ChromeSeparation;

namespace {

void DrawRoundedRectTop(PaintContext& context, const Rect& rect, const Color& color, float radius, bool /*squareTopLeft*/ = false) {
    if (radius <= 0.01f) {
        context.DrawRect(rect, color);
        return;
    }
    // Always round both top corners — first-tab flush-left no longer squares the left edge.
    context.DrawRoundedRect(rect, color, radius);
    const float coverH = radius + 1.0f;
    context.DrawRect(Rect{ rect.x, rect.y + rect.height - coverH, rect.width, coverH }, color);
}

void ResolvePanelBevelColors(Color& outHighlight, Color& outShadow) {
    // Dark Graphite only — no light grey outline, but enough delta to read on Panel.
    const Color panel = we::runtime::kindui::ColorSpace::OpaqueSurface(
        we::runtime::kindui::ResolveColor(ColorToken::PanelBackground));
    const Color header = we::runtime::kindui::ColorSpace::OpaqueSurface(
        we::runtime::kindui::ResolveColor(ColorToken::HeaderBackground));
    const Color deep = we::runtime::kindui::ColorSpace::OpaqueSurface(
        we::runtime::kindui::ResolveColor(ColorToken::WorkspaceBackground));

    outHighlight = we::runtime::kindui::ColorSpace::OpaqueSurface(
        we::runtime::kindui::ColorSpace::LerpColor(panel, header, 0.85f));
    // Bottom/right: sink toward workspace gap color.
    outShadow = we::runtime::kindui::ColorSpace::OpaqueSurface(
        we::runtime::kindui::ColorSpace::LerpColor(panel, deep, 0.70f));
}
void PaintTabShoulderFill(
    PaintContext& context,
    float tabEdgeX,
    float panelTopY,
    float radius,
    bool leftSide,
    const Color& panelColor)
{
    if (radius < 2.0f) {
        return;
    }
    const float r = std::max(2.0f, IconMetrics::SnapPx(radius));
    const float ex = IconMetrics::SnapPx(tabEdgeX);
    const float py = IconMetrics::SnapPx(panelTopY);
    const int ri = std::max(2, static_cast<int>(r));

    // Rows from the top of the shoulder pocket down to just above the join.
    for (int i = 0; i < ri; ++i) {
        const float relY = static_cast<float>(i);
        const float rowY = py - r + relY;
        const float dx = std::sqrt(std::max(0.0f, r * r - relY * relY));
        // Arc x from outside center; fill only the inside of the silhouette.
        if (leftSide) {
            // Center (ex - r, py - r); arc at ex - r + dx.
            const float arcX = ex - r + dx;
            const float span = std::max(0.0f, ex - arcX);
            if (span >= 1.0f) {
                context.DrawRect(Rect{ arcX, rowY, span, 1.0f }, panelColor);
            }
        } else {
            // Center (ex + r, py - r); arc at ex + r - dx.
            const float arcX = ex + r - dx;
            const float span = std::max(0.0f, arcX - ex);
            if (span >= 1.0f) {
                context.DrawRect(Rect{ ex, rowY, span, 1.0f }, panelColor);
            }
        }
    }
}

void PaintHEdge(PaintContext& context, float x, float y, float width, const Color& color) {
    if (width < 0.5f) {
        return;
    }
    const float sx = IconMetrics::SnapPx(x);
    const float sy = IconMetrics::SnapPx(y);
    const float ex = IconMetrics::SnapPx(x + width);
    context.DrawRect(Rect{ sx, sy, std::max(1.0f, ex - sx), 1.0f }, color);
}

void PaintVEdge(PaintContext& context, float x, float y, float height, const Color& color) {
    if (height < 0.5f) {
        return;
    }
    const float sx = IconMetrics::SnapPx(x);
    const float sy = IconMetrics::SnapPx(y);
    const float ey = IconMetrics::SnapPx(y + height);
    context.DrawRect(Rect{ sx, sy, 1.0f, std::max(1.0f, ey - sy) }, color);
}

void PaintPx(PaintContext& context, float x, float y, const Color& color) {
    context.DrawRect(
        Rect{ IconMetrics::SnapPx(x), IconMetrics::SnapPx(y), 1.0f, 1.0f },
        color);
}

void PaintConvexTopCorner(
    PaintContext& context,
    float tabLeft,
    float tabRight,
    float tabTop,
    float radius,
    bool left,
    const Color& color)
{
    if (radius < 1.0f) {
        return;
    }
    constexpr float kHalfPi = 1.57079632679f;
    const int steps = std::max(6, static_cast<int>(radius) * 3);
    for (int i = 0; i <= steps; ++i) {
        const float theta = kHalfPi * (static_cast<float>(i) / static_cast<float>(steps));
        if (left) {
            PaintPx(
                context,
                tabLeft + radius * (1.0f - std::cos(theta)),
                tabTop + radius * (1.0f - std::sin(theta)),
                color);
        } else {
            PaintPx(
                context,
                tabRight - 1.0f - radius * (1.0f - std::cos(theta)),
                tabTop + radius * (1.0f - std::sin(theta)),
                color);
        }
    }
}

/// 1px rim along the outward shoulder fillet (matches PaintTabShoulderFill).
void PaintShoulderRim(
    PaintContext& context,
    float tabEdgeX,
    float panelTopY,
    float radius,
    bool left,
    const Color& color)
{
    if (radius < 2.0f) {
        return;
    }
    const float r = std::max(2.0f, IconMetrics::SnapPx(radius));
    const float ex = IconMetrics::SnapPx(tabEdgeX);
    const float py = IconMetrics::SnapPx(panelTopY);
    const int ri = std::max(2, static_cast<int>(r));

    for (int i = 0; i <= ri; ++i) {
        const float relY = static_cast<float>(i);
        const float rowY = py - r + relY;
        const float dx = std::sqrt(std::max(0.0f, r * r - relY * relY));
        if (left) {
            // Center (ex - r, py - r); arc x = ex - r + dx.
            PaintPx(context, ex - r + dx, rowY, color);
        } else {
            // Center (ex + r, py - r); arc x = ex + r - dx.
            PaintPx(context, ex + r - dx - 1.0f, rowY, color);
        }
    }
}

void PaintDockConnectedFrameBevel(
    PaintContext& context,
    const Rect& contentRect,
    const Rect& activeTabRect,
    float tabTopRadius)
{
    if (contentRect.width < 2.0f || contentRect.height < 2.0f) {
        return;
    }

    Color highlight{};
    Color shadow{};
    ResolvePanelBevelColors(highlight, shadow);

    const Color panel = we::runtime::kindui::ColorSpace::OpaqueSurface(
        we::runtime::kindui::ResolveColor(ColorToken::PanelBackground));

    const float x0 = IconMetrics::SnapPx(contentRect.x);
    const float yJoin = IconMetrics::SnapPx(contentRect.y);
    float x1 = IconMetrics::SnapPx(contentRect.x + contentRect.width);
    float y1 = IconMetrics::SnapPx(contentRect.y + contentRect.height);
    if (x1 < x0 + 2.0f) {
        x1 = x0 + 2.0f;
    }
    if (y1 < yJoin + 2.0f) {
        y1 = yJoin + 2.0f;
    }

    const bool hasTab = !activeTabRect.IsEmpty()
        && activeTabRect.width > 2.0f
        && activeTabRect.height > 2.0f;
    if (!hasTab) {
        PaintHEdge(context, x0, yJoin, x1 - x0, highlight);
        PaintVEdge(context, x0, yJoin, y1 - yJoin, highlight);
        PaintHEdge(context, x0, y1 - 1.0f, x1 - x0, shadow);
        PaintVEdge(context, x1 - 1.0f, yJoin, y1 - yJoin, shadow);
        return;
    }

    const float tx0 = IconMetrics::SnapPx(activeTabRect.x);
    const float ty0 = IconMetrics::SnapPx(activeTabRect.y);
    const float tx1 = IconMetrics::SnapPx(activeTabRect.x + activeTabRect.width);

    float topR = std::max(0.0f, IconMetrics::SnapPx(tabTopRadius));
    topR = std::min(topR, std::floor((tx1 - tx0) * 0.45f));

    const float sideLen = std::max(0.0f, yJoin - ty0);
    float shoulderR = IconMetrics::SnapPx(4.0f);
    shoulderR = std::max(3.0f, std::min(shoulderR, 5.0f));
    shoulderR = std::min(shoulderR, std::max(0.0f, sideLen - topR - 1.0f));

    const bool flushLeft = (tx0 - x0) <= 1.5f;
    const bool flushRight = (x1 - tx1) <= 1.5f;
    const bool bendLeft = !flushLeft && shoulderR >= 3.0f;
    const bool bendRight = !flushRight && shoulderR >= 3.0f;

    if (bendLeft) {
        PaintTabShoulderFill(context, tx0, yJoin, shoulderR, true, panel);
    }
    if (bendRight) {
        PaintTabShoulderFill(context, tx1, yJoin, shoulderR, false, panel);
    }

    // Seal interior join only (do not cover shoulder flare pockets).
    {
        const float sealL = tx0;
        const float sealR = tx1;
        if (sealR > sealL + 1.0f) {
            PaintHEdge(context, sealL, yJoin - 1.0f, sealR - sealL, panel);
            PaintHEdge(context, sealL, yJoin, sealR - sealL, panel);
        }
    }

    PaintHEdge(context, x0, y1 - 1.0f, x1 - x0, shadow);

    if (flushLeft) {
        PaintVEdge(context, x0, ty0 + topR, (y1 - 1.0f) - (ty0 + topR), highlight);
        PaintConvexTopCorner(context, tx0, tx1, ty0, topR, true, highlight);
    } else {
        PaintVEdge(context, x0, yJoin, y1 - yJoin, highlight);
        if (bendLeft) {
            PaintHEdge(context, x0, yJoin, (tx0 - shoulderR) - x0, highlight);
            PaintShoulderRim(context, tx0, yJoin, shoulderR, true, highlight);
            PaintVEdge(context, tx0, ty0 + topR, (yJoin - shoulderR) - (ty0 + topR), highlight);
        } else {
            PaintHEdge(context, x0, yJoin, tx0 - x0, highlight);
            PaintVEdge(context, tx0, ty0 + topR, yJoin - (ty0 + topR), highlight);
        }
        PaintConvexTopCorner(context, tx0, tx1, ty0, topR, true, highlight);
    }

    if (tx1 - tx0 > topR * 2.0f + 1.0f) {
        PaintHEdge(context, tx0 + topR, ty0, (tx1 - topR) - (tx0 + topR), highlight);
    }

    if (flushRight) {
        PaintConvexTopCorner(context, tx0, tx1, ty0, topR, false, highlight);
        PaintVEdge(context, x1 - 1.0f, ty0 + topR, (y1 - 1.0f) - (ty0 + topR), shadow);
    } else {
        PaintConvexTopCorner(context, tx0, tx1, ty0, topR, false, highlight);
        if (bendRight) {
            // Vertical stops above the shoulder; rim bends out into the panel top.
            PaintVEdge(
                context,
                tx1 - 1.0f,
                ty0 + topR,
                (yJoin - shoulderR) - (ty0 + topR),
                highlight);
            PaintShoulderRim(context, tx1, yJoin, shoulderR, false, highlight);
            PaintHEdge(context, tx1 + shoulderR, yJoin, x1 - (tx1 + shoulderR), highlight);
        } else {
            PaintVEdge(context, tx1 - 1.0f, ty0 + topR, yJoin - (ty0 + topR), highlight);
            PaintHEdge(context, tx1 - 1.0f, yJoin, x1 - (tx1 - 1.0f), highlight);
        }
        PaintVEdge(context, x1 - 1.0f, yJoin, y1 - yJoin, shadow);
    }
}

void PaintSeparatorEdge(PaintContext& context, const Rect& rect, bool topEdge) {
    const float scale = PanelChrome::UiScale();
    const float thickness = (std::max)(1.0f, IconMetrics::SnapPx(
        we::runtime::kindui::ResolveMetric(MetricToken::PanelDividerWidth) * scale));
    const float edgeY = topEdge
        ? rect.y
        : IconMetrics::SnapPx(rect.y + rect.height - thickness);
    const Rect edge{ rect.x, edgeY, rect.width, thickness };
    context.DrawSurface(edge, we::runtime::kindui::SurfaceRole::Separator, 0.0f, "PanelSeparatorEdge");
}

void PaintInsetWellTopEdge(PaintContext& context, const Rect& rect) {
    PaintSeparatorEdge(context, rect, true);
}

Color ResolveTabIconColor(bool isActive, float hoverAnim) {
    return we::runtime::kindui::ResolveColor(ColorToken::IconSecondary);
}

Color ResolveTabTextColor(bool isActive, float hoverAnim) {
    return we::runtime::kindui::ResolveTextForState(
        !isActive && hoverAnim > 0.01f,
        isActive);
}

}

namespace PanelChrome {

float UiScale() {
    return (std::max)(1.0f, DPIContext::GetScale());
}

float TabHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::PanelTabHeight) * UiScale();
}

float ToolbarHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::PanelToolbarHeight) * UiScale();
}

float SearchHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::SearchBoxHeight) * UiScale();
}

float ListRowHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ListRowHeight) * UiScale();
}

float PanelPaddingH() {
    return we::runtime::kindui::ResolveMetric(MetricToken::DockPanelGap) * UiScale();
}

float CategoryHeaderHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::FormRowHeight) * UiScale();
}

float PanelPaddingV() {
    return we::runtime::kindui::ResolveMetric(MetricToken::DockPanelGap) * UiScale();
}

float ModeTabRowHeight() {
    return TabHeight();
}

float SearchRowHeight() {
    return we::runtime::kindui::LayoutMetrics::SearchRowHeight();
}

float ToolbarRowHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::PanelToolbarHeight) * UiScale();
}

float ViewportToolbarRowHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ViewportToolbarHeight) * UiScale();
}

float ColumnHeaderRowHeight() {
    return CategoryHeaderHeight();
}

float FooterRowHeight() {
    return CategoryHeaderHeight();
}

float TabPadH() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabPaddingH) * UiScale();
}

float TabPadV() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabPaddingV) * UiScale();
}

float TabStripPadH() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabStripPadH) * UiScale();
}

float TabStripPadTop() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabStripPadV) * UiScale();
}

float TabActiveIndicatorWidth() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabActiveIndicatorWidth) * UiScale();
}

float TabIconSize() {
    return we::runtime::kindui::ResolveMetric(MetricToken::IconSizeToolbar) * UiScale();
}

float CloseGlyphSize() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CloseGlyphSize) * UiScale();
}

float TabGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabGap) * UiScale();
}

float TabIconGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabIconGap) * UiScale();
}

float TabCloseGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabCloseGap) * UiScale();
}

float TabMinWidth() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabMinWidth) * UiScale();
}

float TabTopRadius() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabTopRadius) * UiScale();
}

float HeaderButtonSize() {
    return we::runtime::kindui::ResolveMetric(MetricToken::IconButtonSize) * UiScale();
}

void PaintPanelSurface(PaintContext& context, const Rect& rect) {
    context.DrawSurface(rect, we::runtime::kindui::SurfaceRole::Panel, 0.0f, "Panel");
}

void PaintPanelAmbientShadow(PaintContext& context, const Rect& rect) {
    if (rect.width < 2.0f || rect.height < 2.0f) {
        return;
    }

    // Soft ambient depth only — edges stay sharp; no border/glow changes.
    // Spread ~2–3px, offset ~1–2px, very low opacity black.
    const float scale = UiScale();
    const float blur = 2.5f * scale;
    const float offsetY = 1.5f * scale;

    Color shadow = we::runtime::kindui::ResolveColor(ColorToken::ShadowSubtle);
    shadow.r = 0.0f;
    shadow.g = 0.0f;
    shadow.b = 0.0f;
    // ShadowSubtle starts ~0.22a; keep final contribution barely visible.
    shadow.a = std::min(shadow.a, 0.22f) * 0.40f;

    Rect shadowRect = rect;
    shadowRect.y += offsetY;
    context.DrawShadow(shadowRect, shadow, 0.0f, blur);
}

void PaintPanelFrameBevel(PaintContext& context, const Rect& rect) {
    if (rect.width < 2.0f || rect.height < 2.0f) {
        return;
    }

    // Pixel-snap the frame so 1px edges stay crisp and never collapse.
    const float x0 = IconMetrics::SnapPx(rect.x);
    const float y0 = IconMetrics::SnapPx(rect.y);
    float x1 = IconMetrics::SnapPx(rect.x + rect.width);
    float y1 = IconMetrics::SnapPx(rect.y + rect.height);
    if (x1 < x0 + 2.0f) {
        x1 = x0 + 2.0f;
    }
    if (y1 < y0 + 2.0f) {
        y1 = y0 + 2.0f;
    }

    constexpr float edge = 1.0f;
    const float w = x1 - x0;
    const float h = y1 - y0;
    const float innerH = h - edge * 2.0f;

    Color highlight{};
    Color shadow{};
    ResolvePanelBevelColors(highlight, shadow);

    // Highlight L (top + left).
    context.DrawRect(Rect{ x0, y0, w, edge }, highlight);
    if (innerH > 0.0f) {
        context.DrawRect(Rect{ x0, y0 + edge, edge, innerH }, highlight);
    }

    // Shadow L (bottom + right); right owns the corners for a soft drop.
    context.DrawRect(Rect{ x0, y1 - edge, w, edge }, shadow);
    context.DrawRect(Rect{ x1 - edge, y0, edge, h }, shadow);
}

void PaintToolbarRegion(PaintContext& context, const Rect& rect) {
    context.DrawSurface(rect, we::runtime::kindui::SurfaceRole::Toolbar, 0.0f, "PanelToolbar");
}

void PaintListLabelBand(PaintContext& context, const Rect& rect) {
    context.DrawSurface(rect, we::runtime::kindui::SurfaceRole::PanelHeader, 0.0f, "ListLabelBand");
}

void PaintHeaderRegion(PaintContext& context, const Rect& rect) {
    PaintListLabelBand(context, rect);
    PaintSeparatorEdge(context, rect, false);
}

void PaintExplorerColumnHeader(PaintContext& context, const Rect& rect, std::string_view labelText) {
    const float uiScale = UiScale();
    const float headerTextSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeCaption) * uiScale;
    const float headerTextY = we::runtime::kindui::LayoutMetrics::AlignTextTopY(rect, headerTextSize);
    const Color sepColor = we::runtime::kindui::ResolveColor(ColorToken::Separator);
    const Color textColor = we::runtime::kindui::ResolveColor(ColorToken::TextSecondary);

    const float borderW = (std::max)(1.0f, we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth));
    const float topBorderY = std::floor(rect.y);
    const float botBorderY = std::floor(rect.y + rect.height - borderW);

    context.DrawRect(Rect{ rect.x, topBorderY, rect.width, borderW }, sepColor);
    context.DrawRect(Rect{ rect.x, botBorderY, rect.width, borderW }, sepColor);

    const float eyeColWidth = std::floor(30.0f * uiScale);
    const Rect eyeBand{ rect.x, rect.y, eyeColWidth, rect.height };
    IconPainter::Draw(
        context, WindIcons::Eye16, IconMetrics::PlaceGlyphCentered(eyeBand, 16u), textColor);

    const float sep1X = std::floor(rect.x + eyeColWidth);
    context.DrawRect(Rect{ sep1X, rect.y, borderW, rect.height }, sepColor);

    const float dirtyColWidth = std::floor(28.0f * uiScale);
    const float sep2X = std::floor(sep1X + dirtyColWidth);
    const Rect starBand{ sep1X, rect.y, dirtyColWidth, rect.height };
    IconPainter::Draw(
        context, WindIcons::Pin16, IconMetrics::PlaceGlyphCentered(starBand, 16u), textColor);

    context.DrawRect(Rect{ sep2X, rect.y, borderW, rect.height }, sepColor);

    const float labelPad = std::floor(16.0f * uiScale);
    const float labelX = sep2X + labelPad;
    context.DrawText(
        std::string(labelText),
        Point{ labelX, headerTextY },
        textColor,
        headerTextSize,
        we::runtime::text::layout::FontWeight::Regular);

    const float typeColWidth = std::floor(90.0f * uiScale);
    const float sep3X = std::floor(rect.x + rect.width - typeColWidth);
    context.DrawRect(Rect{ sep3X, rect.y, borderW, rect.height }, sepColor);

    const float typeX = sep3X + labelPad;
    context.DrawText(
        "Type",
        Point{ typeX, headerTextY },
        textColor,
        headerTextSize,
        we::runtime::text::layout::FontWeight::Regular);
}

void PaintElevatedHeaderRegion(PaintContext& context, const Rect& rect) {
    PaintHeaderRegion(context, rect);
}

void PaintFooterRegion(PaintContext& context, const Rect& rect) {
    PaintListLabelBand(context, rect);
    PaintSeparatorEdge(context, rect, true);
}

void PaintContentWell(PaintContext& context, const Rect& rect) {
    context.DrawSurface(rect, we::runtime::kindui::SurfaceRole::Recessed, 0.0f, "PanelWell");
}

void PaintContentWellWithTopEdge(PaintContext& context, const Rect& rect) {
    PaintContentWell(context, rect);
    PaintInsetWellTopEdge(context, rect);
}

void PaintPrimaryContentRegion(PaintContext& context, const Rect& rect) {
    context.DrawSurface(rect, we::runtime::kindui::SurfaceRole::Panel, 0.0f, "PanelContent");
}

void PaintNavigationRegion(PaintContext& context, const Rect& rect) {
    context.DrawSurface(rect, we::runtime::kindui::SurfaceRole::Recessed, 0.0f, "PanelNavigation");
}

void PaintContentRegion(PaintContext& context, const Rect& rect) {
    PaintPrimaryContentRegion(context, rect);
}

void PaintDockTabStripDivider(PaintContext& context, const Rect& headerRect) {

}

void PaintDockFooterDivider(PaintContext& context, const Rect& footerRect) {
    PaintSeparatorEdge(context, footerRect, true);
}

void PaintDockHeaderBand(PaintContext& context, const Rect& headerRect) {

}

float MeasureDockTabWidth(
    PaintContext& context,
    const DockTabDescriptor& tab,
    bool isActive,
    bool showClose,
    bool flushLeft,
    bool modeTabs)
{

    const float scale = UiScale();
    const float fontSize = modeTabs
        ? we::runtime::kindui::ResolveMetric(MetricToken::TextSizeCaption) * scale
        : we::runtime::kindui::ResolveMetric(MetricToken::TextSizeTabs) * scale;
    const float iconSize = TabIconSize();
    const float padLeft = modeTabs
        ? we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale
        : TabPadH();
    const float padRight = modeTabs
        ? we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale
        : TabPadH();
    const float iconGap = TabIconGap();
    const float closeGap = TabCloseGap();
    const float closeGlyph = CloseGlyphSize();

    float leadingWidth = 0.0f;
    if (tab.hasBrand) {
        leadingWidth = tab.brandLogicalSize * scale + iconGap;
    } else if (tab.icon.IsValid()) {
        leadingWidth = iconSize + iconGap;
    }

    const float textWidth = context.GetTextWidth(
        tab.title,
        fontSize,
        we::runtime::text::layout::FontWeight::Regular);
    const float closeWidth = showClose ? closeGlyph + closeGap : 0.0f;
    float width = padLeft + leadingWidth + textWidth + closeWidth + padRight;
    if (!modeTabs) {
        width = std::max(width, TabMinWidth());
    }
    return width;
}

DockTabLayout LayoutDockTabGeometries(
    PaintContext& context,
    const DockTabDescriptor& tab,
    const Rect& headerRect,
    float x,
    bool isActive,
    bool showClose,
    bool modeTabs)
{
    const float scale = UiScale();
    const float padRight = modeTabs
        ? we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale
        : TabPadH();
    const float closeGlyph = CloseGlyphSize();
    const bool floatingDockTabs = !modeTabs && UsesGapCutDockTabs();
    const float stripPadV = floatingDockTabs ? TabStripPadTop() : 0.0f;
    // Connected dock tabs meet the panel — no reserved 1px divider under the tab.
    const float dividerH = 0.0f;
    const float topPad = modeTabs ? 0.0f : (floatingDockTabs ? stripPadV : TabStripPadTop());
    const float bottomPad = floatingDockTabs ? stripPadV : 0.0f;
    const float tabHeight = (std::max)(16.0f, headerRect.height - topPad - bottomPad - dividerH);
    const float tabY = headerRect.y + topPad;
    const float centerY = std::floor(tabY + tabHeight * 0.5f);

    DockTabLayout layout{};
    const float tabWidth = MeasureDockTabWidth(context, tab, isActive, showClose, false, modeTabs);
    layout.tabRect = Rect{ x, tabY, tabWidth, tabHeight };

    if (showClose) {
        const float closeX = layout.tabRect.x + layout.tabRect.width - padRight - closeGlyph;
        const float closeY = std::floor(centerY - closeGlyph * 0.5f);
        layout.closeRect = Rect{ closeX, closeY, closeGlyph, closeGlyph };
    }

    return layout;
}

void PaintDockTab(
    PaintContext& context,
    const DockTabDescriptor& tab,
    const DockTabLayout& layout,
    const Rect& headerRect,
    bool isActive,
    float hoverAnim,
    bool showClose,
    bool closeHovered,
    bool flushLeft,
    bool flatCorners)
{
    const float scale = UiScale();
    const float fontSize = flatCorners
        ? we::runtime::kindui::ResolveMetric(MetricToken::TextSizeCaption) * scale
        : we::runtime::kindui::ResolveMetric(MetricToken::TextSizeTabs) * scale;
    const float iconSize = TabIconSize();
    const float padLeft = flatCorners
        ? we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale
        : TabPadH();
    const float iconGap = TabIconGap();
    const bool dockTabs = !flatCorners;
    const bool floatingDockTabs = dockTabs && UsesGapCutDockTabs();
    const float radius = flatCorners ? 0.0f : TabTopRadius();

    if (isActive) {
        Rect activeRect = layout.tabRect;
        if (!floatingDockTabs) {
            // Extend through the header so the tab body meets the panel fill below.
            activeRect.height = (headerRect.y + headerRect.height) - activeRect.y;
            // Overlap 1px into the panel to kill any hairline seam at the join.
            activeRect.height += 1.0f;
        }
        const auto activeRole = we::runtime::kindui::SurfaceRole::TabActive;
        if (floatingDockTabs) {
            context.DrawSurface(activeRect, activeRole, radius, "DockTabActive");
        } else if (radius <= 0.01f) {
            context.DrawSurface(activeRect, activeRole, 0.0f, "DockTabActive");
        } else {
            const Color activeColor = we::runtime::kindui::ResolveSurfaceColor(activeRole);
            DrawRoundedRectTop(context, activeRect, activeColor, radius, flushLeft);
        }

        // Outward shoulders so the tab fill bends into the panel (not a hard L).
        if (!floatingDockTabs && !flatCorners) {
            const Color panelColor = we::runtime::kindui::ResolveSurfaceColor(
                we::runtime::kindui::SurfaceRole::Panel);
            const float joinY = headerRect.y + headerRect.height;
            const float sideLen = std::max(0.0f, joinY - layout.tabRect.y);
            float shoulderR = IconMetrics::SnapPx(4.0f);
            shoulderR = std::max(3.0f, std::min(shoulderR, 5.0f));
            shoulderR = std::min(shoulderR, std::max(0.0f, sideLen - radius - 1.0f));
            if (shoulderR >= 3.0f) {
                const float tabLeft = IconMetrics::SnapPx(layout.tabRect.x);
                const float tabRight = IconMetrics::SnapPx(layout.tabRect.x + layout.tabRect.width);
                const float panelLeft = IconMetrics::SnapPx(headerRect.x);
                const float panelRight = IconMetrics::SnapPx(headerRect.x + headerRect.width);
                if ((tabLeft - panelLeft) > 1.5f) {
                    PaintTabShoulderFill(context, tabLeft, joinY, shoulderR, true, panelColor);
                }
                if ((panelRight - tabRight) > 1.5f) {
                    PaintTabShoulderFill(context, tabRight, joinY, shoulderR, false, panelColor);
                }
            }
        }
    } else if (hoverAnim > 0.01f) {
        we::runtime::kindui::ControlChrome::PaintInteractiveFill(
            context,
            layout.tabRect,
            floatingDockTabs ? radius : radius,
            hoverAnim,
            0.0f,
            false,
            we::runtime::kindui::SurfaceRole::TabInactive);
    }

    float itemX = layout.tabRect.x + padLeft;
    const float centerY = std::floor(layout.tabRect.y + layout.tabRect.height * 0.5f);

    if (tab.hasBrand) {
        const float brandSize = tab.brandLogicalSize * scale;
        const float logoY = std::floor(centerY - brandSize * 0.5f);
        const auto snap = [](float v) { return std::floor(v + 0.5f); };
        if (tab.brandDescriptor != we::rhi::RHIDescriptorSetHandle::Invalid) {
            context.DrawTexture(
                Rect{ snap(itemX), snap(logoY), brandSize, brandSize },
                tab.brandDescriptor,
                we::runtime::kindui::ResolveColor(ColorToken::TextPrimary));
        }
        itemX += brandSize + iconGap;
    } else if (tab.icon.IsValid()) {
        const float effectiveIconSize = std::min(iconSize, std::max(10.0f, layout.tabRect.height - 4.0f));
        const float iconY = std::floor(centerY - effectiveIconSize * 0.5f);
        const Rect iconSlot{
            itemX,
            iconY,
            effectiveIconSize,
            effectiveIconSize
        };
        const Rect iconRect = IconMetrics::PlaceGlyphCentered(iconSlot, static_cast<uint32_t>(effectiveIconSize));
        IconPainter::Draw(
            context,
            tab.icon,
            iconRect,
            ResolveTabIconColor(isActive, hoverAnim));
        itemX += effectiveIconSize + iconGap;
    }

    const float titleY = std::floor(::we::runtime::kindui::LayoutMetrics::AlignTextTopAtCenterY(centerY, fontSize));
    context.DrawText(
        tab.title,
        Point{ itemX, titleY },
        ResolveTabTextColor(isActive, hoverAnim),
        fontSize,
        we::runtime::text::layout::FontWeight::Regular);

    if (showClose && !layout.closeRect.IsEmpty()) {
        PaintHeaderIconButton(context, layout.closeRect, WindIcons::Xv212, closeHovered, false, true);
    }
}

DockTabLayout PaintDockTab(
    PaintContext& context,
    const DockTabDescriptor& tab,
    const Rect& headerRect,
    float x,
    bool isActive,
    float hoverAnim,
    bool showClose,
    bool closeHovered,
    bool flushLeft,
    bool flatCorners)
{
    DockTabLayout layout = LayoutDockTabGeometries(context, tab, headerRect, x, isActive, showClose);
    PaintDockTab(context, tab, layout, headerRect, isActive, hoverAnim, showClose, closeHovered, flushLeft,
        flatCorners);
    return layout;
}

void LayoutFloatingPanelHeaderGeometries(
    const Rect& headerRect,
    bool showOptionsMenu,
    size_t actionCount,
    Rect& outOptionsMenuRect,
    const std::function<void(size_t actionIndex, const Rect& actionRect)>& setActionRect)
{
    const float scale = UiScale();
    const float padH = TabPadH();
    const float buttonSize = HeaderButtonSize();
    const float gap = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    const float centerY = headerRect.y + headerRect.height * 0.5f;
    const float optionsX = headerRect.x + headerRect.width - padH - buttonSize;

    outOptionsMenuRect = {};
    float actionX = headerRect.x + headerRect.width - padH;
    if (showOptionsMenu) {
        outOptionsMenuRect = Rect{ optionsX, centerY - buttonSize * 0.5f, buttonSize, buttonSize };
        actionX = optionsX - gap - buttonSize;
    }

    for (size_t i = 0; i < actionCount; ++i) {
        const size_t reverseIndex = actionCount - 1 - i;
        const Rect actionRect{ actionX, centerY - buttonSize * 0.5f, buttonSize, buttonSize };
        if (setActionRect) {
            setActionRect(reverseIndex, actionRect);
        }
        actionX -= buttonSize + gap;
    }
}

void PaintFloatingPanelHeader(
    PaintContext& context,
    const Rect& headerRect,
    const std::string& title,
    we::runtime::kindui::WindIconRef icon,
    bool hasBrand,
    we::rhi::RHIDescriptorSetHandle brandDescriptor,
    float brandLogicalSize,
    const std::vector<FloatingHeaderAction>& actions,
    bool showOptionsMenu,
    bool optionsMenuHovered,
    Rect& outOptionsMenuRect)
{
    const float scale = UiScale();
    const float gap = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    const float buttonSize = HeaderButtonSize();

    DockTabDescriptor descriptor{};
    descriptor.title = title;
    descriptor.icon = icon;
    descriptor.hasBrand = hasBrand;
    descriptor.brandDescriptor = brandDescriptor;
    descriptor.brandLogicalSize = brandLogicalSize;

    bool showClose = false;
    bool closeHovered = false;
    for (const auto& action : actions) {
        if ((action.icon.stem == WindIcons::X16.stem && action.icon.sizePx == WindIcons::X16.sizePx)
            || (action.icon.stem == WindIcons::Xv212.stem && action.icon.sizePx == WindIcons::Xv212.sizePx)) {
            showClose = true;
            closeHovered = action.hovered;
            break;
        }
    }

    const float tabX = headerRect.x + TabStripPadH();
    DockTabLayout layout = LayoutDockTabGeometries(context, descriptor, headerRect, tabX, true, showClose);

    PaintDockTab(context, descriptor, layout, headerRect, true, 0.0f, showClose, closeHovered);

    const float centerY = std::floor(layout.tabRect.y + layout.tabRect.height * 0.5f);
    float actionX = layout.tabRect.x + layout.tabRect.width + gap;
    outOptionsMenuRect = {};
    if (showOptionsMenu) {
        outOptionsMenuRect = Rect{ actionX, std::floor(centerY - buttonSize * 0.5f), buttonSize, buttonSize };
        PaintHeaderIconButton(context, outOptionsMenuRect, WindIcons::EllipsisVertical16, optionsMenuHovered, false);
        actionX += buttonSize + gap;
    }

    for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
        const auto& action = *it;
        if ((action.icon.stem == WindIcons::X16.stem && action.icon.sizePx == WindIcons::X16.sizePx)
            || (action.icon.stem == WindIcons::Xv212.stem && action.icon.sizePx == WindIcons::Xv212.sizePx)) {
            continue;
        }
        Rect actionRect{ actionX, std::floor(centerY - buttonSize * 0.5f), buttonSize, buttonSize };
        PaintHeaderIconButton(context, actionRect, action.icon, action.hovered, action.pressed, true);
        actionX += buttonSize + gap;
    }
}

DockTabStripLayout LayoutDockTabStrip(
    PaintContext& context,
    const Rect& stripRect,
    const std::vector<DockTabDescriptor>& descriptors,
    const DockTabStripState& state)
{
    DockTabStripLayout result{};
    result.tabs.reserve(descriptors.size());

    const bool modeTabs = state.flatCorners;
    float x = stripRect.x + (modeTabs ? 0.0f : TabStripPadH());
    for (size_t i = 0; i < descriptors.size(); ++i) {
        const bool isActive = i == state.activeIndex;
        const bool isHovered = state.hoverAnim ? (state.hoverAnim(i) > 0.01f) : false;
        const bool showClose = state.showClose
            ? state.showClose(i, isActive, isHovered)
            : false;
        const auto layout = LayoutDockTabGeometries(
            context,
            descriptors[i],
            stripRect,
            x,
            isActive,
            showClose,
            modeTabs);
        result.tabs.push_back(layout);
        x += layout.tabRect.width + TabGap();
    }
    return result;
}

void PaintDockTabStrip(
    PaintContext& context,
    const Rect& stripRect,
    const std::vector<DockTabDescriptor>& descriptors,
    const DockTabStripLayout& layout,
    const DockTabStripState& state)
{
    if (!stripRect.IsEmpty()) {
        const float stripEndX = stripRect.x + stripRect.width;
        float fillX = stripRect.x;
        const size_t visibleCount = std::min(descriptors.size(), layout.tabs.size());
        for (size_t i = 0; i < visibleCount; ++i) {
            const Rect& tabRect = layout.tabs[i].tabRect;
            const float tabStartX = std::max(stripRect.x, tabRect.x);
            const float tabEndX = std::min(stripEndX, tabRect.x + tabRect.width);
            if (tabStartX > fillX) {
                context.DrawSurface(
                    Rect{ fillX, stripRect.y, tabStartX - fillX, stripRect.height },
                    we::runtime::kindui::SurfaceRole::DockChrome,
                    0.0f,
                    "DockTabStrip");
            }
            fillX = std::max(fillX, tabEndX);
        }
        if (fillX < stripEndX) {
            context.DrawSurface(
                Rect{ fillX, stripRect.y, stripEndX - fillX, stripRect.height },
                we::runtime::kindui::SurfaceRole::DockChrome,
                0.0f,
                "DockTabStrip");
        }
    }

    const size_t count = std::min(descriptors.size(), layout.tabs.size());
    for (size_t i = 0; i < count; ++i) {
        const bool isActive = i == state.activeIndex;
        const float hover = state.hoverAnim ? state.hoverAnim(i) : 0.0f;
        const bool isHovered = hover > 0.01f;
        const bool showClose = state.showClose
            ? state.showClose(i, isActive, isHovered)
            : false;
        const bool closeHovered = state.closeHovered ? state.closeHovered(i) : false;
        PaintDockTab(
            context,
            descriptors[i],
            layout.tabs[i],
            stripRect,
            isActive,
            hover,
            showClose,
            closeHovered,
            i == 0,
            state.flatCorners);
    }

    const float scale = UiScale();

    if (!stripRect.IsEmpty() && state.showOptionsMenu) {
        const float buttonSize = HeaderButtonSize();
        const float rightPad = (std::max)(TabStripPadH(), TabPadH());
        const Rect optionsRect{
            stripRect.x + stripRect.width - rightPad - buttonSize,
            std::floor(stripRect.y + (stripRect.height - buttonSize) * 0.5f),
            buttonSize,
            buttonSize
        };
        PaintHeaderIconButton(
            context,
            optionsRect,
            WindIcons::EllipsisVertical16,
            state.optionsMenuHovered,
            false);

        if (state.optionsMenuHovered) {
            std::string tooltip = "Panel Options";
            const float textSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeCaption) * scale;
            const float padH = we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale;
            const float padV = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
            const float tooltipW = tooltip.length() * (6.8f * scale) + padH * 2.0f;
            const float tooltipH = textSize + padV * 2.0f;
            const Rect tooltipRect{
                optionsRect.x + (optionsRect.width - tooltipW) * 0.5f,
                optionsRect.y + optionsRect.height + padV,
                tooltipW,
                tooltipH
            };
            we::runtime::kindui::ControlChrome::PaintTooltipSurface(context, tooltipRect);
            context.DrawText(
                tooltip,
                Point{ tooltipRect.x + padH, tooltipRect.y + (tooltipRect.height - textSize) * 0.5f },
                we::runtime::kindui::ResolveColor(ColorToken::TextPrimary),
                textSize,
                we::runtime::text::layout::FontWeight::Regular,
                "PanelOptionsTooltip");
        }
    }

    if (we::runtime::kindui::UiGeometryDebug::IsEnabled() && !layout.tabs.empty()) {
        we::runtime::kindui::UiGeometryDebug::Get().TraceRegion(
            "DockTabStrip",
            stripRect,
            "DockContainer",
            TabPadH(),
            TabPadV(),
            we::runtime::kindui::ResolveMetric(MetricToken::TextSizeTabs),
            TabIconSize());
        we::runtime::kindui::UiGeometryDebug::Get().TraceRegion(
            "DockTab",
            layout.tabs.front().tabRect,
            "DockTabStrip",
            TabPadH(),
            TabPadV());
    }
}

bool UsesGapCutDockTabs() {
    // Dock tabs always use connected geometry: the active tab is the top
    // edge of its panel. Gap-cuts apply only between separate dock panels, not
    // between a panel's own tab strip and its content.
    return false;
}

float DockStructureGapDevice() {
    return ChromeSeparation::DockStructureGapPx();
}

float DockHeaderContentGap() {
    // No seam between a dock tab strip and the panel body below it.
    return 0.0f;
}

Rect InsetDockChromeRect(const Rect& allottedRect) {
    const float gap = DockStructureGapDevice();
    if (gap <= 0.0f || allottedRect.IsEmpty()) {
        return allottedRect;
    }

    return ClampRectToParent(
        Rect{
            allottedRect.x + gap,
            allottedRect.y + gap,
            std::max(0.0f, allottedRect.width - (2.0f * gap)),
            std::max(0.0f, allottedRect.height - (2.0f * gap))
        },
        allottedRect);
}

DockPanelGeometry LayoutDockPanel(const Rect& allottedRect, float headerHeightDevice) {
    DockPanelGeometry geometry{};
    geometry.chromeRect = InsetDockChromeRect(allottedRect);
    const float headerContentGap = DockHeaderContentGap();

    geometry.headerRect = ClampRectToParent(
        Rect{ geometry.chromeRect.x, geometry.chromeRect.y, geometry.chromeRect.width, headerHeightDevice },
        allottedRect);

    geometry.contentRect = ClampRectToParent(
        Rect{
            geometry.chromeRect.x,
            geometry.chromeRect.y + headerHeightDevice + headerContentGap,
            geometry.chromeRect.width,
            std::max(0.0f, geometry.chromeRect.height - headerHeightDevice - headerContentGap)
        },
        allottedRect);

    if (headerContentGap > 0.0f) {
        geometry.headerContentGapRect = ClampRectToParent(
            Rect{
                geometry.chromeRect.x,
                geometry.chromeRect.y + headerHeightDevice,
                geometry.chromeRect.width,
                headerContentGap
            },
            allottedRect);
    }

    return geometry;
}

Size InsetDockMeasureAvailable(const Size& availableSize) {
    const float structureGap = DockStructureGapDevice();
    Size measuredAvail = availableSize;
    if (measuredAvail.width < 1.0e8f) {
        measuredAvail.width = std::max(0.0f, measuredAvail.width - (2.0f * structureGap));
    }
    if (measuredAvail.height < 1.0e8f) {
        measuredAvail.height = std::max(0.0f, measuredAvail.height - (2.0f * structureGap));
    }
    return measuredAvail;
}

Size ExpandDockMeasuredSize(const Size& innerDesired, const Size& availableSize) {
    const float structureGap = DockStructureGapDevice();
    Size desired = innerDesired;
    if (availableSize.width < 1.0e8f) {
        desired.width = std::min(desired.width + (2.0f * structureGap), availableSize.width);
    }
    if (availableSize.height < 1.0e8f) {
        desired.height = std::min(desired.height + (2.0f * structureGap), availableSize.height);
    }
    return desired;
}

void PaintDockPanelContent(
    PaintContext& context,
    const Rect& contentRect,
    const std::function<void(PaintContext& context)>& paintBody)
{
    if (contentRect.IsEmpty() || !paintBody) {
        return;
    }

    // PanelBuilder sets transparent panel backgrounds so body regions own differing surfaces
    // (toolbar/header/recessed). Always paint the panel fill once here; body Content regions
    // suppress their duplicate Panel fill via SetSuppressContentSurfaces.
    PaintPanelSurface(context, contentRect);
    paintBody(context);
}

void PaintDockHeaderContentGap(PaintContext& context, const Rect& gapRect) {
    if (gapRect.IsEmpty()) {
        return;
    }
    context.DrawSurface(
        gapRect,
        we::runtime::kindui::SurfaceRole::Separator,
        0.0f,
        "DockHeaderContentGap");
}

void PaintDockPanelChrome(
    PaintContext& context,
    const Rect& headerRect,
    const Rect& headerContentGapRect,
    const Rect& contentRect,
    const std::vector<DockTabDescriptor>& descriptors,
    const DockTabStripLayout& stripLayout,
    const DockTabStripState& state,
    const std::function<void(PaintContext& context)>& paintBody,
    bool paintAmbientShadow)
{
    // Ambient depth behind the full dock silhouette (tabs + body). Paint first
    // so fills and sharp edges cover the shadow and stay crisp.
    if (paintAmbientShadow) {
        Rect chrome = contentRect;
        if (!headerRect.IsEmpty()) {
            const float top = headerRect.y;
            const float bottom = contentRect.IsEmpty()
                ? (headerRect.y + headerRect.height)
                : (contentRect.y + contentRect.height);
            const float left = std::min(
                headerRect.x,
                contentRect.IsEmpty() ? headerRect.x : contentRect.x);
            const float right = std::max(
                headerRect.x + headerRect.width,
                contentRect.IsEmpty() ? (headerRect.x + headerRect.width)
                                      : (contentRect.x + contentRect.width));
            chrome = Rect{
                left,
                top,
                std::max(0.0f, right - left),
                std::max(0.0f, bottom - top)};
        }
        PaintPanelAmbientShadow(context, chrome);
    }

    PaintDockTabStrip(context, headerRect, descriptors, stripLayout, state);
    PaintDockHeaderContentGap(context, headerContentGapRect);
    PaintDockPanelContent(context, contentRect, paintBody);

    // Connected frame: active tab bends into the panel — not a hard rectangle
    // across the whole header.
    Rect activeTab{};
    if (state.activeIndex < stripLayout.tabs.size()) {
        activeTab = stripLayout.tabs[state.activeIndex].tabRect;
        // Join tab body to the panel top so the silhouette is continuous.
        const float tabBottom = contentRect.y + 1.0f;
        if (tabBottom > activeTab.y) {
            activeTab.height = tabBottom - activeTab.y;
        }
    }
    const float tabRadius = state.flatCorners ? 0.0f : TabTopRadius();
    PaintDockConnectedFrameBevel(context, contentRect, activeTab, tabRadius);

    // Existing KindUI separator border system; width matches the tab of the panel
    if (!activeTab.IsEmpty()) {
        PaintSeparatorEdge(context, Rect{ activeTab.x, headerRect.y, activeTab.width, headerRect.height }, false);
    } else if (!headerRect.IsEmpty()) {
        PaintSeparatorEdge(context, headerRect, false);
    }
}

void PaintSearchField(
    PaintContext& context,
    const Rect& rect,
    const std::string& placeholder,
    const std::string& text,
    bool focused,
    bool showCaret)
{
    ControlChrome::InteractionState state{};
    state.focused = focused;
    ControlChrome::SearchFieldPaintOptions options{};
    ControlChrome::PaintSearchField(context, rect, placeholder, text, state, showCaret, options);
}

void PaintAlternatingListRowBackground(PaintContext& context, const Rect& rowRect, int rowIndex) {
    // Even rows match Recessed navigation/well parents (TreeView paints Recessed first).
    // Odd rows get a soft Panel stripe at low opacity — visible rhythm, not a highlight.
    if ((rowIndex % 2) == 0) {
        return;
    }
    Color stripe = we::runtime::kindui::ResolveColor(ColorToken::PanelBackground);
    stripe.a *= 0.55f;
    context.DrawRect(rowRect, stripe);
}

void PaintListRowBackground(PaintContext& context, const Rect& rowRect, bool hovered, bool selected, bool focused) {
    if (selected) {
        const we::runtime::kindui::SurfaceRole role = focused
            ? we::runtime::kindui::SurfaceRole::Selected
            : we::runtime::kindui::SurfaceRole::SelectedInactive;
        context.DrawSurface(rowRect, role, 0.0f, "TreeRow");
        return;
    }
    if (hovered) {
        context.DrawSurface(rowRect, we::runtime::kindui::SurfaceRole::ControlHover, 0.0f, "TreeRow");
    }
}

void PaintCategoryHeader(
    PaintContext& context,
    const Rect& rect,
    const std::string& title,
    bool expanded,
    bool hovered,
    float indent)
{
    we::runtime::kindui::PropertyPanelChrome::PaintSectionHeader(
        context, rect, title, expanded, hovered, indent);
}

void PaintHeaderIconButton(
    PaintContext& context,
    const Rect& rect,
    we::runtime::kindui::WindIconRef icon,
    bool hovered,
    bool pressed,
    bool compactGlyph)
{
    if (!icon.IsValid()) {
        return;
    }

    const float glyphPx = (compactGlyph
            || (icon.stem == WindIcons::X16.stem && icon.sizePx == WindIcons::X16.sizePx)
            || (icon.stem == WindIcons::Xv212.stem && icon.sizePx == WindIcons::Xv212.sizePx))
        ? CloseGlyphSize()
        : TabIconSize();

    we::runtime::kindui::ToolbarButtonChrome::PaintFloatingIcon(
        context,
        icon,
        rect,
        glyphPx,
        hovered ? 1.0f : 0.0f,
        pressed ? 1.0f : 0.0f,
        false);
}

void RoutePanelBodyPointer(
    const MouseEvent& event,
    const std::shared_ptr<Widget>& toolbar,
    const Rect& toolbarRect,
    const std::shared_ptr<Widget>& content,
    const Rect& contentRect,
    void (Widget::*handler)(const MouseEvent&))
{
    if (toolbar && toolbarRect.Contains(event.position)) {
        (toolbar.get()->*handler)(event);
        return;
    }
    if (content && contentRect.Contains(event.position)) {
        (content.get()->*handler)(event);
    }
}

Rect InsetSearchRect(const Rect& toolbarRect, float searchWidth) {
    const float padH = we::runtime::kindui::ResolveMetric(MetricToken::Space2);
    const float searchH = we::runtime::kindui::ResolveMetric(MetricToken::SearchBoxHeight);
    const float searchY = toolbarRect.y + (toolbarRect.height - searchH) * 0.5f;
    return Rect{ toolbarRect.x + padH, searchY, searchWidth, searchH };
}

}

}

