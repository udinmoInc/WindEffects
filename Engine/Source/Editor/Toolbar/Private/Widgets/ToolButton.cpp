#include "Widgets/ToolButton.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include "Core/DPIContext.h"
#include <algorithm>
#include <cctype>

namespace we::UI {
namespace {
    float PressStrength(bool pressed, float pressAnim) {
        return pressed ? 1.0f : pressAnim;
    }

    Color MakePressBackground(float strength) {
        Color pressed = Theme::Get().PressedOverlay;
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
            Color hoverBg = Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, Theme::Get().HoverButton, hoverAnim);
            context.DrawRoundedRect(rect, hoverBg, radius);
        } else if (active) {
            context.DrawRoundedRect(rect, Theme::Get().SelectedBg, radius);
        }
    }

    Color ResolveInteractiveIconColor(float hoverAnim, float pressStrength, bool active) {
        Color iconColor = Theme::Get().IconDefault;
        if (hoverAnim > 0.01f || pressStrength > 0.01f || active) {
            iconColor = Color::Lerp(iconColor, Theme::Get().IconHover, std::max({hoverAnim, pressStrength, active ? 1.0f : 0.0f}));
        }
        return iconColor;
    }

    Color ResolveInteractiveTextColor(float hoverAnim, float pressStrength, bool active) {
        Color textColor = Theme::Get().TextPrimary;
        if (hoverAnim > 0.01f || pressStrength > 0.01f || active) {
            textColor = Color::Lerp(Theme::Get().TextSecondary, Color::White(), std::max({hoverAnim, pressStrength, active ? 1.0f : 0.0f}));
        }
        return textColor;
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
        m_DesiredSize = Size{ 30.0f * uiScale, 30.0f * uiScale };
        return m_DesiredSize;
    }
    
    if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        m_DesiredSize = Size{ 26.0f * uiScale, 26.0f * uiScale };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarInline) {
        const float padH     = 6.0f * uiScale;  // extra-compact inline horizontal padding
        const float iconSz   = 18.0f * uiScale;
        const float iconGap  = 3.0f * uiScale;
        const float chevGap  = 1.0f * uiScale;
        const float chevW    = 8.0f * uiScale;
        const float textSize = 13.0f * uiScale;
        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);
        
        float textW = m_Label.empty() ? 0.0f : ApproxInlineTextWidth(m_Label, textSize);
        
        float width = padH * 2.0f; // both sides
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
        
        m_DesiredSize = Size{ width, 22.0f * uiScale };
        return m_DesiredSize;
    }

    // TransportButton: Play / Pause / Stop – compact hit area
    if (m_ButtonStyle == ToolButtonStyle::TransportButton || m_ButtonStyle == ToolButtonStyle::PlayButton) {
        m_DesiredSize = Size{ 22.0f * uiScale, 22.0f * uiScale };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly) {
        m_DesiredSize = Size{ 22.0f * uiScale, 22.0f * uiScale };
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
                            ? Color{ 0.8f, 0.2f, 0.2f, m_HoverAnim }
                            : Color{ 0.196f, 0.196f, 0.196f, m_HoverAnim };
            context.DrawRoundedRect(renderRect, hoverBg, 4.0f * uiScale);
        }
        Color iconColor = Theme::Get().IconDefault;
        if (m_HoverAnim > 0.01f) iconColor = Color::Lerp(iconColor, Theme::Get().IconHover, m_HoverAnim);
        if (m_ButtonStyle == ToolButtonStyle::WindowClose && m_HoverAnim > 0.5f)
            iconColor = Theme::Get().IconActive;
        float iconSize = 16.0f * uiScale;
        // Use smaller icon size for square/stop icon to match close button proportions
        if (m_IconName == Icons::StopName) iconSize = 14.0f * uiScale;
        float drawX = renderRect.x + (renderRect.width - iconSize) / 2.0f;
        IconPainter::DrawIcon(context, m_IconName, Rect{ drawX, centerY - iconSize / 2.0f, iconSize, iconSize }, iconColor);
        return;
    }

    // ── Transport buttons (Play / Pause / Stop) ──────────────────────────────
    if (isTransport) {
        DrawInteractiveBackground(context, renderRect, m_HoverAnim, pressStrength, m_Active, 3.0f * uiScale);

        const float iconSize = 18.0f * uiScale;
        Color iconColor = ResolveInteractiveIconColor(m_HoverAnim, pressStrength, m_Active);
        float drawX = renderRect.x + (renderRect.width  - iconSize) / 2.0f;
        float drawY = renderRect.y + (renderRect.height - iconSize) / 2.0f;
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

    // ── Inline toolbar items (Platform / Settings) – transparent, hover only ─
    if (isInline) {
        DrawInteractiveBackground(context, renderRect, m_HoverAnim, pressStrength, m_Active, 3.0f * uiScale);

        const float iconSize  = 18.0f * uiScale;
        const float textSize  = 13.0f * uiScale;
        const float iconGap   = 3.0f * uiScale;
        const float padLeft   = 6.0f * uiScale;
        const float padRight  = 4.0f * uiScale;
        Color textColor = ResolveInteractiveTextColor(m_HoverAnim, pressStrength, m_Active);

        float currentX = renderRect.x + padLeft;
        const bool hasIcon = !m_IconName.empty() && Icons::IsKnownIcon(m_IconName);
        if (hasIcon) {
            IconPainter::DrawIcon(context, m_IconName, Rect{ currentX, centerY - iconSize / 2.0f, iconSize, iconSize }, textColor);
            currentX += iconSize;
            if (!m_Label.empty()) {
                currentX += iconGap;
            }
        }

        if (!m_Label.empty()) {
            context.DrawText(m_Label, Point{ currentX, centerY - textSize / 2.0f }, textColor, textSize);
        }

        if (m_IsDropdown) {
            const float chevSize = 8.0f * uiScale;
            float chevX = renderRect.x + renderRect.width - padRight - chevSize;
            IconPainter::DrawIcon(context, Icons::ChevronDownName, Rect{ chevX, centerY - chevSize / 2.0f, chevSize, chevSize }, textColor);
        }
        return;
    }

    // ── Toolbar icon-only buttons ────────────────────────────────────────────
    if (isToolbarIcon) {
        DrawInteractiveBackground(context, renderRect, m_HoverAnim, pressStrength, m_Active, 3.0f * uiScale);

        const float iconSize = 18.0f * uiScale;
        Color iconColor = ResolveInteractiveIconColor(m_HoverAnim, pressStrength, m_Active);
        float drawX = renderRect.x + (renderRect.width - iconSize) / 2.0f;
        float drawY = renderRect.y + (renderRect.height - iconSize) / 2.0f;
        IconPainter::DrawIcon(context, m_IconName, Rect{ drawX, drawY, iconSize, iconSize }, iconColor);
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
            bg = Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, Theme::Get().HoverButton, m_HoverAnim);
            drawBg = true;
        } else if (m_Active) {
            bg = Theme::Get().SelectedBg;
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
            const float textSize = Theme::Get().TextSizeToolbar * uiScale;
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
    context.DrawRect(lineRect, Theme::Get().BorderDefault);
}

} // namespace we::editor::toolbar::UI
