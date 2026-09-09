#include "KindUI/Panel/PanelChrome.h"

#include "KindUI/Core/ControlChrome.h"
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
    const Color panel = we::runtime::kindui::ColorSpace::OpaqueSurface(
        we::runtime::kindui::ResolveColor(ColorToken::PanelBackground));
    const Color bevelHi = we::runtime::kindui::ResolveColor(ColorToken::ButtonBevelHighlight);
    const Color bevelLo = we::runtime::kindui::ResolveColor(ColorToken::ButtonBevelShadow);

    auto channelDelta = [](const Color& a, const Color& b) {
        return std::fabs(a.r - b.r) + std::fabs(a.g - b.g) + std::fabs(a.b - b.b);
    };

    outHighlight = we::runtime::kindui::ColorSpace::OpaqueSurface(
        we::runtime::kindui::ColorSpace::LerpColor(panel, bevelHi, 0.75f));
    if (channelDelta(outHighlight, panel) < 0.05f) {
        outHighlight = we::runtime::kindui::ColorSpace::OpaqueSurface(
            we::runtime::kindui::ColorSpace::LerpColor(
                panel,
                we::runtime::kindui::ResolveColor(ColorToken::BorderLight),
                0.35f));
    }

    outShadow = we::runtime::kindui::ColorSpace::OpaqueSurface(
        we::runtime::kindui::ColorSpace::LerpColor(panel, bevelLo, 0.80f));
    if (channelDelta(panel, outShadow) < 0.05f) {
        outShadow = we::runtime::kindui::ColorSpace::OpaqueSurface(
            we::runtime::kindui::ColorSpace::LerpColor(panel, Color::Black(), 0.40f));
    }
}

/// Soft concave shoulder where the active tab bends into the panel top.
void PaintTabShoulderFill(
    PaintContext& context,
    float tabEdgeX,
    float panelTopY,
    float radius,
    bool leftSide,
    const Color& panelColor)
{
    if (radius < 1.0f) {
        return;
    }
    const float r = IconMetrics::SnapPx(radius);
    const float cx = IconMetrics::SnapPx(tabEdgeX);
    const float cy = IconMetrics::SnapPx(panelTopY);
    const Rect pocket = leftSide
        ? Rect{ cx - r, cy - r, r, r }
        : Rect{ cx, cy - r, r, r };
    if (pocket.IsEmpty()) {
        return;
    }
    // Quarter-disk of panel color centered on the tab/panel junction.
    context.PushClipRect(pocket);
    context.DrawRoundedRect(Rect{ cx - r, cy - r, r * 2.0f, r * 2.0f }, panelColor, r);
    context.PopClipRect();
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

/// Convex outer corner (tab top). `left` = top-left, otherwise top-right.
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
    const int steps = std::max(4, static_cast<int>(radius) * 2);
    for (int i = 0; i <= steps; ++i) {
        const float theta = kHalfPi * (static_cast<float>(i) / static_cast<float>(steps));
        // theta 0 at top edge → pi/2 at side edge.
        if (left) {
            const float px = tabLeft + radius * (1.0f - std::cos(theta));
            const float py = tabTop + radius * (1.0f - std::sin(theta));
            PaintPx(context, px, py, color);
        } else {
            const float px = tabRight - 1.0f - radius * (1.0f - std::cos(theta));
            const float py = tabTop + radius * (1.0f - std::sin(theta));
            PaintPx(context, px, py, color);
        }
    }
}

/// Concave shoulder where tab side bends into the panel top.
void PaintConcaveShoulderRim(
    PaintContext& context,
    float tabEdgeX,
    float panelTopY,
    float radius,
    bool left,
    const Color& color)
{
    if (radius < 1.0f) {
        return;
    }
    constexpr float kHalfPi = 1.57079632679f;
    const int steps = std::max(4, static_cast<int>(radius) * 2);
    for (int i = 0; i <= steps; ++i) {
        const float theta = kHalfPi * (static_cast<float>(i) / static_cast<float>(steps));
        // theta 0 at tab side (above panel) → pi/2 at panel top (outside tab).
        if (left) {
            const float px = tabEdgeX - radius * std::sin(theta);
            const float py = panelTopY - radius * std::cos(theta);
            PaintPx(context, px, py, color);
        } else {
            const float px = tabEdgeX + radius * std::sin(theta) - 1.0f;
            const float py = panelTopY - radius * std::cos(theta);
            PaintPx(context, px, py, color);
        }
    }
}

