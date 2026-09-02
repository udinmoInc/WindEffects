#include "Widgets/ToolButton.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>
#include <cctype>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;

namespace we::editor::toolbar {
using ::we::runtime::kindui::Animator;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::WindIconRef;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace ToolbarButtonChrome = ::we::runtime::kindui::ToolbarButtonChrome;

namespace {
    using namespace ToolbarButtonChrome;

    constexpr float kChevronSlotPx = 16.0f;

    float PressStrength(bool pressed, float pressAnim) {
        return pressed ? 1.0f : pressAnim;
    }

    float HoverDamping() {
        return we::runtime::kindui::ResolveMetric(MetricToken::HoverAnimationDamping);
    }

    float PressDamping() {
        return we::runtime::kindui::ResolveMetric(MetricToken::PressAnimationDamping);
    }

    Color ResolveInteractiveTextColor(float hoverAnim, float pressStrength, bool active) {
        return we::runtime::kindui::ResolveTextForState(hoverAnim > 0.01f || pressStrength > 0.01f, active);
    }

    float ApproxInlineTextWidth(const std::string& text, float textSize) {
        float width = 0.0f;
        for (unsigned char ch : text) {
            if (std::isspace(ch)) {
                width += textSize * 0.32f;
            } else if (std::isdigit(ch)) {
                width += textSize * 0.54f;
            } else if (std::isupper(ch)) {
                width += textSize * 0.58f;
            } else {
                switch (ch) {
                    case 'i': case 'l': case 't': case 'f': case 'r': case 'j':
                    case '.': case ',': case ':': case ';': case '!': case '|':
                        width += textSize * 0.30f;
                        break;
                    case 'm': case 'w':
                        width += textSize * 0.75f;
                        break;
                    default:
                        width += textSize * 0.52f;
                        break;
                }
            }
        }
        return width;
    }

    bool IconsMatch(WindIconRef a, WindIconRef b) {
        if (!a.IsValid() || !b.IsValid()) {
            return false;
        }
        return a.sizePx == b.sizePx && std::string_view(a.stem) == b.stem;
    }
}

ToolButton::ToolButton(WindIconRef icon, const std::string& label, std::function<void()> onClicked, const std::string& tooltip)
    : m_Icon(icon)
    , m_Label(label)
    , m_Tooltip(tooltip)
    , m_OnClicked(onClicked)
    , m_Style(WidgetStyle::ToolButton())
{}

Size ToolButton::Measure(const Size& availableSize) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    if (m_ButtonStyle == ToolButtonStyle::WindowControl || m_ButtonStyle == ToolButtonStyle::WindowClose) {
        const float controlWidth = ThemeMetric(MetricToken::WindowControlWidth) * uiScale;
        const float controlHeight = ThemeMetric(MetricToken::TitleBarHeight) * uiScale;
        m_DesiredSize = Size{ controlWidth, controlHeight };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        const float controlSize = ThemeMetric(MetricToken::IconButtonSize) * uiScale;
        m_DesiredSize = Size{ controlSize, controlSize };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::StatusBar) {
        const float padH = ChipHorizontalPad(uiScale);
        const float iconSz = IconSize(uiScale);
        const float iconGap = IconGapPx(uiScale);
        const float textSize = ThemeMetric(MetricToken::TextSizeSmall) * uiScale;
        const float controlH = ThemeMetric(MetricToken::StatusBarHeight) * uiScale;
        const bool hasIcon = m_Icon.IsValid();

        float textW = m_Label.empty() ? 0.0f : ApproxInlineTextWidth(m_Label, textSize);
        float width = padH * 2.0f;
        if (hasIcon) {
            width += iconSz;
            if (!m_Label.empty()) {
                width += iconGap;
            }
        }
        width += textW;
        m_DesiredSize = Size{ width, controlH };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarInline) {
        const float padH     = ChipHorizontalPad(uiScale);
        const float iconSz   = IconSize(uiScale);
        const float iconGap  = IconGapPx(uiScale);
        const float chevGap  = ChevronGapPx(uiScale);
        const float chevW    = kChevronSlotPx;
        const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
        const float controlH = ToolbarButtonChrome::RowContentHeight(uiScale);
        const bool hasIcon = m_Icon.IsValid();

        float textW = m_Label.empty() ? 0.0f : ApproxInlineTextWidth(m_Label, textSize);

        float width = padH * 2.0f;
        if (hasIcon) {
            width += iconSz;
            if (!m_Label.empty() || m_IsDropdown) {
                width += iconGap;
            }
        }
        width += textW;
        if (m_IsDropdown) {
            width += chevGap + chevW;
        }

        m_DesiredSize = Size{ width, controlH };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ViewportChip) {
        const float padH     = ChipHorizontalPad(uiScale);
        const float iconSz   = IconSize(uiScale);
        const float iconGap  = IconGapPx(uiScale);
        const float chevGap  = ChevronGapPx(uiScale);
        const float chevW    = kChevronSlotPx;
        const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
        const float controlH = ThemeMetric(MetricToken::ViewportToolbarHeight) * uiScale;
        const bool hasIcon = m_Icon.IsValid();

        float textW = m_Label.empty() ? 0.0f : ApproxInlineTextWidth(m_Label, textSize);

        float width = padH * 2.0f;
        if (hasIcon) {
            width += iconSz;
            if (!m_Label.empty() || m_IsDropdown) {
                width += iconGap;
            }
        }
        width += textW;
        if (m_IsDropdown) {
            width += chevGap + chevW;
        }

        m_DesiredSize = Size{ width, controlH };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarLabeled) {
        const float padH = ThemeMetric(MetricToken::Space2) * uiScale;
        const float textSize = ThemeMetric(MetricToken::TextSizeCaption) * uiScale;
        const float labelW = m_Label.empty() ? 0.0f : ApproxInlineTextWidth(m_Label, textSize);
        const float minW = ThemeMetric(MetricToken::ToolbarLabeledMinWidth) * uiScale;
        const float width = (std::max)(minW, labelW + padH * 2.0f);
        const float height = ThemeMetric(MetricToken::ToolbarLabeledHeight) * uiScale;
        m_DesiredSize = Size{ width, height };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::TransportButton || m_ButtonStyle == ToolButtonStyle::PlayButton || m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly) {
        const float controlSize = ToolbarButtonChrome::ItemSize(uiScale);
        m_DesiredSize = Size{ controlSize, controlSize };
        return m_DesiredSize;
    }

