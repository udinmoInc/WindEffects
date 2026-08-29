#include "WindEffects/Editor/UI/Panel/PanelChrome.h"

#include "KindUI/Core/PropertyPanelChrome.h"

#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeColors.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Rendering/IconMetrics.h"
#include <algorithm>
#include <cmath>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::IconColorRole;
using ::we::runtime::kindui::ResolveIconColor;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::DPIContext;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;

namespace we::editor::panels {
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::Point;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;

namespace {

void DrawRoundedRectTop(PaintContext& context, const Rect& rect, const Color& color, float radius) {
    if (radius <= 0.01f) {
        context.DrawRect(rect, color);
        return;
    }
    context.DrawRoundedRect(rect, color, radius);
    const float coverH = radius + 1.0f;
    context.DrawRect(Rect{ rect.x, rect.y + rect.height - coverH, rect.width, coverH }, color);
}

void PaintSeparatorEdge(PaintContext& context, const Rect& rect, bool topEdge) {
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float thickness = we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth) * scale;
    const Color separator = we::runtime::kindui::ds::Border::Separator();
    if (topEdge) {
        context.DrawRect(Rect{ rect.x, rect.y, rect.width, thickness }, separator);
    } else {
        context.DrawRect(Rect{ rect.x, rect.y + rect.height - thickness, rect.width, thickness }, separator);
    }
}

Color ResolveTabIconColor(bool isActive, float hoverAnim) {
    if (isActive) {
        return we::runtime::kindui::ResolveColor(ColorToken::IconAccent);
    }
    return ResolveIconColor(IconColorRole::Secondary, hoverAnim);
}

Color ResolveTabTextColor(bool isActive, float hoverAnim) {
    if (isActive) {
        return we::runtime::kindui::ResolveColor(ColorToken::TextPrimary);
    }
    Color text = we::runtime::kindui::ResolveColor(ColorToken::TextSecondary);
    if (hoverAnim > 0.01f) {
        text = Color::Lerp(text, we::runtime::kindui::ResolveColor(ColorToken::TextPrimary), hoverAnim * 0.55f);
    }
    return text;
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
    return SearchRowHeight();
}

float ColumnHeaderRowHeight() {
    return ListRowHeight();
}

float FooterRowHeight() {
    return ListRowHeight();
}

float TabPadH() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space2) * UiScale();
}

float TabStripPadH() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabStripPadH) * UiScale();
}

float TabIconSize() {
    return static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar)) * UiScale();
}

float TabGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabGap) * UiScale();
}

float TabTopRadius() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall) * UiScale();
}

float HeaderButtonSize() {
    return we::runtime::kindui::ResolveMetric(MetricToken::IconButtonSize) * UiScale();
}

void PaintPanelSurface(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, we::runtime::kindui::ds::Surface::Panel());
}

void PaintToolbarRegion(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, we::runtime::kindui::ds::Surface::Toolbar());
    PaintSeparatorEdge(context, rect, false);
}

void PaintHeaderRegion(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, we::runtime::kindui::ds::Surface::Header());
    PaintSeparatorEdge(context, rect, false);
}

void PaintFooterRegion(PaintContext& context, const Rect& rect) {
    PaintSeparatorEdge(context, rect, true);
    const float scale = UiScale();
    const float thickness = we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth) * scale;
    context.DrawRect(
        Rect{ rect.x, rect.y + thickness, rect.width, rect.height - thickness },
        we::runtime::kindui::ds::Surface::Header());
}

void PaintContentWell(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, we::runtime::kindui::ds::Panel::ContentWellBackground());
}

void PaintPrimaryContentRegion(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, we::runtime::kindui::ds::Panel::PrimaryContentBackground());
}

void PaintNavigationRegion(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, we::runtime::kindui::ds::Panel::NavigationBackground());
}

void PaintContentRegion(PaintContext& context, const Rect& rect) {
    PaintContentWell(context, rect);
}

void PaintDockTabStripDivider(PaintContext& context, const Rect& headerRect) {
    PaintSeparatorEdge(context, headerRect, false);
}

void PaintDockHeaderBand(PaintContext& context, const Rect& headerRect) {
    PaintDockTabStripDivider(context, headerRect);
}

float MeasureDockTabWidth(
    PaintContext& context,
    const DockTabDescriptor& tab,
    bool isActive,
    bool showClose,
    bool flushLeft)
{
    (void)flushLeft;
    const float scale = UiScale();
    const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeTabs) * scale;
    const float iconSize = TabIconSize();
    const float padH = TabPadH();
    const float iconGap = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    const float closeGap = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    const float buttonSize = HeaderButtonSize();

    float leadingWidth = 0.0f;
    if (tab.hasBrand) {
        leadingWidth = tab.brandLogicalSize * scale + iconGap;
    } else if (!tab.iconName.empty()) {
        leadingWidth = iconSize + iconGap;
    }

    const float textWidth = context.GetTextWidth(tab.title, fontSize, isActive);
    const float closeWidth = showClose ? buttonSize + closeGap : 0.0f;
    return padH + leadingWidth + textWidth + closeWidth + padH;
}

