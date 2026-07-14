#include "Widgets/ToolButton.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeColors.h"
#include "Core/Animator.h"
#include "Core/Icon.h"
#include "Core/ToolbarButtonChrome.h"
#include "Core/DPIContext.h"
#include "Rendering/IconMetrics.h"
#include <algorithm>
#include <cctype>

namespace WindEffects::Editor::UI {
namespace {
    using namespace ToolbarButtonChrome;

    float PressStrength(bool pressed, float pressAnim) {
        return pressed ? 1.0f : pressAnim;
    }

    float HoverDamping() {
        return ResolveThemeMetric(ThemeToken::HoverAnimationDamping);
    }

    float PressDamping() {
        return ResolveThemeMetric(ThemeToken::PressAnimationDamping);
    }

    Color ResolveInteractiveTextColor(float hoverAnim, float pressStrength, bool active) {
        return ResolveThemeTextForState(hoverAnim > 0.01f || pressStrength > 0.01f, active);
    }

    Color ResolvePlayIconColor(float hoverAnim, float pressStrength, bool active) {
        return ResolveIconColor(IconColorRole::Primary, hoverAnim, pressStrength, active);
    }

    bool IsPlayTransportIcon(const std::string& iconName) {
        return iconName == Icons::MediaPlayName
            || iconName == Icons::PlaySolidName
            || iconName == Icons::PlayName;
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
}

ToolButton::ToolButton(const std::string& iconName, const std::string& label, std::function<void()> onClicked, const std::string& tooltip)
    : m_IconName(iconName)
    , m_Label(label)
    , m_Tooltip(tooltip)
    , m_OnClicked(onClicked)
    , m_Style(WidgetStyle::ToolButton())
{}

Size ToolButton::Measure(const Size& availableSize) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    if (m_ButtonStyle == ToolButtonStyle::WindowControl || m_ButtonStyle == ToolButtonStyle::WindowClose) {
        const float controlWidth = ThemeMetric(ThemeToken::WindowControlWidth) * uiScale;
        const float controlHeight = ThemeMetric(ThemeToken::TitleBarHeight) * uiScale;
        m_DesiredSize = Size{ controlWidth, controlHeight };
        return m_DesiredSize;
    }
    