    const float height  = ToolbarButtonChrome::RowContentHeight(uiScale);
    const float padL    = ThemeMetric(MetricToken::ButtonPaddingHorizontal) * uiScale;
    const float padR    = ThemeMetric(MetricToken::Space2) * uiScale;
    const float iconSz  = IconSize(uiScale);
    const float iconGap = ThemeMetric(MetricToken::Space1) * uiScale;
    const float chevW   = kChevronSlotPx;

    float width = padL + iconSz;
    if (!m_Label.empty()) {
        width += iconGap + m_Label.length() * (7.2f * uiScale);
    }
    if (m_IsDropdown) {
        width += chevW;
    }
    width += padR;

    m_DesiredSize = Size{ width, height };
    return m_DesiredSize;
}

void ToolButton::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ToolButton::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, HoverDamping());
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, PressDamping());
    m_ActiveAnim = Animator::Damp(m_ActiveAnim, m_Active ? 1.0f : 0.0f, HoverDamping());

    const float pressStrength = PressStrength(m_Pressed, m_PressAnim);

    Rect renderRect = m_Geometry;
    float centerY   = renderRect.y + renderRect.height / 2.0f;

    const bool isWindowControl = (m_ButtonStyle == ToolButtonStyle::WindowControl ||
                                   m_ButtonStyle == ToolButtonStyle::WindowClose);
    const bool isToolbarIcon   = (m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly ||
                                   m_ButtonStyle == ToolButtonStyle::TransportButton ||
                                   m_ButtonStyle == ToolButtonStyle::PlayButton);
    const bool isLabeled       = (m_ButtonStyle == ToolButtonStyle::ToolbarLabeled);
    const bool isStatusBar     = (m_ButtonStyle == ToolButtonStyle::StatusBar);
    const bool isInline        = (m_ButtonStyle == ToolButtonStyle::ToolbarInline);
    const bool isViewportChip  = (m_ButtonStyle == ToolButtonStyle::ViewportChip);
    const bool isNormal        = (m_ButtonStyle == ToolButtonStyle::Normal);

    if (isWindowControl) {
        if (m_HoverAnim > 0.01f) {
            const Color base = ThemeColor(ColorToken::WindowBackground);
            const Color hover = (m_ButtonStyle == ToolButtonStyle::WindowClose)
                ? ThemeColor(ColorToken::CloseButtonHover)
                : ThemeColor(ColorToken::HoverBackground);
            context.DrawRect(renderRect, Color::Pick(base, hover, m_HoverAnim));
        }

        const float iconSize = WindowControlIconSize(uiScale);
        IconPainter::Draw(context, m_Icon, renderRect, static_cast<uint32_t>(iconSize));
        return;
    }

    if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        PaintIconButton(context, renderRect, m_HoverAnim, pressStrength, m_Active, m_ActiveAnim, uiScale);
        const float iconSize = IconSize(uiScale);
        IconPainter::Draw(context, m_Icon, PlaceIconInControl(renderRect, iconSize));
        return;
    }

    if (isLabeled) {
        PaintIconButton(context, renderRect, m_HoverAnim, pressStrength, m_Active, m_ActiveAnim, uiScale);

        const float iconSize = PrimaryIconSize(uiScale);
        const float textSize = ThemeMetric(MetricToken::TextSizeCaption) * uiScale;
        const float labelGap = 2.0f * uiScale;
        const float contentH = iconSize + labelGap + textSize;
        const float topY = renderRect.y + (renderRect.height - contentH) * 0.5f;

        Rect iconBand{ renderRect.x, topY, renderRect.width, iconSize };
        IconPainter::Draw(context, m_Icon, PlaceIconInControl(iconBand, iconSize));

        if (!m_Label.empty()) {
            const float labelW = ApproxInlineTextWidth(m_Label, textSize);
            const float labelX = renderRect.x + (renderRect.width - labelW) * 0.5f;
            const float labelY = topY + iconSize + labelGap;
            Color labelColor = m_Active
                ? ThemeColor(ColorToken::IconAccent)
                : ThemeColor(ColorToken::TextSecondary);
            if (m_HoverAnim > 0.01f && !m_Active) {
                labelColor = Color::Pick(labelColor, ThemeColor(ColorToken::TextPrimary), m_HoverAnim);
            }
            context.DrawText(m_Label, Point{ labelX, labelY }, labelColor, textSize);
        }
        return;
    }

    if (isStatusBar) {
        ToolbarButtonChrome::PaintStatusBarControl(context, renderRect, m_HoverAnim, m_Active, uiScale);

        const float iconSize = IconSize(uiScale);
        const float textSize = ThemeMetric(MetricToken::TextSizeSmall) * uiScale;
        const float iconGap = IconGapPx(uiScale);
        const float padH = ChipHorizontalPad(uiScale);

        float currentX = renderRect.x + padH;

        if (m_Icon.IsValid()) {
            Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
            IconPainter::Draw(context, m_Icon, PlaceIconInControl(iconBand, iconSize));
            currentX += iconSize + iconGap;
        }

        if (!m_Label.empty()) {
            Color textColor = m_Active
                ? ThemeColor(ColorToken::TextPrimary)
                : Color::Pick(
                    ThemeColor(ColorToken::TextPrimary),
                    ThemeColor(ColorToken::TextSecondary),
                    0.18f);
            if (m_HoverAnim > 0.01f && !m_Active) {
                textColor = Color::Pick(textColor, ThemeColor(ColorToken::TextPrimary), m_HoverAnim * 0.35f);
            }
            context.DrawText(m_Label, Point{ currentX, centerY - textSize * 0.5f }, textColor, textSize);
        }
        return;
    }

    if (isInline) {
        if (!m_Chromeless) {
            PaintInlineDropdown(context, renderRect, m_HoverAnim, pressStrength, uiScale);
        } else {
            PaintIconButton(context, renderRect, m_HoverAnim, pressStrength, m_Active, m_ActiveAnim, uiScale);
        }

        const float iconSize  = IconSize(uiScale);
        const float textSize  = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
        const float iconGap   = IconGapPx(uiScale);
        const float chevGap   = ChevronGapPx(uiScale);
        const float padH      = ChipHorizontalPad(uiScale);

        float currentX = renderRect.x + padH;
        if (m_Icon.IsValid()) {
            Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
            IconPainter::Draw(context, m_Icon, PlaceIconInControl(iconBand, iconSize));
            currentX += iconSize + iconGap;
        }

        if (!m_Label.empty()) {
            Color textColor = ResolveInteractiveTextColor(m_HoverAnim, pressStrength, false);
            context.DrawText(m_Label, Point{ currentX, centerY - textSize * 0.5f }, textColor, textSize);
            currentX += ApproxInlineTextWidth(m_Label, textSize);
        }

        if (m_IsDropdown) {
            currentX += chevGap;
            IconPainter::Draw(context, WindIcons::ChevronDown16, IconMetrics::CompactGlyphBand(renderRect, currentX));
        }
        return;
    }

    if (isViewportChip) {
        PaintViewportChip(context, renderRect, m_HoverAnim, pressStrength, uiScale);

        const float iconSize  = NavigationIconSize(uiScale);
        const float textSize  = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
        const float iconGap   = IconGapPx(uiScale);
        const float chevGap   = ChevronGapPx(uiScale);
        const float padH      = ChipHorizontalPad(uiScale);

        float currentX = renderRect.x + padH;
        if (m_Icon.IsValid()) {
            Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
            IconPainter::Draw(context, m_Icon, PlaceIconInControl(iconBand, iconSize));
            currentX += iconSize + iconGap;
        }

        if (!m_Label.empty()) {
            Color textColor = ResolveInteractiveTextColor(m_HoverAnim, pressStrength, m_Active);
            context.DrawText(m_Label, Point{ currentX, centerY - textSize * 0.5f }, textColor, textSize);
            currentX += ApproxInlineTextWidth(m_Label, textSize);
        }

        if (m_IsDropdown) {
            currentX += chevGap;
            IconPainter::Draw(context, WindIcons::ChevronDown16, IconMetrics::CompactGlyphBand(renderRect, currentX));
        }
        return;
    }

    if (isToolbarIcon) {
        PaintIconButton(context, renderRect, m_HoverAnim, pressStrength, m_Active, m_ActiveAnim, uiScale);

        const bool isTransport = (m_ButtonStyle == ToolButtonStyle::TransportButton
            || m_ButtonStyle == ToolButtonStyle::PlayButton);
        const float iconSize = isTransport ? PrimaryIconSize(uiScale) : IconSize(uiScale);
        IconPainter::Draw(context, m_Icon, PlaceIconInControl(renderRect, iconSize));
        return;
    }

    if (isNormal) {
        PaintInlineDropdown(context, renderRect, m_HoverAnim, pressStrength, uiScale);

        const float iconSize = IconSize(uiScale);
        float currentX = renderRect.x + ThemeMetric(MetricToken::ButtonPaddingHorizontal) * uiScale;

        if (m_Icon.IsValid()) {
            if (m_Label.empty() && !m_IsDropdown) {
                IconPainter::Draw(context, m_Icon, PlaceIconInControl(renderRect, iconSize));
            } else if (m_Label.empty() && m_IsDropdown) {
                currentX = renderRect.x + ChipHorizontalPad(uiScale);
                Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
                IconPainter::Draw(context, m_Icon, PlaceIconInControl(iconBand, iconSize));
            } else {
                Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
                IconPainter::Draw(context, m_Icon, PlaceIconInControl(iconBand, iconSize));
                currentX += iconSize + IconGapPx(uiScale);
            }
        } else {
            currentX = renderRect.x + ThemeMetric(MetricToken::ButtonPaddingHorizontal) * uiScale;
        }

        if (!m_Label.empty()) {
            const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
            Color textColor = ToolbarButtonChrome::ResolveIconColor(m_HoverAnim, pressStrength, m_Active);
            context.DrawText(m_Label, Point{ currentX, centerY - textSize / 2.0f }, textColor, textSize);
        }

        if (m_IsDropdown) {
            const float chevronX = renderRect.x + renderRect.width - ChipHorizontalPad(uiScale) - kChevronSlotPx;
            IconPainter::Draw(context, WindIcons::ChevronDown16, IconMetrics::CompactGlyphBand(renderRect, chevronX));
        }
    }
}

void ToolButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = true;
        m_PressAnim = 1.0f;
    }
}

void ToolButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void ToolButton::OnMouseMove(const MouseEvent& event) {
    (void)event;
}

void ToolButton::OnMouseWheel(const MouseEvent& event) {
    if (m_OnMouseWheel && m_Geometry.Contains(event.position)) {
        m_OnMouseWheel(event.wheelDeltaY);
    }
}

float ToolSeparator::SeparatorHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ToolbarSeparatorHeight);
}

float ToolSeparator::SeparatorWidth() {
    return we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth);
}

ToolSeparator::ToolSeparator() {}

Size ToolSeparator::Measure(const Size& availableSize) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    return Size{ SeparatorWidth() * uiScale, SeparatorHeight() * uiScale };
}

void ToolSeparator::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ToolSeparator::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float sepH = SeparatorHeight() * uiScale;
    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
    const float centerX = m_Geometry.x + m_Geometry.width * 0.5f;
    we::runtime::kindui::ControlChrome::PaintVerticalSeparator(
        context,
        centerX,
        centerY - sepH * 0.5f,
        centerY + sepH * 0.5f,
        SeparatorWidth(),
        ColorToken::Separator);
}

} // namespace we::editor::toolbar
