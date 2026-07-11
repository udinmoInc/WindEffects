#include "Widgets/ToolButton.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "Core/Animator.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include "Core/DPIContext.h"
#include <algorithm>
#include <cctype>

namespace WindEffects::Editor::UI {
namespace {
    float PressStrength(bool pressed, float pressAnim) {
        return pressed ? 1.0f : pressAnim;
    }

    Color MakePressBackground(float strength) {
        Color pressed = ResolveThemeColor(ThemeToken::PressedBackground);
        pressed.a *= strength;
        return pressed;
    }

    void DrawInteractiveBackground(
        PaintContext& context,
        const Rect& rect,
        float hoverAnim,
        float pressStrength,
        bool active,
        float radius = 3.0f)
    {
        if (pressStrength > 0.01f) {
            context.DrawRoundedRect(rect, MakePressBackground(pressStrength), radius);
        } else if (hoverAnim > 0.01f) {
            Color hoverBg = Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, ResolveThemeColor(ThemeToken::HoverBackground), hoverAnim);
            context.DrawRoundedRect(rect, hoverBg, radius);
        } else if (active) {
            context.DrawRoundedRect(rect, ResolveThemeColor(ThemeToken::SelectedBackground), radius);
        }
    }

    Color ResolveInteractiveIconColor(float hoverAnim, float pressStrength, bool active) {
        Color iconColor = ResolveThemeColor(ThemeToken::IconDefault);
        if (hoverAnim > 0.01f || pressStrength > 0.01f || active) {
            iconColor = Color::Lerp(iconColor, ResolveThemeColor(ThemeToken::IconHover), std::max({hoverAnim, pressStrength, active ? 1.0f : 0.0f}));
        }
        return iconColor;
    }

    Color ResolveInteractiveTextColor(float hoverAnim, float pressStrength, bool active) {
        return ResolveThemeTextForState(hoverAnim > 0.01f || pressStrength > 0.01f, active);
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
        m_DesiredSize = Size{ 26.0f * uiScale, 26.0f * uiScale };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarInline) {
        const float padH     = 8.0f * uiScale;
        const float iconSz   = 16.0f * uiScale;
        const float iconGap  = 6.0f * uiScale;
        const float chevGap  = 8.0f * uiScale;
        const float chevW    = 10.0f * uiScale;
        const float textSize = 12.0f * uiScale;
        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);

        float textW = m_Label.empty() ? 0.0f : ApproxInlineTextWidth(m_Label, textSize);

        float width = padH * 2.0f;
        if (hasIcon) {
            width += iconSz;
            if (!m_Label.empty()) {
                width += iconGap;
            }
        }
        width += textW;
        if (m_IsDropdown) {
            width += chevGap + chevW;
        }

