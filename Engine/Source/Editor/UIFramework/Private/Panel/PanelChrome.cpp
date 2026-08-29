#include "WindEffects/Editor/UI/Panel/PanelChrome.h"

#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeColors.h"
#include "KindUI/Tokens/DesignToken.h"
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
    return SearchHeight() + PanelPaddingV() * 2.0f;
}

float ToolbarRowHeight() {
    return ToolbarHeight() + PanelPaddingV() * 2.0f;
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

float TabIconSize() {
    return static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar)) * UiScale();
}

float TabGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TabGap) * UiScale();
}

float TabTopRadius() {
    return 0.0f;
}

float HeaderButtonSize() {
    return we::runtime::kindui::ResolveMetric(MetricToken::IconButtonSize) * UiScale();
}

void PaintPanelSurface(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, we::runtime::kindui::ResolveColor(ColorToken::PanelBackground));
}

void PaintToolbarRegion(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, we::runtime::kindui::ResolveColor(ColorToken::ToolbarBackground));

    const float scale = UiScale();
    Rect separator{
        rect.x,
        rect.y + rect.height - 1.0f * scale,
        rect.width,
        1.0f * scale
    };
    context.DrawRect(separator, we::runtime::kindui::ResolveColor(ColorToken::Separator));
}

void PaintContentRegion(PaintContext& context, const Rect& rect) {
    PaintPanelSurface(context, rect);
}

