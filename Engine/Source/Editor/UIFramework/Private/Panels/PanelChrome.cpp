#include "WindEffects/Editor/UI/Panel/PanelChrome.h"

#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeColors.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include <algorithm>
#include <cmath>

namespace we::runtime::kindui::PanelChrome {
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
        return ResolveColor(ColorToken::IconAccent);
    }
    return ResolveIconColor(IconColorRole::Secondary, hoverAnim);
}

Color ResolveTabTextColor(bool isActive, float hoverAnim) {
    if (isActive) {
        return ResolveColor(ColorToken::TextPrimary);
    }
    Color text = ResolveColor(ColorToken::TextSecondary);
    if (hoverAnim > 0.01f) {
        text = Color::Lerp(text, ResolveColor(ColorToken::TextPrimary), hoverAnim * 0.55f);
    }
    return text;
}

} // namespace

float UiScale() {
    return (std::max)(1.0f, DPIContext::GetScale());
}

float TabHeight() {
    return ResolveMetric(MetricToken::PanelTabHeight) * UiScale();
}

float ToolbarHeight() {
    return ResolveMetric(MetricToken::PanelToolbarHeight) * UiScale();
}

float SearchHeight() {
    return ResolveMetric(MetricToken::SearchBoxHeight) * UiScale();
}

float ListRowHeight() {
    return ResolveMetric(MetricToken::ListRowHeight) * UiScale();
}

float PanelPaddingH() {
    return ResolveMetric(MetricToken::Space2) * UiScale();
}

float CategoryHeaderHeight() {
    return ResolveMetric(MetricToken::CategoryHeaderHeight) * UiScale();
}

float TabPadH() {
    return ResolveMetric(MetricToken::Space2) * UiScale();
}

float TabIconSize() {
    return static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar)) * UiScale();
}

float TabGap() {
    return 2.0f * UiScale();
}

float TabTopRadius() {
    return 4.0f * UiScale();
}

float HeaderButtonSize() {
    return ResolveMetric(MetricToken::IconButtonSize) * UiScale();
}

void PaintPanelSurface(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, ResolveColor(ColorToken::PanelBackground));
}

void PaintToolbarRegion(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, ResolveColor(ColorToken::PanelToolbarBackground));

    const float scale = UiScale();
    Rect separator{
        rect.x,
        rect.y + rect.height - 1.0f * scale,
        rect.width,
        1.0f * scale
    };
    context.DrawRect(separator, ResolveColor(ColorToken::Separator));
}

