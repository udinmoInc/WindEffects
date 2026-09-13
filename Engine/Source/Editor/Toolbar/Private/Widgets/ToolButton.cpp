// ==============================================================================
// WindEffects — Toolbar — ToolButton
// UI widget used by the Toolbar module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Widgets/ToolButton.h"
#include "KindUI/Layout/AutoAlign.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Profiling/UiInputDebug.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Text/Layout/TextStyle.h"

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
namespace LayoutMetrics = ::we::runtime::kindui::LayoutMetrics;
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
                        width += textSize * 0.54f;
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

    float LabelWidth(const std::string& label, float textSize, float& cachedTextSize, float& cachedWidth) {
        if (cachedTextSize != textSize) {
            cachedTextSize = textSize;
            cachedWidth = label.empty() ? 0.0f : ApproxInlineTextWidth(label, textSize);
        }
        return cachedWidth;
    }
}

ToolButton::ToolButton(WindIconRef icon, const std::string& label, std::function<void()> onClicked, const std::string&
    tooltip)
    : m_Icon(icon)
    , m_Label(label)
    , m_Tooltip(tooltip)
    , m_OnClicked(std::move(onClicked))
    , m_Style(WidgetStyle::ToolButton())
{}

ToolButton::~ToolButton() = default;

void ToolButton::SetIcon(WindIconRef icon) {
    m_Icon = icon;
}

const std::string& ToolButton::GetTooltip() const {
    return m_Tooltip;
}

void ToolButton::SetLabel(const std::string& label) {
    m_Label = label;
    m_CachedLabelWidthTextSize = -1.0f;
}

const std::string& ToolButton::GetLabel() const {
    return m_Label;
}

void ToolButton::SetOnMouseWheel(std::function<void(float wheelDeltaY)> onMouseWheel) {
    m_OnMouseWheel = std::move(onMouseWheel);
}

void ToolButton::SetTooltip(const std::string& tooltip) {
    m_Tooltip = tooltip;
}