DockTabLayout LayoutDockTabGeometries(
    PaintContext& context,
    const DockTabDescriptor& tab,
    const Rect& headerRect,
    float x,
    bool isActive,
    bool showClose)
{
    const float scale = UiScale();
    const float padH = TabPadH();
    const float buttonSize = HeaderButtonSize();
    const float dividerH = we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth) * scale;
    const float tabHeight = (std::max)(16.0f, headerRect.height - dividerH);
    const float tabY = headerRect.y;
    const float centerY = std::floor(tabY + tabHeight * 0.5f);

    DockTabLayout layout{};
    const float tabWidth = MeasureDockTabWidth(context, tab, isActive, showClose);
    layout.tabRect = Rect{ x, tabY, tabWidth, tabHeight };

    if (showClose) {
        const float closeX = layout.tabRect.x + layout.tabRect.width - padH - buttonSize;
        const float closeY = std::floor(centerY - buttonSize * 0.5f);
        layout.closeRect = Rect{ closeX, closeY, buttonSize, buttonSize };
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
    bool flushLeft)
{
    (void)flushLeft;
    (void)headerRect;
    const float scale = UiScale();
    const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeTabs) * scale;
    const float iconSize = TabIconSize();
    const float padH = TabPadH();
    const float iconGap = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    const float radius = TabTopRadius();

    if (isActive) {
        Rect activeRect = layout.tabRect;
        activeRect.height = headerRect.height;
        DrawRoundedRectTop(context, activeRect, we::runtime::kindui::ds::Tab::ActiveBackground(), radius);
    } else if (hoverAnim > 0.01f) {
        Color tabBg = Color::Lerp(
            Color::Transparent(),
            we::runtime::kindui::ds::Tab::InactiveBackground(),
            hoverAnim * 0.85f);
        if (tabBg.a > 0.01f) {
            context.DrawRoundedRect(layout.tabRect, tabBg, radius);
        }
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
    } else if (!tab.iconName.empty()) {
        IconPainter::DrawIcon(
            context,
            tab.iconName,
            Rect{ itemX, std::floor(centerY - iconSize * 0.5f), iconSize, iconSize },
            ResolveTabIconColor(isActive, hoverAnim));
        itemX += iconSize + iconGap;
    }

    const float titleY = std::floor(centerY - fontSize * 0.5f);
    context.DrawText(
        tab.title,
        Point{ itemX, titleY },
        ResolveTabTextColor(isActive, hoverAnim),
        fontSize,
        isActive);

    if (showClose && !layout.closeRect.IsEmpty()) {
        PaintHeaderIconButton(context, layout.closeRect, Icons::XName, closeHovered, false, true);
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
    bool flushLeft)
{
    DockTabLayout layout = LayoutDockTabGeometries(context, tab, headerRect, x, isActive, showClose);
    PaintDockTab(context, tab, layout, headerRect, isActive, hoverAnim, showClose, closeHovered, flushLeft);
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
    const std::string& iconName,
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
    descriptor.iconName = iconName;
    descriptor.hasBrand = hasBrand;
    descriptor.brandDescriptor = brandDescriptor;
    descriptor.brandLogicalSize = brandLogicalSize;

    bool showClose = false;
    bool closeHovered = false;
    for (const auto& action : actions) {
        if (action.iconName == Icons::XName) {
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
        PaintHeaderIconButton(context, outOptionsMenuRect, Icons::MoreName, optionsMenuHovered, false);
        actionX += buttonSize + gap;
    }

    for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
        const auto& action = *it;
        if (action.iconName == Icons::XName) {
            continue;
        }
        Rect actionRect{ actionX, std::floor(centerY - buttonSize * 0.5f), buttonSize, buttonSize };
        PaintHeaderIconButton(context, actionRect, action.iconName, action.hovered, action.pressed, true);
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

    float x = stripRect.x + TabStripPadH();
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
            showClose);
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
            i == 0);
    }
    PaintDockTabStripDivider(context, stripRect);
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
    options.showClearButton = !text.empty();
    ControlChrome::PaintSearchField(context, rect, placeholder, text, state, showCaret, options);
}

void PaintListRowBackground(PaintContext& context, const Rect& rowRect, bool hovered, bool selected) {
    Color bg{ 0.0f, 0.0f, 0.0f, 0.0f };
    if (selected) {
        bg = we::runtime::kindui::ResolveColor(ColorToken::SelectionHighlight);
    } else if (hovered) {
        bg = we::runtime::kindui::ResolveColor(ColorToken::HoverBackground);
    }
    if (bg.a > 0.001f) {
        context.DrawRect(rowRect, bg);
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
    const std::string& iconName,
    bool hovered,
    bool pressed,
    bool compactGlyph)
{
    const float scale = UiScale();
    const float radius = we::runtime::kindui::ResolveMetric(MetricToken::IconButtonRadius) * scale;

    if (pressed) {
        context.DrawRoundedRect(rect, we::runtime::kindui::ResolveColor(ColorToken::PressedBackground), radius);
    } else if (hovered) {
        context.DrawRoundedRect(rect, we::runtime::kindui::ResolveColor(ColorToken::HoverBackground), radius);
    }

    const float emphasis = (hovered || pressed) ? 1.0f : 0.0f;
    Color iconColor = ResolveIconColor(IconColorRole::Secondary, emphasis, pressed ? 1.0f : 0.0f);

    if (compactGlyph) {
        IconPainter::DrawCompactIcon(context, iconName, rect, iconColor);
    } else {
        const float iconSize = static_cast<float>(TabIconSize());
        const Rect iconRect = IconMetrics::PlaceGlyphCentered(rect, iconSize);
        IconPainter::DrawIcon(context, iconName, iconRect, iconColor);
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