/// 1px bevel that follows the active-tab silhouette into the panel (not a hard rectangle).
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
    const float yBody = IconMetrics::SnapPx(contentRect.y);
    float x1 = IconMetrics::SnapPx(contentRect.x + contentRect.width);
    float y1 = IconMetrics::SnapPx(contentRect.y + contentRect.height);
    if (x1 < x0 + 2.0f) {
        x1 = x0 + 2.0f;
    }
    if (y1 < yBody + 2.0f) {
        y1 = yBody + 2.0f;
    }

    const bool hasTab = !activeTabRect.IsEmpty()
        && activeTabRect.width > 2.0f
        && activeTabRect.height > 2.0f;
    if (!hasTab) {
        PaintHEdge(context, x0, yBody, x1 - x0, highlight);
        PaintVEdge(context, x0, yBody, y1 - yBody, highlight);
        PaintHEdge(context, x0, y1 - 1.0f, x1 - x0, shadow);
        PaintVEdge(context, x1 - 1.0f, yBody, y1 - yBody, shadow);
        return;
    }

    const float tx0 = IconMetrics::SnapPx(activeTabRect.x);
    const float ty0 = IconMetrics::SnapPx(activeTabRect.y);
    const float tx1 = IconMetrics::SnapPx(activeTabRect.x + activeTabRect.width);
    float topR = std::max(0.0f, IconMetrics::SnapPx(tabTopRadius));
    // Keep radius inside the tab; avoid degenerate arcs on narrow tabs.
    topR = std::min(topR, std::floor((tx1 - tx0) * 0.45f));
    float shoulderR = std::max(
        0.0f,
        IconMetrics::SnapPx(std::min(topR > 0.0f ? topR : 4.0f, 5.0f)));
    if (shoulderR > 0.0f) {
        const float maxShoulder = std::max(0.0f, (yBody - ty0) - topR - 2.0f);
        shoulderR = std::min(shoulderR, maxShoulder);
    }

    const bool flushLeft = (tx0 - x0) <= 1.5f;
    const bool flushRight = (x1 - tx1) <= 1.5f;

    // Soft fill bend at tab→panel shoulders.
    if (!flushLeft && shoulderR >= 1.0f) {
        PaintTabShoulderFill(context, tx0, yBody, shoulderR, true, panel);
    }
    if (!flushRight && shoulderR >= 1.0f) {
        PaintTabShoulderFill(context, tx1, yBody, shoulderR, false, panel);
    }

    // Seal under the active tab so no chord/seam remains.
    {
        const float sealL = flushLeft ? x0 : tx0;
        const float sealR = flushRight ? x1 : tx1;
        PaintHEdge(context, sealL, yBody - 1.0f, sealR - sealL, panel);
        PaintHEdge(context, sealL, yBody, sealR - sealL, panel);
    }

    // Panel bottom + outer sides under the body.
    PaintHEdge(context, x0, y1 - 1.0f, x1 - x0, shadow);
    if (!flushLeft) {
        PaintVEdge(context, x0, yBody, y1 - yBody, highlight);
    }
    if (!flushRight) {
        PaintVEdge(context, x1 - 1.0f, yBody, y1 - yBody, shadow);
    }

    // --- Continuous silhouette rim (no DrawControlOutline — it broke corner joins) ---

    // Left path.
    if (flushLeft) {
        // One vertical from below the top corner through the panel bottom.
        PaintVEdge(context, x0, ty0 + topR, (y1 - 1.0f) - (ty0 + topR), highlight);
        PaintConvexTopCorner(context, tx0, tx1, ty0, topR, true, highlight);
    } else {
        PaintHEdge(context, x0, yBody, (tx0 - shoulderR) - x0, highlight);
        PaintConcaveShoulderRim(context, tx0, yBody, shoulderR, true, highlight);
        PaintVEdge(context, tx0, ty0 + topR, (yBody - shoulderR) - (ty0 + topR), highlight);
        PaintConvexTopCorner(context, tx0, tx1, ty0, topR, true, highlight);
    }

    // Tab top flat between the two convex corners.
    if (tx1 - tx0 > topR * 2.0f + 1.0f) {
        PaintHEdge(context, tx0 + topR, ty0, (tx1 - topR) - (tx0 + topR), highlight);
    }

    // Right path.
    if (flushRight) {
        PaintConvexTopCorner(context, tx0, tx1, ty0, topR, false, shadow);
        PaintVEdge(context, x1 - 1.0f, ty0 + topR, (y1 - 1.0f) - (ty0 + topR), shadow);
    } else {
        PaintConvexTopCorner(context, tx0, tx1, ty0, topR, false, highlight);
        PaintVEdge(context, tx1 - 1.0f, ty0 + topR, (yBody - shoulderR) - (ty0 + topR), highlight);
        PaintConcaveShoulderRim(context, tx1, yBody, shoulderR, false, highlight);
        PaintHEdge(context, tx1 + shoulderR, yBody, x1 - (tx1 + shoulderR), highlight);
        PaintVEdge(context, x1 - 1.0f, yBody, y1 - yBody, shadow);
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
    if (isActive) {
        return we::runtime::kindui::ResolveColor(ColorToken::IconActive);
    }
    if (hoverAnim > 0.01f) {
        return we::runtime::kindui::ResolveColor(ColorToken::IconHover);
    }
    return we::runtime::kindui::ResolveColor(ColorToken::IconSecondary);
}