        m_DesiredSize = Size{ width, ThemeMetric(ThemeToken::HeaderControlHeight) * uiScale };
        return m_DesiredSize;
    }

    // TransportButton: Play / Pause / Stop
    if (m_ButtonStyle == ToolButtonStyle::TransportButton || m_ButtonStyle == ToolButtonStyle::PlayButton) {
        const float controlSize = ThemeMetric(ThemeToken::HeaderControlHeight) * uiScale;
        m_DesiredSize = Size{ controlSize, controlSize };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly) {
        m_DesiredSize = Size{ ThemeMetric(ThemeToken::IconButtonSize) * uiScale, ThemeMetric(ThemeToken::IconButtonSize) * uiScale };
        return m_DesiredSize;
    }

    // Normal – used for labeled dropdowns (Platform, Settings)
    const float height  = 22.0f * uiScale;
    const float padL    = 12.0f * uiScale; // left padding (icon start) – 12px for breathing room
    const float padR    = 10.0f * uiScale; // right padding (after chevron) – 10px
    const float iconSz  = 18.0f * uiScale;
    const float iconGap = 6.0f * uiScale;  // gap between icon and text
    const float chevW   = 16.0f * uiScale; // chevron area (icon 8px + gap 8px)

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
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 40.0f);
    m_ActiveAnim = Animator::Damp(m_ActiveAnim, m_Active ? 1.0f : 0.0f, 15.0f);

    const float pressStrength = PressStrength(m_Pressed, m_PressAnim);

    Rect renderRect = m_Geometry;
    float centerY   = renderRect.y + renderRect.height / 2.0f;

    const bool isWindowControl = (m_ButtonStyle == ToolButtonStyle::WindowControl ||
                                   m_ButtonStyle == ToolButtonStyle::WindowClose);
    const bool isTransport     = (m_ButtonStyle == ToolButtonStyle::TransportButton ||
                                   m_ButtonStyle == ToolButtonStyle::PlayButton);
    const bool isToolbarIcon   = (m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly);
    const bool isInline        = (m_ButtonStyle == ToolButtonStyle::ToolbarInline);
    const bool isNormal        = (m_ButtonStyle == ToolButtonStyle::Normal);

    // ── Window controls (Minimize / Maximize / Close) ────────────────────────
    if (isWindowControl) {
        if (m_HoverAnim > 0.01f) {
            Color hoverBg = (m_ButtonStyle == ToolButtonStyle::WindowClose)
                            ? Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, ThemeColor(ThemeToken::CloseButtonHover), m_HoverAnim)
                            : Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, ThemeColor(ThemeToken::HoverBackground), m_HoverAnim);
            context.DrawRect(renderRect, hoverBg);
        }

        Color iconColor = ThemeColor(ThemeToken::IconDefault);
        if (m_HoverAnim > 0.01f) {
            iconColor = Color::Lerp(iconColor, ThemeColor(ThemeToken::IconHover), m_HoverAnim);
        }
        if (m_ButtonStyle == ToolButtonStyle::WindowClose && m_HoverAnim > 0.35f) {
            iconColor = ThemeColor(ThemeToken::IconActive);
        }

        const float iconSize = 16.0f * uiScale;
        const float drawX = renderRect.x + (renderRect.width - iconSize) * 0.5f;
        IconPainter::DrawIcon(context, m_IconName, Rect{ drawX, centerY - iconSize * 0.5f, iconSize, iconSize }, iconColor);
        return;
    }

    // ── Transport buttons (Play / Pause / Stop) ──────────────────────────────
    if (isTransport) {
        DrawInteractiveBackground(context, renderRect, m_HoverAnim, pressStrength, m_Active, 4.0f * uiScale);

        const float iconSize = 16.0f * uiScale;
        Color iconColor = ResolveInteractiveIconColor(m_HoverAnim, pressStrength, m_Active);
        if (m_IconName == Icons::PlayName) {
            iconColor = ThemeColor(ThemeToken::Success);
        }
        const float drawX = renderRect.x + (renderRect.width  - iconSize) * 0.5f;
        const float drawY = renderRect.y + (renderRect.height - iconSize) * 0.5f;
        IconPainter::DrawIcon(context, m_IconName, Rect{ drawX, drawY, iconSize, iconSize }, iconColor);
        return;
    }

    // ── TitleBar tool buttons ────────────────────────────────────────────────
    if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        DrawInteractiveBackground(context, renderRect, m_HoverAnim, pressStrength, m_Active, 3.0f * uiScale);

        const float iconSize = 15.0f * uiScale;
        Color iconColor = ResolveInteractiveIconColor(m_HoverAnim, pressStrength, m_Active);
        float drawX = renderRect.x + (renderRect.width  - iconSize) / 2.0f;
        float drawY = renderRect.y + (renderRect.height - iconSize) / 2.0f;
        IconPainter::DrawIcon(context, m_IconName, Rect{ drawX, drawY, iconSize, iconSize }, iconColor);
        return;
    }

    // ── Inline toolbar items (Platform / Settings) ───────────────────────────
    if (isInline) {
        DrawInteractiveBackground(context, renderRect, m_HoverAnim, pressStrength, m_Active, 4.0f * uiScale);

        const float iconSize  = 16.0f * uiScale;
        const float textSize  = 12.0f * uiScale;
        const float iconGap   = 6.0f * uiScale;
        const float padLeft   = 8.0f * uiScale;
        const float padRight  = 8.0f * uiScale;
        const float chevGap   = 8.0f * uiScale;
        Color textColor = ResolveInteractiveTextColor(m_HoverAnim, pressStrength, m_Active);

        float currentX = renderRect.x + padLeft;
        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);
        if (hasIcon) {
            IconPainter::DrawIcon(context, m_IconName, Rect{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize }, textColor);
            currentX += iconSize;
            if (!m_Label.empty()) {
                currentX += iconGap;
            }
        }

        if (!m_Label.empty()) {
            context.DrawText(m_Label, Point{ currentX, centerY - textSize * 0.5f }, textColor, textSize);
        }

        if (m_IsDropdown) {
            const float chevSize = 10.0f * uiScale;
            float chevX = renderRect.x + renderRect.width - padRight - chevSize;
            IconPainter::DrawIcon(context, Icons::ChevronDownName, Rect{ chevX, centerY - chevSize * 0.5f, chevSize, chevSize }, textColor);
        }
        return;
    }

    // ── Toolbar icon-only buttons ────────────────────────────────────────────
    if (isToolbarIcon) {
        DrawInteractiveBackground(context, renderRect, m_HoverAnim, pressStrength, m_Active, 3.0f * uiScale);

        const float iconSize = ThemeMetric(ThemeToken::IconSizeToolbar) * uiScale;
        Color iconColor = ResolveInteractiveIconColor(m_HoverAnim, pressStrength, m_Active);
        if (m_Active) {
            iconColor = ThemeColor(ThemeToken::AccentPrimary);
        }
        float drawX = renderRect.x + (renderRect.width - iconSize) / 2.0f;
        float drawY = renderRect.y + (renderRect.height - iconSize) / 2.0f;
        IconPainter::DrawIcon(context, m_IconName, Rect{ drawX, drawY, iconSize, iconSize }, iconColor);

        if (m_Active) {
            Rect underline{
                renderRect.x + 4.0f * uiScale,
                renderRect.y + renderRect.height - 2.0f * uiScale,
                renderRect.width - 8.0f * uiScale,
                2.0f * uiScale
            };
            context.DrawRect(underline, ThemeColor(ThemeToken::ActiveTabLine));
        }
        return;
    }

    // ── Normal labeled buttons (legacy dropdown style) ───────────────────────
    if (isNormal) {
        const bool isDropdownControl = m_IsDropdown;
        Color bg{ 0.0f, 0.0f, 0.0f, 0.0f };
        bool drawBg = false;
        if (pressStrength > 0.01f) {
            bg = MakePressBackground(pressStrength);
            drawBg = true;
        } else if (m_HoverAnim > 0.01f) {
            bg = Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, ThemeColor(ThemeToken::HoverBackground), m_HoverAnim);
            drawBg = true;
        } else if (m_Active) {
            bg = ThemeColor(ThemeToken::SelectedBackground);
            drawBg = true;
        } else if (isDropdownControl) {
            bg = Color{ 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
            drawBg = true;
        }

        if (drawBg) context.DrawRoundedRect(renderRect, bg, 4.0f * uiScale);

        if (isDropdownControl) {
            Color borderCol = m_HoverAnim > 0.01f
                ? Color{ 0.298f, 0.298f, 0.298f, 1.0f }  // #4C4C4C
                : Color{ 0.227f, 0.227f, 0.227f, 1.0f }; // #3A3A3A
            context.DrawRoundedRectOutline(renderRect, borderCol, 1.0f * uiScale, 4.0f * uiScale);
        }

        Color iconColor = ResolveInteractiveIconColor(m_HoverAnim, pressStrength, m_Active);

        const float iconSize = 18.0f * uiScale;
        float currentX = renderRect.x + 12.0f * uiScale;

        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);
        if (hasIcon) {
            if (m_Label.empty() && !m_IsDropdown) {
                float drawX = renderRect.x + (renderRect.width - iconSize) / 2.0f;
                IconPainter::DrawIcon(context, m_IconName, Rect{ drawX, centerY - iconSize / 2.0f, iconSize, iconSize }, iconColor);
            } else if (m_Label.empty() && m_IsDropdown) {
                currentX = renderRect.x + 8.0f * uiScale;
                IconPainter::DrawIcon(context, m_IconName, Rect{ currentX, centerY - iconSize / 2.0f, iconSize, iconSize }, iconColor);
            } else {
                currentX = renderRect.x + 12.0f * uiScale;
                IconPainter::DrawIcon(context, m_IconName, Rect{ currentX, centerY - iconSize / 2.0f, iconSize, iconSize }, iconColor);
                currentX += iconSize + 6.0f * uiScale;
            }
        } else {
            currentX = renderRect.x + 12.0f * uiScale;
        }

        if (!m_Label.empty()) {
            const float textSize = ThemeMetric(ThemeToken::TextSizeToolbar) * uiScale;
            context.DrawText(m_Label, Point{ currentX, centerY - textSize / 2.0f }, iconColor, textSize);
        }

        if (m_IsDropdown) {
            const float chevSize = 8.0f * uiScale;
            float chevX = renderRect.x + renderRect.width - 10.0f * uiScale - chevSize;
            IconPainter::DrawIcon(context, Icons::ChevronDownName, Rect{ chevX, centerY - chevSize / 2.0f, chevSize, chevSize }, iconColor);
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
    float centerY = m_Geometry.y + m_Geometry.height / 2.0f;
    float halfHeight = SEPARATOR_HEIGHT / 2.0f;
    
    Rect lineRect{ m_Geometry.x, centerY - halfHeight, SEPARATOR_WIDTH, SEPARATOR_HEIGHT };
    context.DrawRect(lineRect, ThemeColor(ThemeToken::BorderDefault));
}

} // namespace WindEffects::Editor::UI