void PaintContentRegion(PaintContext& context, const Rect& rect) {
    context.DrawRect(rect, ResolveColor(ColorToken::PanelBackground));
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
    bool flushLeft)
{
    (void)flushLeft;
    const float scale = UiScale();
    const float fontSize = ResolveMetric(MetricToken::TextSizeTabs) * scale;
    const float iconSize = TabIconSize();
    const float padH = TabPadH();
    const float iconGap = ResolveMetric(MetricToken::Space1) * scale;
    const float closeGap = ResolveMetric(MetricToken::Space1) * scale;
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
    DockTabLayout layout{};
    (void)flushLeft;
    const float scale = UiScale();
    const float fontSize = ResolveMetric(MetricToken::TextSizeTabs) * scale;
    const float iconSize = TabIconSize();
    const float padH = TabPadH();
    const float iconGap = ResolveMetric(MetricToken::Space1) * scale;
    const float radius = TabTopRadius();
    const float buttonSize = HeaderButtonSize();

    const float tabWidth = MeasureDockTabWidth(context, tab, isActive, showClose);
    layout.tabRect = Rect{ x, headerRect.y, tabWidth, headerRect.height };

    if (isActive) {
        DrawRoundedRectTop(context, layout.tabRect, ResolveColor(ColorToken::PanelBackground), radius);
    } else if (hoverAnim > 0.01f) {
        Color hoverBg = Color::Lerp(
            Color{0.0f, 0.0f, 0.0f, 0.0f},
            ResolveColor(ColorToken::HoverBackground),
            hoverAnim * 0.85f);
        DrawRoundedRectTop(context, layout.tabRect, hoverBg, radius);
    }

    float itemX = layout.tabRect.x + padH;
    const float centerY = headerRect.y + headerRect.height * 0.5f;

    if (tab.hasBrand) {
        const float brandSize = tab.brandLogicalSize * scale;
        const float logoY = centerY - brandSize * 0.5f;
        const auto snap = [](float v) { return std::floor(v + 0.5f); };
        if (tab.brandDescriptor != we::rhi::RHIDescriptorSetHandle::Invalid) {
            context.DrawTexture(
                Rect{ snap(itemX), snap(logoY), brandSize, brandSize },
                tab.brandDescriptor,
                ResolveColor(ColorToken::TextPrimary));
        }
        itemX += brandSize + iconGap;
    } else if (!tab.iconName.empty()) {
        IconPainter::DrawIcon(
            context,
            tab.iconName,
            Rect{ itemX, centerY - iconSize * 0.5f, iconSize, iconSize },
            ResolveTabIconColor(isActive, hoverAnim));
        itemX += iconSize + iconGap;
    }

    const float titleY = centerY - fontSize * 0.5f;
    context.DrawText(
        tab.title,
        Point{ itemX, titleY },
        ResolveTabTextColor(isActive, hoverAnim),
        fontSize,
        isActive);

    if (showClose) {
        const float closeX = layout.tabRect.x + layout.tabRect.width - padH - buttonSize;
        const float closeY = centerY - buttonSize * 0.5f;
        layout.closeRect = Rect{ closeX, closeY, buttonSize, buttonSize };
        PaintHeaderIconButton(context, layout.closeRect, Icons::XName, closeHovered, false, true);
    }

    return layout;
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
    const float fontSize = ResolveMetric(MetricToken::TextSizeTabs) * scale;
    const float iconSize = TabIconSize();
    const float padH = TabPadH();
    const float iconGap = ResolveMetric(MetricToken::Space1) * scale;
    const float radius = TabTopRadius();
    const float buttonSize = HeaderButtonSize();

    Color headerBg = ResolveColor(ColorToken::PanelContentBackground);
    DrawRoundedRectTop(context, headerRect, headerBg, radius);

    Rect topSheen{ headerRect.x, headerRect.y, headerRect.width, 1.0f * scale };
    context.DrawRect(topSheen, ResolveColor(ColorToken::HighlightSubtle));

    Rect separator{
        headerRect.x,
        headerRect.y + headerRect.height - 1.0f * scale,
        headerRect.width,
        1.0f * scale
    };
    context.DrawRect(separator, ResolveColor(ColorToken::Separator));

    const float centerY = headerRect.y + headerRect.height * 0.5f;
    float itemX = headerRect.x + padH;

    if (hasBrand) {
        const float brandSize = brandLogicalSize * scale;
        if ((brandDescriptor != we::rhi::RHIDescriptorSetHandle::Invalid)) {
            context.DrawTexture(
                Rect{ itemX, centerY - brandSize * 0.5f, brandSize, brandSize },
                brandDescriptor,
                ResolveColor(ColorToken::TextPrimary));
        }
        itemX += brandSize + iconGap;
    } else if (!iconName.empty()) {
        IconPainter::DrawIcon(
            context,
            iconName,
            Rect{ itemX, centerY - iconSize * 0.5f, iconSize, iconSize },
            ResolveColor(ColorToken::IconActive));
        itemX += iconSize + iconGap;
    }

    context.DrawText(
        title,
        Point{ itemX, centerY - fontSize * 0.5f },
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        true);

    const float optionsX = headerRect.x + headerRect.width - padH - buttonSize;
    outOptionsMenuRect = {};
    float actionX = headerRect.x + headerRect.width - padH;
    if (showOptionsMenu) {
        outOptionsMenuRect = Rect{ optionsX, centerY - buttonSize * 0.5f, buttonSize, buttonSize };
        PaintHeaderIconButton(context, outOptionsMenuRect, Icons::MoreName, optionsMenuHovered, false);
        actionX = optionsX - ResolveMetric(MetricToken::Space1) * scale - buttonSize;
    }

    for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
        const auto& action = *it;
        Rect actionRect{ actionX, centerY - buttonSize * 0.5f, buttonSize, buttonSize };
        PaintHeaderIconButton(context, actionRect, action.iconName, action.hovered, action.pressed, false);
        actionX -= buttonSize + ResolveMetric(MetricToken::Space1) * scale;
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
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * scale;
    const float fontSize = ResolveMetric(MetricToken::TextSizeBody) * scale;
    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(ResolveMetric(MetricToken::IconSizeSearch)));
    const float padH = TabPadH();

    Color bg = ResolveColor(ColorToken::SearchBoxBackground);
    if (focused) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::HoverBackground), 0.35f);
    }
    context.DrawRoundedRect(rect, bg, radius);

    const float iconX = rect.x + padH;
    Rect iconBand{ iconX, rect.y, iconSize, rect.height };
    IconPainter::DrawIcon(
        context,
        Icons::SearchName,
        IconMetrics::PlaceGlyphCentered(iconBand, iconSize),
        ResolveColor(ColorToken::IconSecondary));

    const float textX = iconX + iconSize + ResolveMetric(MetricToken::Space1) * scale;
    const float textY = rect.y + (rect.height - fontSize) * 0.5f;

    if (text.empty()) {
        context.DrawText(placeholder, Point{ textX, textY }, ResolveColor(ColorToken::SearchPlaceholder), fontSize);
    } else {
        context.DrawText(text, Point{ textX, textY }, ResolveColor(ColorToken::TextPrimary), fontSize);
        if (focused && showCaret) {
            const float caretX = textX + context.GetTextWidth(text, fontSize);
            context.DrawRect(
                Rect{ caretX, textY, ResolveMetric(MetricToken::BorderWidth), fontSize },
                ResolveColor(ColorToken::TextPrimary));
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
        bg = ResolveColor(ColorToken::SelectionHighlight);
    } else if (hovered) {
        bg = ResolveColor(ColorToken::HoverBackground);
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
    const float fontSize = ResolveMetric(MetricToken::TextSizeCategory) * scale;
    const float centerY = rect.y + rect.height * 0.5f;

    if (hovered) {
        context.DrawRect(rect, ResolveColor(ColorToken::HoverBackground));
    }

    const char* chevronIcon = expanded ? Icons::ChevronDownName : Icons::ChevronRightName;
    IconPainter::DrawIcon(
        context,
        chevronIcon,
        Rect{ rect.x + padH, centerY - chevronSize * 0.5f, chevronSize, chevronSize },
        ResolveColor(ColorToken::TextSecondary));

    const float textX = rect.x + padH + chevronSize + ResolveMetric(MetricToken::Space1) * scale;
    context.DrawText(title, Point{ textX, centerY - fontSize * 0.5f }, ResolveColor(ColorToken::TextSecondary), fontSize, true);
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
    const float radius = ResolveMetric(MetricToken::IconButtonRadius) * scale;

    if (pressed) {
        context.DrawRoundedRect(rect, ResolveColor(ColorToken::PressedBackground), radius);
    } else if (hovered) {
        context.DrawRoundedRect(rect, ResolveColor(ColorToken::HoverBackground), radius);
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

void PaintToolbarIconButton(PaintContext& context, const Rect& rect, const std::string& iconName, bool hovered, bool pressed) {
    PaintHeaderIconButton(context, rect, iconName, hovered, pressed, false);
}

Rect InsetSearchRect(const Rect& toolbarRect, float searchWidth) {
    const float padH = ResolveMetric(MetricToken::Space2);
    const float searchH = ResolveMetric(MetricToken::SearchBoxHeight);
    const float searchY = toolbarRect.y + (toolbarRect.height - searchH) * 0.5f;
    return Rect{ toolbarRect.x + padH, searchY, searchWidth, searchH };
}

} // namespace we::runtime::kindui::PanelChrome