Color ResolveTabTextColor(bool isActive, float hoverAnim) {
    return we::runtime::kindui::ResolveTextForState(
        !isActive && hoverAnim > 0.01f,
        isActive);
}

} // namespace

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
    return we::runtime::kindui::ResolveMetric(MetricToken::CategoryHeaderHeight) * UiScale();
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
    (void)context;
    (void)headerRect;
}

void PaintDockFooterDivider(PaintContext& context, const Rect& footerRect) {
    PaintSeparatorEdge(context, footerRect, true);
}

void PaintDockHeaderBand(PaintContext& context, const Rect& headerRect) {
    (void)context;
    (void)headerRect;
}

float MeasureDockTabWidth(
    PaintContext& context,
    const DockTabDescriptor& tab,
    bool isActive,
    bool showClose,
    bool flushLeft,
    bool modeTabs)
{
    (void)flushLeft;
    (void)isActive;
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
    const float dividerH = (modeTabs || floatingDockTabs)
        ? 0.0f
        : we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth) * scale;
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
    PaintDockTab(context, tab, layout, headerRect, isActive, hoverAnim, showClose, closeHovered, flushLeft, flatCorners);
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
            const float scale = UiScale();
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
    const std::function<void(PaintContext& context)>& paintBody)
{
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
    we::runtime::kindui::SurfaceRole role = we::runtime::kindui::SurfaceRole::Transparent;
    if (selected) {
        role = focused
            ? we::runtime::kindui::SurfaceRole::Selected
            : we::runtime::kindui::SurfaceRole::SelectedInactive;
    } else if (hovered) {
        role = we::runtime::kindui::SurfaceRole::ControlHover;
    }
    if (role != we::runtime::kindui::SurfaceRole::Transparent) {
        context.DrawSurface(rowRect, role, 0.0f, "TreeRow");
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
    const bool isClose = (icon.stem == WindIcons::X16.stem && icon.sizePx == WindIcons::X16.sizePx)
        || (icon.stem == WindIcons::Xv212.stem && icon.sizePx == WindIcons::Xv212.sizePx);
    const float scale = UiScale();
    const float radius = we::runtime::kindui::ResolveMetric(MetricToken::IconButtonRadius) * scale;

    if (!isClose) {
        we::runtime::kindui::ControlChrome::PaintInteractiveFill(
            context,
            rect,
            radius,
            hovered ? 1.0f : 0.0f,
            pressed ? 1.0f : 0.0f,
            false,
            ColorToken::ControlBackground);
    }

    if (!icon.IsValid()) {
        return;
    }

    if (isClose || compactGlyph) {
        const uint32_t glyph = static_cast<uint32_t>(CloseGlyphSize());
        const Rect iconRect = IconMetrics::PlaceGlyphCentered(rect, glyph);
        IconPainter::Draw(context, icon, iconRect, glyph);
    } else {
        const uint32_t iconSize = static_cast<uint32_t>(TabIconSize());
        const Rect iconRect = IconMetrics::PlaceGlyphCentered(rect, iconSize);
        IconPainter::Draw(context, icon, iconRect, iconSize);
    }
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

} // namespace PanelChrome

} // namespace we::runtime::kindui::panels
 
