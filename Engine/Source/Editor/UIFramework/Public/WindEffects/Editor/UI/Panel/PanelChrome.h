#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Input/InputEvents.h"
#include "RHI/Types.h"
#include <string>
#include <vector>
#include <functional>
#include <functional>

namespace we::editor::panels {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::MouseEvent;

/// Shared panel design system — dock tabs, headers, toolbars, search, and rows.
namespace PanelChrome {

UIFRAMEWORK_API float UiScale();
UIFRAMEWORK_API float TabHeight();
UIFRAMEWORK_API float ToolbarHeight();
UIFRAMEWORK_API float SearchHeight();
UIFRAMEWORK_API float ListRowHeight();
UIFRAMEWORK_API float PanelPaddingH();
UIFRAMEWORK_API float CategoryHeaderHeight();
UIFRAMEWORK_API float PanelPaddingV();
UIFRAMEWORK_API float ModeTabRowHeight();
UIFRAMEWORK_API float SearchRowHeight();
UIFRAMEWORK_API float ToolbarRowHeight();
UIFRAMEWORK_API float ViewportToolbarRowHeight();
UIFRAMEWORK_API float ColumnHeaderRowHeight();
UIFRAMEWORK_API float FooterRowHeight();

UIFRAMEWORK_API float TabPadH();
UIFRAMEWORK_API float TabPadV();
UIFRAMEWORK_API float TabStripPadH();
UIFRAMEWORK_API float TabStripPadTop();
UIFRAMEWORK_API float TabActiveIndicatorWidth();
UIFRAMEWORK_API float TabIconSize();
UIFRAMEWORK_API float TabGap();
UIFRAMEWORK_API float TabTopRadius();
UIFRAMEWORK_API float HeaderButtonSize();

UIFRAMEWORK_API void PaintPanelSurface(PaintContext& context, const Rect& rect);
UIFRAMEWORK_API void PaintToolbarRegion(PaintContext& context, const Rect& rect);
/// Column label rows, list headers, and panel footer/status bands (#2F2F2F).
UIFRAMEWORK_API void PaintListLabelBand(PaintContext& context, const Rect& rect);
UIFRAMEWORK_API void PaintHeaderRegion(PaintContext& context, const Rect& rect);
UIFRAMEWORK_API void PaintFooterRegion(PaintContext& context, const Rect& rect);
UIFRAMEWORK_API void PaintContentWell(PaintContext& context, const Rect& rect);
UIFRAMEWORK_API void PaintPrimaryContentRegion(PaintContext& context, const Rect& rect);
UIFRAMEWORK_API void PaintNavigationRegion(PaintContext& context, const Rect& rect);
/// Recessed panel content well — alias for PaintContentWell.
UIFRAMEWORK_API void PaintContentRegion(PaintContext& context, const Rect& rect);
/// Single subtle divider beneath the dock tab strip (no full-width tab background).
UIFRAMEWORK_API void PaintDockTabStripDivider(PaintContext& context, const Rect& headerRect);
UIFRAMEWORK_API void PaintDockFooterDivider(PaintContext& context, const Rect& footerRect);
/// @deprecated Use PaintDockTabStripDivider — kept for callers that only need the strip edge.
UIFRAMEWORK_API void PaintDockHeaderBand(PaintContext& context, const Rect& headerRect);

struct DockTabDescriptor {
    std::string title;
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    bool hasBrand = false;
    we::rhi::RHIDescriptorSetHandle brandDescriptor = we::rhi::RHIDescriptorSetHandle::Invalid;
    float brandLogicalSize = 0.0f;
};

struct DockTabLayout {
    Rect tabRect;
    Rect closeRect;
};

UIFRAMEWORK_API float MeasureDockTabWidth(
    PaintContext& context,
    const DockTabDescriptor& tab,
    bool isActive,
    bool showClose,
    bool flushLeft = false,
    bool modeTabs = false);

UIFRAMEWORK_API DockTabLayout LayoutDockTabGeometries(
    PaintContext& context,
    const DockTabDescriptor& tab,
    const Rect& headerRect,
    float x,
    bool isActive,
    bool showClose,
    bool modeTabs = false);

UIFRAMEWORK_API DockTabLayout PaintDockTab(
    PaintContext& context,
    const DockTabDescriptor& tab,
    const Rect& headerRect,
    float x,
    bool isActive,
    float hoverAnim,
    bool showClose,
    bool closeHovered,
    bool flushLeft = false,
    bool flatCorners = false);

UIFRAMEWORK_API void PaintDockTab(
    PaintContext& context,
    const DockTabDescriptor& tab,
    const DockTabLayout& layout,
    const Rect& headerRect,
    bool isActive,
    float hoverAnim,
    bool showClose,
    bool closeHovered,
    bool flushLeft = false,
    bool flatCorners = false);

struct DockTabStripState {
    size_t activeIndex = 0;
    bool flatCorners = false; // inner mode tabs — square, no rounded tops
    std::function<bool(size_t index, bool isActive, bool isHovered)> showClose;
    std::function<float(size_t index)> hoverAnim;
    std::function<bool(size_t index)> closeHovered;
};

struct DockTabStripLayout {
    std::vector<DockTabLayout> tabs;
};

[[nodiscard]] UIFRAMEWORK_API DockTabStripLayout LayoutDockTabStrip(
    PaintContext& context,
    const Rect& stripRect,
    const std::vector<DockTabDescriptor>& descriptors,
    const DockTabStripState& state);

UIFRAMEWORK_API void PaintDockTabStrip(
    PaintContext& context,
    const Rect& stripRect,
    const std::vector<DockTabDescriptor>& descriptors,
    const DockTabStripLayout& layout,
    const DockTabStripState& state);

/// Shared dock-panel chrome layout used by every DockContainer (Actors, Viewport, Content Browser, etc.).
struct DockPanelGeometry {
    Rect chromeRect;
    Rect headerRect;
    Rect headerContentGapRect;
    Rect contentRect;
};

[[nodiscard]] UIFRAMEWORK_API bool UsesGapCutDockTabs();
[[nodiscard]] UIFRAMEWORK_API float DockStructureGapDevice();
[[nodiscard]] UIFRAMEWORK_API float DockHeaderContentGap();
[[nodiscard]] UIFRAMEWORK_API Rect InsetDockChromeRect(const Rect& allottedRect);
[[nodiscard]] UIFRAMEWORK_API DockPanelGeometry LayoutDockPanel(
    const Rect& allottedRect,
    float headerHeightDevice);
[[nodiscard]] UIFRAMEWORK_API Size InsetDockMeasureAvailable(const Size& availableSize);
[[nodiscard]] UIFRAMEWORK_API Size ExpandDockMeasuredSize(
    const Size& innerDesired,
    const Size& availableSize);
UIFRAMEWORK_API void PaintDockPanelContent(
    PaintContext& context,
    const Rect& contentRect,
    const std::function<void(PaintContext& context)>& paintBody);
UIFRAMEWORK_API void PaintDockHeaderContentGap(PaintContext& context, const Rect& gapRect);
UIFRAMEWORK_API void PaintDockPanelChrome(
    PaintContext& context,
    const Rect& headerRect,
    const Rect& headerContentGapRect,
    const Rect& contentRect,
    const std::vector<DockTabDescriptor>& descriptors,
    const DockTabStripLayout& stripLayout,
    const DockTabStripState& state,
    const std::function<void(PaintContext& context)>& paintBody);

struct FloatingHeaderAction {
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    Rect geometry;
    bool hovered = false;
    bool pressed = false;
};

UIFRAMEWORK_API void LayoutFloatingPanelHeaderGeometries(
    const Rect& headerRect,
    bool showOptionsMenu,
    size_t actionCount,
    Rect& outOptionsMenuRect,
    const std::function<void(size_t actionIndex, const Rect& actionRect)>& setActionRect);

UIFRAMEWORK_API void PaintFloatingPanelHeader(
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
    Rect& outOptionsMenuRect);

UIFRAMEWORK_API void PaintSearchField(
    PaintContext& context,
    const Rect& rect,
    const std::string& placeholder,
    const std::string& text,
    bool focused,
    bool showCaret);

UIFRAMEWORK_API void PaintAlternatingListRowBackground(
    PaintContext& context,
    const Rect& rowRect,
    int rowIndex);

UIFRAMEWORK_API void PaintListRowBackground(
    PaintContext& context,
    const Rect& rowRect,
    bool hovered,
    bool selected,
    bool focused = true);

UIFRAMEWORK_API void PaintCategoryHeader(
    PaintContext& context,
    const Rect& rect,
    const std::string& title,
    bool expanded,
    bool hovered,
    float indent = 0.0f);

UIFRAMEWORK_API void PaintHeaderIconButton(
    PaintContext& context,
    const Rect& rect,
    we::runtime::kindui::WindIconRef icon,
    bool hovered,
    bool pressed,
    bool compactGlyph = false);

/// Forward a mouse event to toolbar/content regions when the point is inside their rects.
UIFRAMEWORK_API void RoutePanelBodyPointer(
    const MouseEvent& event,
    const std::shared_ptr<Widget>& toolbar,
    const Rect& toolbarRect,
    const std::shared_ptr<Widget>& content,
    const Rect& contentRect,
    void (Widget::*handler)(const MouseEvent&));

UIFRAMEWORK_API Rect InsetSearchRect(const Rect& toolbarRect, float searchWidth);

} // namespace PanelChrome

} // namespace we::editor::panels