void ToolButton::SetOnClicked(std::function<void()> onClicked) {
    m_OnClicked = std::move(onClicked);
}

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

        float textW = LabelWidth(m_Label, textSize, m_CachedLabelWidthTextSize, m_CachedLabelWidth);
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

        float textW = LabelWidth(m_Label, textSize, m_CachedLabelWidthTextSize, m_CachedLabelWidth);

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
        const float iconSz   = m_Icon.IsValid() ? static_cast<float>(m_Icon.sizePx) : IconSize(uiScale);
        const float iconGap  = IconGapPx(uiScale);
        const float chevGap  = ChevronGapPx(uiScale);
        const float chevW    = kChevronSlotPx;
        const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
        const float controlH = ToolbarButtonChrome::RowContentHeight(uiScale);
        const bool hasIcon = m_Icon.IsValid();

        float textW = LabelWidth(m_Label, textSize, m_CachedLabelWidthTextSize, m_CachedLabelWidth);

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
        const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
        const float iconSz = m_Icon.IsValid() ? static_cast<float>(m_Icon.sizePx) : IconSize(uiScale);
        const float iconGap = IconGapPx(uiScale);
        const float labelW = LabelWidth(m_Label, textSize, m_CachedLabelWidthTextSize, m_CachedLabelWidth);
        const float minW = ThemeMetric(MetricToken::ToolbarLabeledMinWidth) * uiScale;
        float width = padH * 2.0f;
        if (m_Icon.IsValid()) {
            width += iconSz;
            if (!m_Label.empty() || m_IsDropdown) {
                width += iconGap;
            }
        }
        width += labelW;
        if (m_IsDropdown) {
            width += iconGap + kChevronSlotPx;
        }
        width = (std::max)(minW, width);
        const float height = ToolbarButtonChrome::ItemSize(uiScale);
        m_DesiredSize = Size{ width, height };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::TransportButton || m_ButtonStyle == ToolButtonStyle::PlayButton ||
        m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly) {
        const float itemH = ToolbarButtonChrome::ItemSize(uiScale);
        const float width = (m_ButtonStyle == ToolButtonStyle::TransportButton || m_ButtonStyle ==
            ToolButtonStyle::PlayButton)
            ? std::round(itemH * 1.15f)
            : itemH;
        m_DesiredSize = Size{ width, itemH };
        return m_DesiredSize;
    }

    const float height  = ToolbarButtonChrome::RowContentHeight(uiScale);
    const float padL    = 10.0f * uiScale;
    const float padR    = 10.0f * uiScale;
    const float iconSz  = IconSize(uiScale);
    const float iconGap = ThemeMetric(MetricToken::Space1) * uiScale;
    const float chevW   = kChevronSlotPx;

    float width = padL;
    if (m_Icon.IsValid()) {
        width += iconSz;
        if (!m_Label.empty()) {
            width += iconGap;
        }
    }
    if (!m_Label.empty()) {
        width += m_Label.length() * (7.2f * uiScale);
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

void ToolButton::Tick(float deltaTime) {
    const float targetHover = m_Hovered ? 1.0f : 0.0f;
    const float targetPress = m_Pressed ? 1.0f : 0.0f;
    const float targetActive = m_Active ? 1.0f : 0.0f;

    m_HoverAnim = Animator::Damp(m_HoverAnim, targetHover, HoverDamping() * 0.8f);
    m_PressAnim = Animator::Damp(m_PressAnim, targetPress, PressDamping());
    m_ActiveAnim = Animator::Damp(m_ActiveAnim, targetActive, HoverDamping());


    Widget::Tick(deltaTime);
}

void ToolButton::Paint(PaintContext& context) {
    if (!m_Visible) return;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float pressStrength = PressStrength(m_Pressed, m_PressAnim);
    const float activeStrength = (std::max)(m_HoverAnim, pressStrength);
    Rect renderRect = m_Geometry;
    float centerY   = renderRect.y + renderRect.height / 2.0f;

    const bool isWindowControl = (m_ButtonStyle == ToolButtonStyle::WindowControl ||
                                   m_ButtonStyle == ToolButtonStyle::WindowClose);
    const bool isViewportChip  = (m_ButtonStyle == ToolButtonStyle::ViewportChip);

    if (activeStrength > 0.01f && !m_Chromeless && !isViewportChip) {
        Color hoverBg;
        float radius = 4.0f * uiScale;
        if (m_ButtonStyle == ToolButtonStyle::WindowClose) {
            hoverBg = m_Pressed ? ThemeColor(ColorToken::ButtonDangerPressed) :
                ThemeColor(ColorToken::CloseButtonHover);
            radius = 0.0f;
        } else {
            hoverBg = m_Pressed ? ThemeColor(ColorToken::ControlBackgroundPressed) :
                ThemeColor(ColorToken::HoverBackground);
        }
        hoverBg.a *= activeStrength;
        context.DrawRoundedRect(renderRect, hoverBg, radius);
    }

    if (isWindowControl) {
        const float iconSize = WindowControlIconSize(uiScale);
        PaintFloatingIcon(
            context, m_Icon, renderRect, iconSize, m_HoverAnim, pressStrength, false);
    } else if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        const float iconSize = IconSize(uiScale);
        PaintFloatingIcon(
            context, m_Icon, renderRect, iconSize, m_HoverAnim, pressStrength, m_Active);
    } else {
        const float iconSize = m_Icon.IsValid() ? static_cast<float>(m_Icon.sizePx) : IconSize(uiScale);
        const float textSize = (m_ButtonStyle == ToolButtonStyle::StatusBar)
            ? ThemeMetric(MetricToken::TextSizeSmall) * uiScale
            : ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
        const float iconGap  = IconGapPx(uiScale);
        const float chevGap  = ChevronGapPx(uiScale);
        const float padH     = ChipHorizontalPad(uiScale);

        const bool iconOnly = m_Icon.IsValid() && m_Label.empty() && !m_IsDropdown;

        if (iconOnly) {
            PaintFloatingIcon(
                context, m_Icon, renderRect, iconSize, m_HoverAnim, pressStrength, m_Active);
        } else {
            const auto layout = ::we::runtime::kindui::AutoAlign::ComputeGroupCenterLayout(
                renderRect, iconSize, m_Icon.IsValid(), m_Label, textSize, 16.0f, m_IsDropdown, iconGap);

            if (m_Icon.IsValid()) {
                const ::we::runtime::kindui::Rect iconRect = layout.iconRect;
                PaintFloatingIcon(
                    context, m_Icon, iconRect, iconSize, m_HoverAnim, pressStrength, m_Active);
            }

            if (!m_Label.empty()) {
                Color textColor = ResolveInteractiveTextColor(m_HoverAnim, pressStrength, m_Active);
                const ::we::runtime::kindui::Point textPos = layout.textPos;
                context.DrawText(
                    m_Label,
                    textPos,
                    textColor,
                    textSize,
                    we::runtime::text::layout::FontWeight::Regular);
            }

            if (m_IsDropdown) {
                const ::we::runtime::kindui::Rect chevRect = layout.chevronRect;
                PaintFloatingIcon(
                    context,
                    WindIcons::ChevronDownV212,
                    chevRect,
                    16.0f,
                    m_HoverAnim,
                    pressStrength,
                    m_Active);
            }
        }
    }

    if (m_Hovered && m_HoverAnim > 0.6f && !m_Tooltip.empty()) {
        const float tooltipPadX = 10.0f * uiScale;
        const float tooltipPadY = 5.0f * uiScale;
        const float tooltipTextSize = ThemeMetric(MetricToken::TextSizeSmall) * uiScale;
        const float tooltipTextW = m_Tooltip.length() * (6.8f * uiScale);
        const float tooltipW = tooltipTextW + tooltipPadX * 2.0f;
        const float tooltipH = tooltipTextSize + tooltipPadY * 2.0f;

        const Rect tooltipRect{
            renderRect.x + (renderRect.width - tooltipW) * 0.5f,
            renderRect.y + renderRect.height + 6.0f * uiScale,
            tooltipW,
            tooltipH
        };

        we::runtime::kindui::ControlChrome::PaintTooltipSurface(context, tooltipRect);
        context.DrawText(
            m_Tooltip,
            Point{ tooltipRect.x + tooltipPadX, tooltipRect.y + tooltipPadY },
            ThemeColor(ColorToken::TextPrimary),
            tooltipTextSize,
            we::runtime::text::layout::FontWeight::Regular);
    }
}

void ToolButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        SetPressed(true);
        m_PressAnim = 1.0f;
    }
}

void ToolButton::OnMouseUp(const MouseEvent& event) {
    if (ShouldFireClickOnLeftUp(event) && m_OnClicked) {
        std::string label = !m_Tooltip.empty() ? m_Tooltip
            : (!m_Label.empty() ? m_Label
                : (!GetId().empty() ? GetId() : "ToolButton"));
        we::runtime::kindui::UiInputDebug::OnClickInvoked("ToolButton", label);
        m_OnClicked();
    }
}

void ToolButton::OnCaptureLost() {
    SetPressed(false);
    m_PressAnim = 0.0f;
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
    return we::runtime::kindui::ds::Toolbar::SeparatorWidth();
}

ToolSeparator::ToolSeparator() {}

Size ToolSeparator::Measure(const Size& availableSize) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    m_DesiredSize = Size{
        SeparatorWidth() * uiScale,
        availableSize.height > 0.0f
            ? availableSize.height
            : SeparatorHeight() * uiScale
    };
    return m_DesiredSize;
}

void ToolSeparator::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ToolSeparator::Paint(PaintContext& context) {
    context.DrawSurface(
        m_Geometry,
        we::runtime::kindui::SurfaceRole::Workspace,
        0.0f,
        "ToolSeparator");
}

}

