#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Core/Types.h"
#include "Core/PaintContext.h"
#include "RHI/Types.h"
#include <string>
#include <vector>

namespace WindEffects::Editor::UI {

class Widget;

/// Shared panel design system — dock tabs, headers, toolbars, search, and rows.
namespace PanelChrome {

UIFRAMEWORK_API float UiScale();
UIFRAMEWORK_API float TabHeight();
UIFRAMEWORK_API float ToolbarHeight();
UIFRAMEWORK_API float SearchHeight();
UIFRAMEWORK_API float ListRowHeight();
UIFRAMEWORK_API float PanelPaddingH();
UIFRAMEWORK_API float CategoryHeaderHeight();

UIFRAMEWORK_API float TabPadH();
UIFRAMEWORK_API float TabIconSize();
UIFRAMEWORK_API float TabGap();
UIFRAMEWORK_API float TabTopRadius();
UIFRAMEWORK_API float HeaderButtonSize();

UIFRAMEWORK_API void PaintPanelSurface(PaintContext& context, const Rect& rect);
UIFRAMEWORK_API void PaintToolbarRegion(PaintContext& context, const Rect& rect);
UIFRAMEWORK_API void PaintContentRegion(PaintContext& context, const Rect& rect);
UIFRAMEWORK_API void PaintDockHeaderBand(PaintContext& context, const Rect& headerRect);

struct DockTabDescriptor {
    std::string title;
    std::string iconName;
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
    bool flushLeft = false);

UIFRAMEWORK_API DockTabLayout PaintDockTab(
    PaintContext& context,
    const DockTabDescriptor& tab,
    const Rect& headerRect,
    float x,
    bool isActive,
    float hoverAnim,
    bool showClose,
    bool closeHovered,
    bool flushLeft = false);

struct FloatingHeaderAction {
    std::string iconName;
    Rect geometry;
    bool hovered = false;
    bool pressed = false;
};

UIFRAMEWORK_API void PaintFloatingPanelHeader(
    PaintContext& context,
    const Rect& headerRect,
    const std::string& title,
    const std::string& iconName,
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

UIFRAMEWORK_API void PaintListRowBackground(
    PaintContext& context,
    const Rect& rowRect,
    bool hovered,
    bool selected);

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
    const std::string& iconName,
    bool hovered,
    bool pressed,
    bool compactGlyph = false);

UIFRAMEWORK_API void PaintToolbarIconButton(
    PaintContext& context,
    const Rect& rect,
    const std::string& iconName,
    bool hovered,
    bool pressed);

UIFRAMEWORK_API Rect InsetSearchRect(const Rect& toolbarRect, float searchWidth);

} // namespace PanelChrome

} // namespace WindEffects::Editor::UI
