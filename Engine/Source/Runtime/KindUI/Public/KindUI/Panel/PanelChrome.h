// ==============================================================================
// WindEffects — KindUI — PanelChrome
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Input/InputEvents.h"
#include "RHI/Types.h"
#include <string>
#include <string_view>
#include <vector>
#include <functional>

namespace we::runtime::kindui::panels {

using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::MouseEvent;

/// Shared panel design system — dock tabs, headers, toolbars, search, and rows.
namespace PanelChrome {

KINDUI_API float UiScale();
KINDUI_API float TabHeight();
KINDUI_API float ToolbarHeight();
KINDUI_API float SearchHeight();
KINDUI_API float ListRowHeight();
KINDUI_API float PanelPaddingH();
KINDUI_API float CategoryHeaderHeight();
KINDUI_API float PanelPaddingV();
KINDUI_API float ModeTabRowHeight();
KINDUI_API float SearchRowHeight();
KINDUI_API float ToolbarRowHeight();
KINDUI_API float ViewportToolbarRowHeight();
KINDUI_API float ColumnHeaderRowHeight();
KINDUI_API float FooterRowHeight();

KINDUI_API float TabPadH();
KINDUI_API float TabPadV();
KINDUI_API float TabStripPadH();
KINDUI_API float TabStripPadTop();
KINDUI_API float TabActiveIndicatorWidth();
KINDUI_API float TabIconSize();
KINDUI_API float TabGap();
KINDUI_API float TabIconGap();
KINDUI_API float TabCloseGap();
KINDUI_API float TabMinWidth();
KINDUI_API float CloseGlyphSize();
KINDUI_API float TabTopRadius();
KINDUI_API float HeaderButtonSize();

KINDUI_API void PaintPanelSurface(PaintContext& context, const Rect& rect);
/// Barely-visible ambient drop shadow behind panel chrome. Does not alter edges.
KINDUI_API void PaintPanelAmbientShadow(PaintContext& context, const Rect& rect);
/// Soft 1px raised frame (brighter top/left, darker bottom/right) around a panel chrome rect.
KINDUI_API void PaintPanelFrameBevel(PaintContext& context, const Rect& rect);
KINDUI_API void PaintToolbarRegion(PaintContext& context, const Rect& rect);
KINDUI_API void PaintListLabelBand(PaintContext& context, const Rect& rect);
KINDUI_API void PaintHeaderRegion(PaintContext& context, const Rect& rect);
KINDUI_API void PaintFooterRegion(PaintContext& context, const Rect& rect);
KINDUI_API void PaintContentWell(PaintContext& context, const Rect& rect);
KINDUI_API void PaintPrimaryContentRegion(PaintContext& context, const Rect& rect);
KINDUI_API void PaintNavigationRegion(PaintContext& context, const Rect& rect);
KINDUI_API void PaintContentRegion(PaintContext& context, const Rect& rect);
KINDUI_API void PaintDockTabStripDivider(PaintContext& context, const Rect& headerRect);
KINDUI_API void PaintDockFooterDivider(PaintContext& context, const Rect& footerRect);
KINDUI_API void PaintDockHeaderBand(PaintContext& context, const Rect& headerRect);

/// Shared explorer/content-browser column header (eye, pin, label, type).
KINDUI_API void PaintExplorerColumnHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view labelText = "Item Label");

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

KINDUI_API float MeasureDockTabWidth(
    PaintContext& context,
    const DockTabDescriptor& tab,
    bool isActive,
    bool showClose,
    bool flushLeft = false,
    bool modeTabs = false);

KINDUI_API DockTabLayout LayoutDockTabGeometries(
    PaintContext& context,
    const DockTabDescriptor& tab,
    const Rect& headerRect,
    float x,
    bool isActive,
    bool showClose,
    bool modeTabs = false);

KINDUI_API DockTabLayout PaintDockTab(
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

KINDUI_API void PaintDockTab(
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
    bool flatCorners = false;
    bool optionsMenuHovered = false;
    bool showOptionsMenu = true;
    std::function<bool(size_t index, bool isActive, bool isHovered)> showClose;
    std::function<float(size_t index)> hoverAnim;
    std::function<bool(size_t index)> closeHovered;
};

struct DockTabStripLayout {
    std::vector<DockTabLayout> tabs;
};

[[nodiscard]] KINDUI_API DockTabStripLayout LayoutDockTabStrip(
    PaintContext& context,
    const Rect& stripRect,
    const std::vector<DockTabDescriptor>& descriptors,
    const DockTabStripState& state);

KINDUI_API void PaintDockTabStrip(
    PaintContext& context,
    const Rect& stripRect,
    const std::vector<DockTabDescriptor>& descriptors,
    const DockTabStripLayout& layout,
    const DockTabStripState& state);

struct DockPanelGeometry {
    Rect chromeRect;
    Rect headerRect;
    Rect headerContentGapRect;
    Rect contentRect;
};

[[nodiscard]] KINDUI_API bool UsesGapCutDockTabs();
[[nodiscard]] KINDUI_API float DockStructureGapDevice();
[[nodiscard]] KINDUI_API float DockHeaderContentGap();
[[nodiscard]] KINDUI_API Rect InsetDockChromeRect(const Rect& allottedRect);
[[nodiscard]] KINDUI_API DockPanelGeometry LayoutDockPanel(
    const Rect& allottedRect,
    float headerHeightDevice);
[[nodiscard]] KINDUI_API Size InsetDockMeasureAvailable(const Size& availableSize);
[[nodiscard]] KINDUI_API Size ExpandDockMeasuredSize(
    const Size& innerDesired,
    const Size& availableSize);
KINDUI_API void PaintDockPanelContent(
    PaintContext& context,
    const Rect& contentRect,
    const std::function<void(PaintContext& context)>& paintBody);
KINDUI_API void PaintDockHeaderContentGap(PaintContext& context, const Rect& gapRect);
KINDUI_API void PaintDockPanelChrome(
    PaintContext& context,
    const Rect& headerRect,
    const Rect& headerContentGapRect,
    const Rect& contentRect,
    const std::vector<DockTabDescriptor>& descriptors,
    const DockTabStripLayout& stripLayout,
    const DockTabStripState& state,
    const std::function<void(PaintContext& context)>& paintBody,
    bool paintAmbientShadow = true);

struct FloatingHeaderAction {
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    Rect geometry;
    bool hovered = false;
    bool pressed = false;
};

KINDUI_API void LayoutFloatingPanelHeaderGeometries(
    const Rect& headerRect,
    bool showOptionsMenu,
    size_t actionCount,
    Rect& outOptionsMenuRect,
    const std::function<void(size_t actionIndex, const Rect& actionRect)>& setActionRect);

KINDUI_API void PaintFloatingPanelHeader(
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

KINDUI_API void PaintSearchField(
    PaintContext& context,
    const Rect& rect,
    const std::string& placeholder,
    const std::string& text,
    bool focused,
    bool showCaret);

KINDUI_API void PaintAlternatingListRowBackground(
    PaintContext& context,
    const Rect& rowRect,
    int rowIndex);

KINDUI_API void PaintListRowBackground(
    PaintContext& context,
    const Rect& rowRect,
    bool hovered,
    bool selected,
    bool focused = true);

KINDUI_API void PaintCategoryHeader(
    PaintContext& context,
    const Rect& rect,
    const std::string& title,
    bool expanded,
    bool hovered,
    float indent = 0.0f);

KINDUI_API void PaintHeaderIconButton(
    PaintContext& context,
    const Rect& rect,
    we::runtime::kindui::WindIconRef icon,
    bool hovered,
    bool pressed,
    bool compactGlyph = false);

KINDUI_API void RoutePanelBodyPointer(
    const MouseEvent& event,
    const std::shared_ptr<Widget>& toolbar,
    const Rect& toolbarRect,
    const std::shared_ptr<Widget>& content,
    const Rect& contentRect,
    void (Widget::*handler)(const MouseEvent&));

KINDUI_API Rect InsetSearchRect(const Rect& toolbarRect, float searchWidth);

} // namespace PanelChrome

} // namespace we::runtime::kindui::panels
