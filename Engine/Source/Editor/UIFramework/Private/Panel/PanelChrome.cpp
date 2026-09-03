#include "WindEffects/Editor/UI/Panel/PanelChrome.h"

#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/PropertyPanelChrome.h"

#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "KindUI/Layout/LayoutAssert.h"
#include "Text/Layout/TextStyle.h"
#include <algorithm>
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

namespace we::editor::panels {
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::Point;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;
namespace ChromeSeparation = ::we::runtime::kindui::ChromeSeparation;

namespace {

void DrawRoundedRectTop(PaintContext& context, const Rect& rect, const Color& color, float radius, bool squareTopLeft = false) {
    if (radius <= 0.01f) {
        context.DrawRect(rect, color);
        return;
    }
    if (squareTopLeft) {
        const float capW = std::min(radius, rect.width);
        const float bodyW = std::max(0.0f, rect.width - capW);
        if (bodyW > 0.0f) {
            context.DrawRect(Rect{ rect.x, rect.y, bodyW, rect.height }, color);
        }
        if (capW > 0.0f) {
            const Rect cap{ rect.x + bodyW, rect.y, capW, rect.height };
            context.DrawRoundedRect(cap, color, radius);
            const float coverH = radius + 1.0f;
            context.DrawRect(Rect{ cap.x, cap.y + cap.height - coverH, cap.width, coverH }, color);
        }
        return;
    }
    context.DrawRoundedRect(rect, color, radius);
    const float coverH = radius + 1.0f;
    context.DrawRect(Rect{ rect.x, rect.y + rect.height - coverH, rect.width, coverH }, color);
}

void PaintSeparatorEdge(PaintContext& context, const Rect& rect, bool topEdge) {
    if (ChromeSeparation::kGapCutsEnabled) {
        (void)context;
        (void)rect;
        (void)topEdge;
        return;
    }
    const float scale = PanelChrome::UiScale();
    const float thickness = (std::max)(1.0f, IconMetrics::SnapPx(
        we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth) * scale));
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
    (void)isActive;
    return ResolveIconColor(IconColorRole::Secondary, hoverAnim);
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
    return we::runtime::kindui::ResolveMetric(MetricToken::Space2) * UiScale();
}

float CategoryHeaderHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CategoryHeaderHeight) * UiScale();
}

float PanelPaddingV() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space1) * UiScale();
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
    return ListRowHeight();
}

float FooterRowHeight() {
    return ListRowHeight();
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
    return static_cast<float>(16u);
}

float CloseGlyphSize() {
    return we::runtime::kindui::ResolveMetric(MetricToken::IconSizeVerySmall) * UiScale();
}

float TabGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabGap) * UiScale();
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
    PaintContentWell(context, rect);
}

void PaintDockTabStripDivider(PaintContext& context, const Rect& headerRect) {
    PaintSeparatorEdge(context, headerRect, false);
}

void PaintDockFooterDivider(PaintContext& context, const Rect& footerRect) {
    PaintSeparatorEdge(context, footerRect, true);
}

void PaintDockHeaderBand(PaintContext& context, const Rect& headerRect) {
    PaintDockTabStripDivider(context, headerRect);
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
    const float padH = modeTabs
        ? we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale
        : TabPadH();
    const float iconGap = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    const float closeGap = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
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
        we::runtime::text::layout::FontWeight::Medium);
    const float closeWidth = showClose ? closeGlyph + closeGap : 0.0f;
    float width = padH + leadingWidth + textWidth + closeWidth + padH;
    if (!modeTabs && UsesGapCutDockTabs()) {
        width = std::max(width, 72.0f * scale);
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
    const float padH = modeTabs
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
        const float closeX = layout.tabRect.x + layout.tabRect.width - padH - closeGlyph;
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
    const float padH = flatCorners
        ? we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale
        : TabPadH();
    const float iconGap = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    const bool dockTabs = !flatCorners;
    const bool floatingDockTabs = dockTabs && UsesGapCutDockTabs();
    const float radius = flatCorners ? 0.0f : TabTopRadius();

    if (isActive) {
        Rect activeRect = layout.tabRect;
        if (!floatingDockTabs) {
            activeRect.height = (headerRect.y + headerRect.height) - activeRect.y;
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

    float itemX = layout.tabRect.x + padH;
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
        const Rect iconSlot{
            itemX,
            layout.tabRect.y,
            iconSize,
            layout.tabRect.height
        };
        const Rect iconRect = IconMetrics::PlaceGlyphCentered(iconSlot, static_cast<uint32_t>(iconSize));
        IconPainter::Draw(
            context,
            tab.icon,
            iconRect);
        itemX += iconSize + iconGap;
    }

    const float titleY = std::floor(centerY - fontSize * 0.5f);
    context.DrawText(
        tab.title,
        Point{ itemX, titleY },
        ResolveTabTextColor(isActive, hoverAnim),
        fontSize,
        we::runtime::text::layout::FontWeight::Medium);

    if (showClose && !layout.closeRect.IsEmpty()) {
        PaintHeaderIconButton(context, layout.closeRect, WindIcons::Close16, closeHovered, false, true);
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
    context.DrawSurface(headerRect, we::runtime::kindui::SurfaceRole::PanelHeader, 0.0f, "FloatingPanelHeader");

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
        if (action.icon.stem == WindIcons::Close16.stem && action.icon.sizePx == WindIcons::Close16.sizePx) {
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
        PaintHeaderIconButton(context, outOptionsMenuRect, WindIcons::VerticalDots16, optionsMenuHovered, false);
        actionX += buttonSize + gap;
    }

    for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
        const auto& action = *it;
        if (action.icon.stem == WindIcons::Close16.stem && action.icon.sizePx == WindIcons::Close16.sizePx) {
            continue;
        }
        Rect actionRect{ actionX, std::floor(centerY - buttonSize * 0.5f), buttonSize, buttonSize };
        PaintHeaderIconButton(context, actionRect, action.icon, action.hovered, action.pressed, true);
        actionX += buttonSize + gap;
    }

    PaintDockTabStripDivider(context, headerRect);
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
    context.DrawSurface(stripRect, we::runtime::kindui::SurfaceRole::DockChrome, 0.0f, "DockTabStrip");

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
    PaintDockTabStripDivider(context, stripRect);

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
    // Dock tabs always use UE5-style connected geometry: the active tab is the top
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

    PaintPanelSurface(context, contentRect);
    context.PushClipRect(contentRect);
    paintBody(context);
    context.PopClipRect();
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
    const we::runtime::kindui::SurfaceRole role = (rowIndex % 2 == 0)
        ? we::runtime::kindui::SurfaceRole::Recessed
        : we::runtime::kindui::SurfaceRole::Panel;
    context.DrawSurface(rowRect, role, 0.0f, "ListRowStripe");
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
    const bool isClose = icon.stem == WindIcons::Close16.stem && icon.sizePx == WindIcons::Close16.sizePx;
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
        IconPainter::Draw(context, icon, rect, glyph);
    } else {
        const uint32_t iconSize = static_cast<uint32_t>(TabIconSize());
        IconPainter::Draw(context, icon, rect, iconSize);
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

} // namespace we::editor::panels