    if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        const float controlSize = ThemeMetric(ThemeToken::IconButtonSize) * uiScale;
        m_DesiredSize = Size{ controlSize, controlSize };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarInline) {
        const float padH     = ChipHorizontalPad(uiScale);
        const float iconSz   = IconSize(uiScale);
        const float iconGap  = IconGapPx(uiScale);
        const float chevGap  = ChevronGapPx(uiScale);
        const float chevW    = IconMetrics::CompactDisplayPx();
        const float textSize = ThemeMetric(ThemeToken::TextSizeToolbar) * uiScale;
        const float controlH = ThemeMetric(ThemeToken::IconButtonSize) * uiScale;
        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);

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
        const float chevW    = IconMetrics::CompactDisplayPx();
        const float textSize = ThemeMetric(ThemeToken::TextSizeToolbar) * uiScale;
        const float controlH = ThemeMetric(ThemeToken::IconButtonSize) * uiScale;
        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);

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
        const float padH = 4.0f * uiScale;
        const float textSize = ThemeMetric(ThemeToken::TextSizeCaption) * uiScale;
        const float labelW = m_Label.empty() ? 0.0f : ApproxInlineTextWidth(m_Label, textSize);
        const float minW = 42.0f * uiScale;
        const float width = (std::max)(minW, labelW + padH * 2.0f);
        const float height = 34.0f * uiScale;
        m_DesiredSize = Size{ width, height };
        return m_DesiredSize;
    }

    // TransportButton: Play / Pause / Stop
    if (m_ButtonStyle == ToolButtonStyle::TransportButton || m_ButtonStyle == ToolButtonStyle::PlayButton) {
        const float controlSize = ThemeMetric(ThemeToken::IconButtonSize) * uiScale;
        m_DesiredSize = Size{ controlSize, controlSize };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly) {
        const float controlSize = ThemeMetric(ThemeToken::IconButtonSize) * uiScale;
        m_DesiredSize = Size{ controlSize, controlSize };
        return m_DesiredSize;
    }

    // Normal – used for labeled dropdowns (Platform, Settings)
    const float height  = ThemeMetric(ThemeToken::ButtonHeight) * uiScale;
    const float padL    = ThemeMetric(ThemeToken::ButtonPaddingHorizontal) * uiScale;
    const float padR    = ThemeMetric(ThemeToken::Space2) * uiScale;
    const float iconSz  = IconSize(uiScale);
    const float iconGap = 4.0f * uiScale;
    const float chevW   = IconMetrics::CompactDisplayPx();

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
    const bool isInline        = (m_ButtonStyle == ToolButtonStyle::ToolbarInline);
    const bool isViewportChip  = (m_ButtonStyle == ToolButtonStyle::ViewportChip);
    const bool isNormal        = (m_ButtonStyle == ToolButtonStyle::Normal);

    // ── Window controls (Minimize / Maximize / Close) ────────────────────────
    if (isWindowControl) {
        if (m_HoverAnim > 0.01f) {
            Color hoverBg = (m_ButtonStyle == ToolButtonStyle::WindowClose)
                            ? Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, ThemeColor(ThemeToken::CloseButtonHover), m_HoverAnim)
                            : Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, ThemeColor(ThemeToken::HoverBackground), m_HoverAnim);
            context.DrawRect(renderRect, hoverBg);
        }

        Color iconColor = ResolveIconColor(IconColorRole::Secondary, m_HoverAnim, pressStrength, false);

        const float iconSize = IconSize(uiScale);
        const Rect iconRect = PlaceIconInControl(renderRect, iconSize);
        IconPainter::DrawIcon(context, m_IconName, iconRect, iconColor);
        return;
    }

    // ── TitleBar tool buttons ────────────────────────────────────────────────
    if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        PaintIconButton(context, renderRect, m_HoverAnim, pressStrength, m_Active, m_ActiveAnim, uiScale);

        const float iconSize = IconSize(uiScale);
        Color iconColor = ResolveIconColor(m_HoverAnim, pressStrength, m_Active);
        IconPainter::DrawIcon(context, m_IconName, PlaceIconInControl(renderRect, iconSize), iconColor);
        return;
    }

    // ── Labeled transform tools (icon above text) ────────────────────────────
    if (isLabeled) {
        PaintIconButton(context, renderRect, m_HoverAnim, pressStrength, m_Active, m_ActiveAnim, uiScale);

        const float iconSize = PrimaryIconSize(uiScale);
        const float textSize = ThemeMetric(ThemeToken::TextSizeCaption) * uiScale;
        const float labelGap = 2.0f * uiScale;
        const float contentH = iconSize + labelGap + textSize;
        const float topY = renderRect.y + (renderRect.height - contentH) * 0.5f;

        Color iconColor = ResolveIconColor(m_HoverAnim, pressStrength, m_Active);

        Rect iconBand{ renderRect.x, topY, renderRect.width, iconSize };
        IconPainter::DrawIcon(context, m_IconName, PlaceIconInControl(iconBand, iconSize), iconColor);

        if (!m_Label.empty()) {
            const float labelW = ApproxInlineTextWidth(m_Label, textSize);
            const float labelX = renderRect.x + (renderRect.width - labelW) * 0.5f;
            const float labelY = topY + iconSize + labelGap;
            Color labelColor = m_Active
                ? ThemeColor(ThemeToken::IconAccent)
                : ThemeColor(ThemeToken::TextSecondary);
            if (m_HoverAnim > 0.01f && !m_Active) {
                labelColor = Color::Lerp(labelColor, ThemeColor(ThemeToken::TextPrimary), m_HoverAnim);
            }
            context.DrawText(m_Label, Point{ labelX, labelY }, labelColor, textSize);
        }
        return;
    }

    // ── Inline toolbar items (Windows / MyProject chip dropdowns) ────────────
    if (isInline) {
        if (!m_Chromeless) {
            PaintChipDropdown(context, renderRect, m_HoverAnim, pressStrength, uiScale);
        } else {
            PaintIconButton(context, renderRect, m_HoverAnim, pressStrength, m_Active, m_ActiveAnim, uiScale);
        }

        const float iconSize  = IconSize(uiScale);
        const float textSize  = ThemeMetric(ThemeToken::TextSizeToolbar) * uiScale;
        const float iconGap   = IconGapPx(uiScale);
        const float chevGap   = ChevronGapPx(uiScale);
        const float padH      = ChipHorizontalPad(uiScale);
        Color iconColor = ResolveIconColor(m_HoverAnim, pressStrength, false);

        float currentX = renderRect.x + padH;
        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);
        if (hasIcon) {
            Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
            IconPainter::DrawIcon(context, m_IconName, PlaceIconInControl(iconBand, iconSize), iconColor);
            currentX += iconSize + iconGap;
        }

        if (!m_Label.empty()) {
            Color textColor = ResolveInteractiveTextColor(m_HoverAnim, pressStrength, false);
            context.DrawText(m_Label, Point{ currentX, centerY - textSize * 0.5f }, textColor, textSize);
            currentX += ApproxInlineTextWidth(m_Label, textSize);
        }

        if (m_IsDropdown) {
            currentX += chevGap;
            const float display = IconMetrics::CompactDisplayPx();
            Rect chevronControl{ currentX, centerY - display * 0.5f, display, display };
            IconPainter::DrawCompactIcon(context, Icons::ChevronDownName, chevronControl, iconColor);
        }
        return;
    }

    // ── Individual floating viewport controls ────────────────────────────────
    if (isViewportChip) {
        PaintViewportChip(context, renderRect, m_HoverAnim, pressStrength, uiScale);

        const float iconSize  = IconSize(uiScale);
        const float textSize  = ThemeMetric(ThemeToken::TextSizeToolbar) * uiScale;
        const float iconGap   = IconGapPx(uiScale);
        const float chevGap   = ChevronGapPx(uiScale);
        const float padH      = ChipHorizontalPad(uiScale);
        const float chevSlot  = IconSize(uiScale);
        Color iconColor = ResolveIconColor(m_HoverAnim, pressStrength, m_Active);

        float currentX = renderRect.x + padH;
        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);
        if (hasIcon) {
            Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
            IconPainter::DrawIcon(context, m_IconName, PlaceIconInControl(iconBand, iconSize), iconColor);
            currentX += iconSize + iconGap;
        }

        if (!m_Label.empty()) {
            Color textColor = ResolveInteractiveTextColor(m_HoverAnim, pressStrength, m_Active);
            context.DrawText(m_Label, Point{ currentX, centerY - textSize * 0.5f }, textColor, textSize);
            currentX += ApproxInlineTextWidth(m_Label, textSize);
        }

        if (m_IsDropdown) {
            currentX += chevGap;
            const float display = IconMetrics::CompactDisplayPx();
            Rect chevronControl{ currentX, centerY - display * 0.5f, display, display };
            IconPainter::DrawCompactIcon(context, Icons::ChevronDownName, chevronControl, iconColor);
        }
        return;
    }

    // ── Toolbar icon-only buttons (includes Play / Pause / Stop) ─────────────
    if (isToolbarIcon) {
        PaintIconButton(context, renderRect, m_HoverAnim, pressStrength, m_Active, m_ActiveAnim, uiScale);

        const bool isTransport = (m_ButtonStyle == ToolButtonStyle::TransportButton
            || m_ButtonStyle == ToolButtonStyle::PlayButton);
        const float iconSize = isTransport ? PrimaryIconSize(uiScale) : IconSize(uiScale);
        Color iconColor = IsPlayTransportIcon(m_IconName)
            ? ResolvePlayIconColor(m_HoverAnim, pressStrength, m_Active)
            : ResolveIconColor(m_HoverAnim, pressStrength, m_Active);
        IconPainter::DrawIcon(context, m_IconName, PlaceIconInControl(renderRect, iconSize), iconColor);
        return;
    }

    // ── Normal labeled buttons (legacy dropdown style) ───────────────────────
    if (isNormal) {
        const bool isDropdownControl = m_IsDropdown;
        Color bg{ 0.0f, 0.0f, 0.0f, 0.0f };
        bool drawBg = false;
        if (pressStrength > 0.01f) {
            bg = ResolveThemeColor(ThemeToken::PressedBackground);
            bg.a *= pressStrength;
            drawBg = true;
        } else if (m_HoverAnim > 0.01f) {
            bg = Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, ThemeColor(ThemeToken::HoverBackground), m_HoverAnim);
            drawBg = true;
        } else if (m_Active) {
            bg = ThemeColor(ThemeToken::SelectedBackground);
            drawBg = true;
        } else if (isDropdownControl) {
            bg = ThemeColor(ThemeToken::StatusBarBackground);
            drawBg = true;
        }

        if (drawBg) context.DrawRoundedRect(renderRect, bg, ThemeMetric(ThemeToken::CornerRadiusSmall) * uiScale);

        if (isDropdownControl) {
            Color borderCol = m_HoverAnim > 0.01f
                ? ThemeColor(ThemeToken::BorderLight)
                : ThemeColor(ThemeToken::BorderDefault);
            context.DrawRoundedRectOutline(renderRect, borderCol, 1.0f * uiScale, ThemeMetric(ThemeToken::CornerRadiusSmall) * uiScale);
        }

        Color iconColor = ResolveIconColor(m_HoverAnim, pressStrength, m_Active);

        const float iconSize = IconSize(uiScale);
        float currentX = renderRect.x + ThemeMetric(ThemeToken::ButtonPaddingHorizontal) * uiScale;

        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);
        if (hasIcon) {
            if (m_Label.empty() && !m_IsDropdown) {
                IconPainter::DrawIcon(context, m_IconName, PlaceIconInControl(renderRect, iconSize), iconColor);
            } else if (m_Label.empty() && m_IsDropdown) {
                currentX = renderRect.x + ChipHorizontalPad(uiScale);
                Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
                IconPainter::DrawIcon(context, m_IconName, PlaceIconInControl(iconBand, iconSize), iconColor);
            } else {
                Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
                IconPainter::DrawIcon(context, m_IconName, PlaceIconInControl(iconBand, iconSize), iconColor);
                currentX += iconSize + IconGapPx(uiScale);
            }
        } else {
            currentX = renderRect.x + ThemeMetric(ThemeToken::ButtonPaddingHorizontal) * uiScale;
        }

        if (!m_Label.empty()) {
            const float textSize = ThemeMetric(ThemeToken::TextSizeToolbar) * uiScale;
            context.DrawText(m_Label, Point{ currentX, centerY - textSize / 2.0f }, iconColor, textSize);
        }

        if (m_IsDropdown) {
            const float display = IconMetrics::CompactDisplayPx();
            const float chevronX = renderRect.x + renderRect.width - ChipHorizontalPad(uiScale) - display;
            Rect chevronControl{ chevronX, centerY - display * 0.5f, display, display };
            IconPainter::DrawCompactIcon(context, Icons::ChevronDownName, chevronControl, iconColor);
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
    // Hover state is handled by EventSystem
}

void ToolButton::OnMouseWheel(const MouseEvent& event) {
    if (m_OnMouseWheel && m_Geometry.Contains(event.position)) {
        m_OnMouseWheel(event.wheelDeltaY);
    }
}

// ToolSeparator implementation
ToolSeparator::ToolSeparator() {}

Size ToolSeparator::Measure(const Size& availableSize) {
    return Size{ SEPARATOR_WIDTH, SEPARATOR_HEIGHT };
}

void ToolSeparator::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ToolSeparator::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    float centerY = m_Geometry.y + m_Geometry.height / 2.0f;
    float halfHeight = (SEPARATOR_HEIGHT * uiScale) / 2.0f;
    
    Rect lineRect{ m_Geometry.x, centerY - halfHeight, SEPARATOR_WIDTH * uiScale, SEPARATOR_HEIGHT * uiScale };
    context.DrawRect(lineRect, ThemeColor(ThemeToken::Separator));
}

} // namespace WindEffects::Editor::UI