void PaintDockHeaderBand(PaintContext& context, const Rect& headerRect) {
    const float scale = UiScale();
    const float separatorHeight = we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth) * scale;
    context.DrawRect(
        Rect{ headerRect.x, headerRect.y + headerRect.height - separatorHeight, headerRect.width, separatorHeight },
        we::runtime::kindui::ResolveColor(ColorToken::Separator));
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
    const float insetTop = 2.0f * scale;
    const float tabHeight = (std::max)(16.0f, headerRect.height - insetTop);
    const float tabY = headerRect.y + insetTop;
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
        Color activeBg = we::runtime::kindui::ResolveColor(ColorToken::PanelBackground);
        DrawRoundedRectTop(context, layout.tabRect, activeBg, radius);

        const float indicatorHeight = we::runtime::kindui::ResolveMetric(MetricToken::TabActiveIndicatorHeight) * scale;
        context.DrawRect(
            Rect{ layout.tabRect.x, layout.tabRect.y, layout.tabRect.width, indicatorHeight },
            we::runtime::kindui::ResolveColor(ColorToken::ActiveTabLine));
    } else if (hoverAnim > 0.01f) {
        Color tabBg = Color::Lerp(
            Color::Transparent(),
            we::runtime::kindui::ResolveColor(ColorToken::HoverBackground),
            hoverAnim * 0.85f);
        DrawRoundedRectTop(context, layout.tabRect, tabBg, radius);
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
    const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeTabs) * scale;
    const float iconSize = TabIconSize();
    const float padH = TabPadH();
    const float iconGap = we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    const float radius = TabTopRadius();
    const float buttonSize = HeaderButtonSize();

    Color headerBg = we::runtime::kindui::ResolveColor(ColorToken::HeaderBackground);
    DrawRoundedRectTop(context, headerRect, headerBg, radius);

    const float indicatorHeight = we::runtime::kindui::ResolveMetric(MetricToken::TabActiveIndicatorHeight) * scale;
    context.DrawRect(
        Rect{ headerRect.x, headerRect.y, headerRect.width, indicatorHeight },
        we::runtime::kindui::ResolveColor(ColorToken::ActiveTabLine));

    Rect separator{
        headerRect.x,
        headerRect.y + headerRect.height - 1.0f * scale,
        headerRect.width,
        1.0f * scale
    };
    context.DrawRect(separator, we::runtime::kindui::ResolveColor(ColorToken::Separator));

    const float centerY = std::floor(headerRect.y + headerRect.height * 0.5f);
    float itemX = headerRect.x + padH;

    if (hasBrand) {
        const float brandSize = brandLogicalSize * scale;
        if ((brandDescriptor != we::rhi::RHIDescriptorSetHandle::Invalid)) {
            context.DrawTexture(
                Rect{ itemX, std::floor(centerY - brandSize * 0.5f), brandSize, brandSize },
                brandDescriptor,
                we::runtime::kindui::ResolveColor(ColorToken::TextPrimary));
        }
        itemX += brandSize + iconGap;
    } else if (!iconName.empty()) {
        IconPainter::DrawIcon(
            context,
            iconName,
            Rect{ itemX, std::floor(centerY - iconSize * 0.5f), iconSize, iconSize },
            we::runtime::kindui::ResolveColor(ColorToken::IconActive));
        itemX += iconSize + iconGap;
    }

    context.DrawText(
        title,
        Point{ itemX, std::floor(centerY - fontSize * 0.5f) },
        we::runtime::kindui::ResolveColor(ColorToken::TextPrimary),
        fontSize,
        true);

    const float optionsX = headerRect.x + headerRect.width - padH - buttonSize;
    outOptionsMenuRect = {};
    float actionX = headerRect.x + headerRect.width - padH;
    if (showOptionsMenu) {
        outOptionsMenuRect = Rect{ optionsX, std::floor(centerY - buttonSize * 0.5f), buttonSize, buttonSize };
        PaintHeaderIconButton(context, outOptionsMenuRect, Icons::MoreName, optionsMenuHovered, false);
        actionX = optionsX - we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale - buttonSize;
    }

    for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
        const auto& action = *it;
        Rect actionRect{ actionX, std::floor(centerY - buttonSize * 0.5f), buttonSize, buttonSize };
        PaintHeaderIconButton(context, actionRect, action.iconName, action.hovered, action.pressed, false);
        actionX -= buttonSize + we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
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
    const float scale = UiScale();
    const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeSmall) * scale;
    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(we::runtime::kindui::ResolveMetric(MetricToken::IconSizeSearch)));
    const float padH = (std::max)(8.0f, rect.height * 0.5f * 0.65f);

    ControlChrome::InteractionState state{};
    state.focused = focused;
    ControlChrome::PaintSearchInputFrame(context, rect, state);

    const float iconX = rect.x + padH;
    Rect iconBand{ iconX, rect.y, iconSize, rect.height };
    IconPainter::DrawIcon(
        context,
        Icons::SearchName,
        IconMetrics::PlaceGlyphCentered(iconBand, iconSize),
        we::runtime::kindui::ResolveColor(ColorToken::IconSecondary));

    const float textX = iconX + iconSize + we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    const float textY = rect.y + (rect.height - fontSize) * 0.5f;

    if (text.empty()) {
        context.DrawText(placeholder, Point{ textX, textY }, we::runtime::kindui::ResolveColor(ColorToken::SearchPlaceholder), fontSize);
    } else {
        context.DrawText(text, Point{ textX, textY }, we::runtime::kindui::ResolveColor(ColorToken::TextPrimary), fontSize);
        if (focused && showCaret) {
            const float caretX = textX + context.GetTextWidth(text, fontSize);
            context.DrawRect(
                Rect{ caretX, textY, we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth), fontSize },
                we::runtime::kindui::ResolveColor(ColorToken::TextPrimary));
        }

        const float clearSize = iconSize;
        const float clearX = rect.x + rect.width - clearSize - padH;
        const float clearY = rect.y + (rect.height - clearSize) * 0.5f;
        PaintHeaderIconButton(context, Rect{ clearX, clearY, clearSize, clearSize }, Icons::XName, false, false, false);
    }
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
    const float scale = UiScale();
    const float padH = PanelPaddingH() + indent;
    const float chevronSize = static_cast<float>(IconMetrics::StandardGlyphTierPx());
    const float fontSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeCategory) * scale;
    const float centerY = rect.y + rect.height * 0.5f;

    if (hovered) {
        context.DrawRect(rect, we::runtime::kindui::ResolveColor(ColorToken::HoverBackground));
    }

    const char* chevronIcon = expanded ? Icons::ChevronDownName : Icons::ChevronRightName;
    IconPainter::DrawIcon(
        context,
        chevronIcon,
        Rect{ rect.x + padH, centerY - chevronSize * 0.5f, chevronSize, chevronSize },
        we::runtime::kindui::ResolveColor(ColorToken::TextSecondary));

    const float textX = rect.x + padH + chevronSize + we::runtime::kindui::ResolveMetric(MetricToken::Space1) * scale;
    context.DrawText(title, Point{ textX, centerY - fontSize * 0.5f }, we::runtime::kindui::ResolveColor(ColorToken::TextSecondary), fontSize, true);
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
